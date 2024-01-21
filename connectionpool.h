#include<string>
#include<memory>
#include<queue>
#include<mutex>
#include<atomic>
#include<condition_variable>
#include"connection.h"

class ConnectionPool {
public:
	// ���ӳ�ֻ��Ҫһ��ʵ��������ConnectionPool�Ե���ģʽ�������
	static ConnectionPool* getConnectionPool();
	~ConnectionPool();
	// �û���ȡ����
	std::shared_ptr<Connection> getConection();

private:
	// ����ģʽ��˽�й��캯��
	ConnectionPool();
	// ���������ļ�
	bool loadConfigFile();
	// �����Ӳ�����ʱ�������µ�����
	void produceMoreConnection();
	// ɾ�����й��õĵĶ�������
	void deleteExcessConnection();
private:
	// ���ӵ����ݿ���Ϣ
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