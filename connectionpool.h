#include<string>
#include<memory>
#include<queue>
#include<mutex>
#include<atomic>
#include<condition_variable>
#include"connection.h"

class ConnectionPool {
public:
	// 连接池只需要一个实例，所以ConnectionPool以单例模式进行设计
	static ConnectionPool* getConnectionPool();
	~ConnectionPool();
	// 用户获取连接
	std::shared_ptr<Connection> getConection();

private:
	// 单例模式，私有构造函数
	ConnectionPool();
	// 加载配置文件
	bool loadConfigFile();
	// 当连接不够用时，创建新的连接
	void produceMoreConnection();
	// 删除空闲过久的的额外连接
	void deleteExcessConnection();
private:
	// 连接的数据库信息
	std::string ip_;
	unsigned short port_;
	std::string user_;
	std::string password_;
	std::string database_;
	//
	int initConnection_;
	int maxConnection_;
	int maxIdleTime_;
	int connectTimeOut_;	
	// 
	static std::atomic_bool isEnd_;
	std::queue<Connection*> connectionQue_;
	std::atomic_int connectionNum_;
	std::mutex mtx_;
	std::condition_variable cv_;

};