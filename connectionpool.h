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
	int initConnection_;	// 初始连接量
	int maxConnection_;	// 最大连接量
	int maxIdleTime_;	// 额外连接的最大空闲时间
	int connectTimeOut_;	// 用户获取连接时的超时时间
	// 
	static std::atomic_bool isEnd_;		// 标志连接池是否关闭
	std::queue<Connection*> connectionQue_; // 连接都存储在队列里
	std::atomic_int connectionNum_;		// 标志总共创建了多少个连接
	std::mutex mtx_;			
	std::condition_variable cv_;

};
