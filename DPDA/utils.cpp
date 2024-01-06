#include "hashes.h"

std::ostream &operator<<(std::ostream &out, Letter l) {
	if (l == Letter::eps) {
		out << "ε";
	} else out << char(l);
	return out;
}

std::ostream &operator<<(std::ostream &out, State s) {
	if (s >= Letter::size) {
		out << "f" << Letter(s - Letter::size);
	} else out << size_t(s);
	return out;
}

using bits = std::vector<bool>;

static void print_exception(std::ostream &out, const std::exception& e, int level =  0)
{
    std::cerr << std::string(level, ' ') << "exception: " << e.what() << '\n';
    try
    {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nestedException)
    {
        print_exception(out, nestedException, level + 1);
    }
    catch (...) {}
}

std::ostream &operator<<(std::ostream &out, const std::exception &e) {
	print_exception(out, e);
	return out;
}
