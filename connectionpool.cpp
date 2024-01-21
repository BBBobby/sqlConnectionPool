#include"connectionpool.h"
#include<fstream>
#include<thread>
#include<functional>

// 连接池只需要一个实例，所以ConnectionPool以单例模式进行设计
ConnectionPool* ConnectionPool::getConnectionPool() {
	static ConnectionPool pool;
	return &pool;
}

std::atomic_bool ConnectionPool::isEnd_ = false;
ConnectionPool::ConnectionPool():connectionNum_(0) {
	// 没成功配置就直接返回
	if (!loadConfigFile()) {
		return;
	}

	// 创建连接并插入队列
	for (int i = 0; i < initConnection_; ++i) {
		Connection* conn = new Connection();
		conn->connect(ip_, port_, user_, password_, database_);
		connectionQue_.push(conn);
		conn->refreshIdleStart();
		++connectionNum_;
	}
	std::thread produceMore(std::bind(&ConnectionPool::produceMoreConnection, this));
	produceMore.detach();
	std::thread scanner(std::bind(&ConnectionPool::deleteExcessConnection, this));
	scanner.detach();
}


/*
delete连接池队列里的所有连接。
（2）为了防止用户没有归还完全部连接，而连接池因为出作用域而析构了，
使用一个static std::atomic_bool isEnd_来表示连接池是否已经析构，在上文的自定义删除器中，
使用会先判断isEnd_的值，若为true，则无需归还连接，而是直接释放。
（因为是单例模式，连接池是一个静态实例，因此会在main函数退出后才析构，且普通实例比静态的先析构，
所以一般不会出现这种情况，只有当用户在实例化连接池前创建了一个静态连接static std::shared_ptr<Connection> conn时
才会出现这种情况）

*/
ConnectionPool::~ConnectionPool() {
	isEnd_ = true;
	std::unique_lock<std::mutex> lock(mtx_);
	while (!connectionQue_.empty()) {
		Connection* conn = connectionQue_.front();
		connectionQue_.pop();
		delete conn;
	}
}

/* 
当连接不够用时，创建新的连接
（1）如果Connection队列为空，而且还需要再获取连接，此时需要动态创建连接，上限数量是maxConnection_
（这个可以根据mysql自身的最大连接数来设置，默认是151）。
（2）这个功能点需要放在独立的线程中去做，在connectionPool的构造函数中即启动一个线程: produceMore，
通过while判断队列是否不为空和cv_.wait()来看啥时候创建新连接，这个wait()会在getConection中notify。
相当于生产者消费者模型，getConnection是消费者，produceMoreConnection是生产者。
*/
void ConnectionPool::produceMoreConnection() {
	for (;;) {
		if (isEnd_) {
			return;
		}
		std::unique_lock<std::mutex> lock(mtx_);
		while (!connectionQue_.empty()) {
			cv_.wait(lock);
		}
		if (connectionNum_ < maxConnection_) {
			Connection* conn = new Connection();
			conn->connect(ip_, port_, user_, password_, database_);
			connectionQue_.push(conn);
			conn->refreshIdleStart();
			++connectionNum_;
		}
		cv_.notify_all();
	}

}

// 根据配置文件初始化相关参数
bool ConnectionPool::loadConfigFile() {
	std::string line;
	std::ifstream file("config.ini");
	if (!file.is_open()) {
		LOG("Initial failed: can not find config.ini");
		return false;
	}
	while (std::getline(file, line)) {
		int idx = line.find('=', 0);
		if (idx == -1) {
			continue;
		}
		int idx2 = line.find('\n', idx);
		std::string key = line.substr(0, idx);
		std::string val = line.substr(idx + 1, idx2 - idx - 1);
		if (key == "ip") {
			ip_ = val;
		}
		else if (key == "port") {
			port_ = atoi(val.c_str());
		}
		else if (key == "user") {
			user_ = val;
		}
		else if (key == "password") {
			password_ = val;
		}
		else if (key == "database") {
			database_ = val;
		}
		else if (key == "initConnection") {
			initConnection_ = atoi(val.c_str());
		}
		else if (key == "maxConnection") {
			maxConnection_ = atoi(val.c_str());
		}
		else if (key == "maxIdleTime") {
			maxIdleTime_ = atoi(val.c_str());
		}
		else if (key == "connectTimeOut") {
			connectTimeOut_ = atoi(val.c_str());
		}
	}

	file.close();
	return true;

}

/*
用户获取连接
（1）用户从ConnectionPool中可以获取和MySQL的连接Connection，用户获取的连接用shared_ptr智能指针来管理，
且通过自定义删除器实现用户端的连接析构时将连接重新插入队列，而不是关闭连接。
（2）如果Connection队列为空，那么等待connectionTimeout时间如果还获取不到空闲的连接，那么获取连接失败，
此处从Connection队列获取空闲连接，使用带超时时间的mutex互斥锁来实现:
*/
std::shared_ptr<Connection> ConnectionPool::getConection() {
	if (isEnd_) {
		LOG("线程池已关闭，无法获取连接");
		return nullptr;
	}
	std::unique_lock<std::mutex> lock(mtx_);
	while (connectionQue_.empty()) {
		// 超时
		if (std::cv_status::timeout == cv_.wait_for(lock, std::chrono::milliseconds(connectTimeOut_))) {
			LOG("获取连接超时");
			return nullptr;
		}
	}

	// 自定义删除器，用户用完后将连接归还连接池，而不是释放
	std::shared_ptr<Connection> returnConn(connectionQue_.front(), [&](Connection* conn) {
		if (isEnd_) {
			std::cout << "直接释放" << std::endl;
			delete conn;
			return;
		}
		std::unique_lock<std::mutex> lock(mtx_);
		connectionQue_.push(conn);
		conn->refreshIdleStart();

	});
	connectionQue_.pop();
	if (connectionQue_.empty()) {
		cv_.notify_all();
	}
	return returnConn;
}

/*
删除空闲时间过长的额外连接
（1）队列中空闲连接时间超过maxIdleTime_的就要被释放掉，只保留初始的initSize个连接。
（2）这个功能点需要放在独立的线程中去做，在connectionPool的构造函数中即启动一个线程: scanner,
每maxIdleTime_时间就查看一下有没有空闲超时的连接，有则释放。
*/ 
void ConnectionPool::deleteExcessConnection() {
	for (;;) {
		if (isEnd_) {
			return;
		}

		std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime_));
		while (connectionNum_ > initConnection_) {
			std::unique_lock<std::mutex> lock(mtx_);
			Connection* temp = connectionQue_.front();
			if (temp->getIdleTime() >= maxIdleTime_ * 1000 && connectionNum_ > initConnection_) {
				connectionQue_.pop();
				--connectionNum_;
				delete temp;
			}
			else break;
		}
	}
}