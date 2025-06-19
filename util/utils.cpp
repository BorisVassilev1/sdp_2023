#include <util/utils.hpp>
#include <mutex>

std::mutex &dbg::getMutex() {
	static std::mutex m;
	return m;
}

std::string getString(std::istream &os) {
	std::stringstream str;
	str << os.rdbuf();
	return str.str();
}
