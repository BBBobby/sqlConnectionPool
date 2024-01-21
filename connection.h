#ifndef CONNECTION_
#define CONNECTION_
#include <mysql.h>
#include <string>
#include <iostream>
#include <ctime>
#include "public.h"

// ���ݿ������
class Connection
{
public:
	// ��ʼ�����ݿ�����
	Connection();
	// �ͷ����ݿ�������Դ
	~Connection();
	// �������ݿ�
	bool connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname);
	// ���²��� insert��delete��update
	bool update(std::string sql);
	// ��ѯ���� select
	MYSQL_RES* query(std::string sql);
	// ���¿�����ʼʱ��
	void refreshIdleStart() { idleStart = clock(); }
	// ��ÿ���ʱ��
	clock_t getIdleTime() { return clock() - idleStart; }
private:
	MYSQL* _conn; // ��ʾ��MySQL Server��һ������
	clock_t idleStart;	// ������ʼʱ��
};
#endif