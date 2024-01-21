#ifndef PUBLIC_
#define PUBLIC_

#define LOG(str) \
	std::cout << __FILE__ << ": " << __LINE__ << " " << \
	__TIMESTAMP__ << ": " << str << std::endl;

#endif