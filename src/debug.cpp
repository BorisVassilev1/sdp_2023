#include <debug.hpp>

std::mutex &dbg::getMutex() {
	static std::mutex m;
	return m;
}
