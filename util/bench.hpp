#pragma once

#include <chrono>

#define BENCH(x, n, s)                                                                                    \
	{                                                                                                     \
		auto t1 = std::chrono::high_resolution_clock::now();                                              \
		for (int i = 0; i < n; ++i)                                                                       \
			x;                                                                                            \
		auto t2	  = std::chrono::high_resolution_clock::now();                                            \
		auto diff = t2 - t1;                                                                              \
		std::cout << s << std::chrono::duration_cast<std::chrono::microseconds>(diff / n).count() << "μs" \
				  << std::endl;                                                                           \
	}

inline std::string gen_random_string(const int len) {
	static const char alphanum[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";
	std::string tmp_s;
	tmp_s.reserve(len);

	if (len > 0) { tmp_s += alphanum[rand() % ((sizeof(alphanum) / 2) - 1)]; }
	for (int i = 1; i < len; ++i) {
		tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	return tmp_s;
}
