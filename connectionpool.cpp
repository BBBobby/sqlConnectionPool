#include"connectionpool.h"
#include<fstream>
#include<thread>
#include<functional>

// ���ӳ�ֻ��Ҫһ��ʵ��������ConnectionPool�Ե���ģʽ�������
ConnectionPool* ConnectionPool::getConnectionPool() {
	static ConnectionPool pool;
	return &pool;
}

std::atomic_bool ConnectionPool::isEnd_ = false;
ConnectionPool::ConnectionPool():connectionNum_(0) {
	// û�ɹ����þ�ֱ�ӷ���
	if (!loadConfigFile()) {
		return;
	}

	// �������Ӳ��������
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
delete���ӳض�������������ӡ�
��2��Ϊ�˷�ֹ�û�û�й黹��ȫ�����ӣ������ӳ���Ϊ��������������ˣ�
ʹ��һ��static std::atomic_bool isEnd_����ʾ���ӳ��Ƿ��Ѿ������������ĵ��Զ���ɾ�����У�
ʹ�û����ж�isEnd_��ֵ����Ϊtrue��������黹���ӣ�����ֱ���ͷš�
����Ϊ�ǵ���ģʽ�����ӳ���һ����̬ʵ������˻���main�����˳��������������ͨʵ���Ⱦ�̬����������
����һ�㲻��������������ֻ�е��û���ʵ�������ӳ�ǰ������һ����̬����static std::shared_ptr<Connection> connʱ
�Ż�������������

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
�����Ӳ�����ʱ�������µ�����
��1�����Connection����Ϊ�գ����һ���Ҫ�ٻ�ȡ���ӣ���ʱ��Ҫ��̬�������ӣ�����������maxConnection_
��������Ը���mysql�������������������ã�Ĭ����151����
��2��������ܵ���Ҫ���ڶ������߳���ȥ������connectionPool�Ĺ��캯���м�����һ���߳�: produceMore��
ͨ��while�ж϶����Ƿ�Ϊ�պ�cv_.wait()����ɶʱ�򴴽������ӣ����wait()����getConection��notify��
�൱��������������ģ�ͣ�getConnection�������ߣ�produceMoreConnection�������ߡ�
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

// ���������ļ���ʼ����ز���
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
�û���ȡ����
��1���û���ConnectionPool�п��Ի�ȡ��MySQL������Connection���û���ȡ��������shared_ptr����ָ��������
��ͨ���Զ���ɾ����ʵ���û��˵���������ʱ���������²�����У������ǹر����ӡ�
��2�����Connection����Ϊ�գ���ô�ȴ�connectionTimeoutʱ���������ȡ�������е����ӣ���ô��ȡ����ʧ�ܣ�
�˴���Connection���л�ȡ�������ӣ�ʹ�ô���ʱʱ���mutex��������ʵ��:
*/
std::shared_ptr<Connection> ConnectionPool::getConection() {
	if (isEnd_) {
		LOG("�̳߳��ѹرգ��޷���ȡ����");
		return nullptr;
	}
	std::unique_lock<std::mutex> lock(mtx_);
	while (connectionQue_.empty()) {
		// ��ʱ
		if (std::cv_status::timeout == cv_.wait_for(lock, std::chrono::milliseconds(connectTimeOut_))) {
			LOG("��ȡ���ӳ�ʱ");
			return nullptr;
		}
	}

	// �Զ���ɾ�������û���������ӹ黹���ӳأ��������ͷ�
	std::shared_ptr<Connection> returnConn(connectionQue_.front(), [&](Connection* conn) {
		if (isEnd_) {
			std::cout << "ֱ���ͷ�" << std::endl;
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
ɾ������ʱ������Ķ�������
��1�������п�������ʱ�䳬��maxIdleTime_�ľ�Ҫ���ͷŵ���ֻ������ʼ��initSize�����ӡ�
��2��������ܵ���Ҫ���ڶ������߳���ȥ������connectionPool�Ĺ��캯���м�����һ���߳�: scanner,
ÿmaxIdleTime_ʱ��Ͳ鿴һ����û�п��г�ʱ�����ӣ������ͷš�
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