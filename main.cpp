#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include"connection.h"
#include"connectionpool.h"
using std::cout;
using std::endl;


int main() {

	//clock_t start = clock();
	//for (int i = 0; i < 1000; i++) {
	//	Connection conn;
	//	bool flag = conn.connect("127.0.0.1", 3306, "root", "04260015AA", "chat");
	//	std::string sql = "INSERT INTO user (name, age, sex) VALUES ('bobby', 28, 'male')";
	//	conn.update(sql);
	//}
	//cout << (clock() - start) << endl;

	//clock_t start = clock();
	//ConnectionPool* cp = ConnectionPool::getConnectionPool();
	//for (int i = 0; i < 1000; i++) {
	//	std::shared_ptr<Connection> conn = cp->getConection();
	//	std::string sql = "INSERT INTO user (name, age, sex) VALUES ('bobby', 28, 'male')";
	//	if (conn != nullptr) {
	//		conn->update(sql);
	//	}
	//}
	//cout << (clock() - start) << endl;


	

	return 0;
}