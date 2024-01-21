#ifndef CONNECTION_
#define CONNECTION_
#include <mysql.h>
#include <string>
#include <iostream>
#include <ctime>
#include "public.h"

// 数据库操作类
class Connection
{
public:
	// 初始化数据库连接
	Connection();
	// 释放数据库连接资源
	~Connection();
	// 连接数据库
	bool connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname);
	// 更新操作 insert、delete、update
	bool update(std::string sql);
	// 查询操作 select
	MYSQL_RES* query(std::string sql);
	// 更新空闲起始时间
	void refreshIdleStart() { idleStart = clock(); }
	// 获得空闲时间
	clock_t getIdleTime() { return clock() - idleStart; }
private:
	MYSQL* _conn; // 表示和MySQL Server的一条连接
	clock_t idleStart;	// 空闲起始时刻
};
#endif