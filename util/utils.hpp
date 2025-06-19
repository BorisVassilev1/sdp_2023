#pragma once

#include <memory>
#include <type_traits>
#include <cxxabi.h>
#include <sstream>

#define JOB(name, ...)                     \
	static int _job_##name = []() -> int { \
		__VA_ARGS__;                       \
		return 0;                          \
	}();

#define STR(x) #x

template <auto N>
struct string_literal {
	constexpr string_literal(const char (&str)[N]) { std::ranges::copy_n(str, N, value); }

	char value[N];

	constexpr operator const char *() const { return value; }
};

std::string getString(std::istream &os);

template <class T>
auto type_name() {
	typedef typename std::remove_reference<T>::type TR;

	std::unique_ptr<char, void (*)(void *)> own(abi::__cxa_demangle(typeid(TR).name(), nullptr, nullptr, nullptr),
												std::free);

	std::string r = own != nullptr ? own.get() : typeid(TR).name();
	if (std::is_const<TR>::value) r += " const";
	if (std::is_volatile<TR>::value) r += " volatile";
	if (std::is_lvalue_reference<T>::value) r += "&";
	else if (std::is_rvalue_reference<T>::value) r += "&&";
	return r;
}

inline auto typename_demangle(const char *n) {
	std::unique_ptr<char, void (*)(void *)> own(abi::__cxa_demangle(n, nullptr, nullptr, nullptr), std::free);
	std::string								r = own != nullptr ? own.get() : n;
	return r;
}

template<class T>
inline auto  type_name(T *v) {
	return typename_demangle(typeid(v).name());
}

/**
 * @brief SpinLock, copy pasta from lectures
 */
struct SpinLock {
	std::atomic_flag flag;
	void			 lock() {
		while (flag.test_and_set())
			;
	}

	void unlock() { flag.clear(); }

	bool tryLock() { return !flag.test_and_set(); }
};

namespace dbg {
/**
 *
 * @brief Log levels
 */
enum {
	LOG_INFO	= (0),
	LOG_DEBUG	= (1),
	LOG_WARNING = (2),
	LOG_ERROR	= (3),
};

#define COLOR_RESET	 "\033[0m"
#define COLOR_RED	 "\x1B[0;91m"
#define COLOR_GREEN	 "\x1B[0;92m"
#define COLOR_YELLOW "\x1B[0;93m"

static const char *log_colors[]{COLOR_RESET, COLOR_GREEN, COLOR_YELLOW, COLOR_RED};

std::mutex &getMutex();

/**
 * @brief prints to std::cerr
 *
 * @return 1
 */
template <class... Types>
bool inline f_dbLog(std::ostream &out, Types... args) {
	std::lock_guard lock(dbg::getMutex());
	(out << ... << args) << std::endl;
	return 1;
}

/**
 * @def dbLog(severity, ...)
 * If severity is greater than the definition DBG_LOG_LEVEL, prints all arguments to std::cerr
 */

#ifndef NDEBUG
	#define DBG_DEBUG
	#define DBG_LOG_LEVEL -1
	#define dbLog(severity, ...)                                                                                   \
		severity >= DBG_LOG_LEVEL                                                                                  \
			? (dbg::f_dbLog(std::cerr, dbg::log_colors[severity], "[", #severity, "] ", __VA_ARGS__, COLOR_RESET)) \
			: 0;
#else
	#define DBG_LOG_LEVEL		 3
	#define dbLog(severity, ...) ((void)0)
#endif

}
