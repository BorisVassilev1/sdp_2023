#include "utils.h"
#include "DPDA/dpda.h"

std::ostream &operator<<(std::ostream &out, Letter l) {
	if (l == Letter::eps) {
		out << "ε";
	} else out << char(l);
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
