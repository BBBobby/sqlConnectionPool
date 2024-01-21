# sqlConnectionPool 一个简单的数据库连接池
1. ConnectionPool* ConnectionPool::getConnectionPool()
连接池只需要一个实例，所以ConnectionPool以单例模式进行设计
2. std::shared_ptr<Connection> ConnectionPool::getConection()
（1）用户从ConnectionPool中可以获取和MySQL的连接Connection，用户获取的连接用shared_ptr智能指针来管理，且通过自定义删除器实现用户端的连接析构时将连接重新插入队列，而不是关闭连接。
（2）如果Connection队列为空，那么等待connectionTimeout时间如果还获取不到空闲的连接，那么获取连接失败，此处从Connection队列获取空闲连接，使用带超时时间的mutex互斥锁来实现:
if (std::cv_status::timeout == cv_.wait_for(lock,std::chrono::milliseconds(connectTimeOut_))) {
	LOG("获取连接超时");
	return nullptr;
}
4. void ConnectionPool::produceMoreConnection()
（1）如果Connection队列为空，而且还需要再获取连接，此时需要动态创建连接，上限数量是maxConnection_（这个可以根据mysql自身的最大连接数来设置，默认是151）。
（2）这个功能点需要放在独立的线程中去做，在connectionPool的构造函数中即启动一个线程: produceMore，通过while判断队列是否不为空和cv_.wait()来看啥时候创建新连接，这个wait()会在getConection中notify。相当于生产者消费者模型，getConnection是消费者，produceMoreConnection是生产者。
5. void ConnectionPool::deleteExcessConnection()
（1）队列中空闲连接时间超过maxIdleTime_的就要被释放掉，只保留初始的initSize个连接。
（2）这个功能点需要放在独立的线程中去做，在connectionPool的构造函数中即启动一个线程: scanner,每maxIdleTime_时间就查看一下有没有空闲超时的连接，有则释放。
6. ConnectionPool::~ConnectionPool()
（1）delete连接池队列里的所有连接。
（2）为了防止用户没有归还完全部连接，而连接池因为出作用域而析构了，使用一个static std::atomic_bool isEnd_来表示连接池是否已经析构，在上文的自定义删除器中，使用会先判断isEnd_的值，若为true，则无需归还连接，而是直接释放。（因为是单例模式，连接池是一个静态实例，因此会在main函数退出后才析构，且普通实例比静态的先析构，所以一般不会出现这种情况，只有当用户在实例化连接池前创建了一个静态连接static std::shared_ptr<Connection> conn时才会出现这种情况）
7. 空闲连接Connection全部维护在一个线程安全的Connection*队列中，使用线程互斥锁保证队列的线程安全。


