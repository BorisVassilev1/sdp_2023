#pragma once

#include <chrono>
#include <functional>

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

inline std::string gen_random_string(const int min, const int max) {
	if (min > max) { throw std::invalid_argument("min must be less than or equal to max"); }
	int len = min + rand() % (max - min + 1);
	return gen_random_string(len);
}

class SlowDown {
	std::chrono::high_resolution_clock::duration delay = std::chrono::milliseconds(100);

   public:
	std::chrono::high_resolution_clock::time_point last;

   public:
	SlowDown(std::chrono::high_resolution_clock::duration delay = std::chrono::milliseconds(100))
		: delay(delay), last() {}

	void do_thing(const std::function<void(void)> &f) {
		auto now = std::chrono::high_resolution_clock::now();
		if (now - last > delay) {
			f();
			last = now;
		}
	}
};

class SlowDown2 {
	time_t delay;

   public:
	time_t last;

   public:
	SlowDown2(time_t delay) : delay(delay), last() {}

	void do_thing(const std::function<void(void)> &f) {
		auto now = time(0);
		if (now - last > delay) [[unlikely]] {
			f();
			last = now;
		}
	}
};

class SlowDown3 {
	uint64_t delay;

   public:
	uint64_t last;

   public:
	SlowDown3(auto delay = std::chrono::milliseconds(100)) : delay(delay.count() * 4000000), last(0) {}

	void do_thing(const std::function<void(void)> &f) {
		auto now = __builtin_ia32_rdtsc();
		if (now - last > delay) {
			f();
			last = now;
		}
	}
};
