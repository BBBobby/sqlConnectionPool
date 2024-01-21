#include"connection.h"

// ��ʼ�����ݿ�����
Connection::Connection()
{
	_conn = mysql_init(nullptr);
}
// �ͷ����ݿ�������Դ
Connection::~Connection()
{
	if (_conn != nullptr)
		mysql_close(_conn);
}
// �������ݿ�
bool Connection::connect(std::string ip, unsigned short port, std::string user, std::string password,
	std::string dbname)
{
	MYSQL* p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	return p != nullptr;
}
// ���²��� insert��delete��update
bool Connection::update(std::string sql)
{
	if (mysql_query(_conn, sql.c_str()))
	{
		LOG("����ʧ��:" + sql);
		return false;
	}
	return true;
}
// ��ѯ���� select
MYSQL_RES* Connection::query(std::string sql)
{
	if (mysql_query(_conn, sql.c_str()))
	{
		LOG("��ѯʧ��:" + sql);
		return nullptr;
	}
	return mysql_use_result(_conn);
}



