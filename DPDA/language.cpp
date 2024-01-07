#include <DPDA/parser.h>
#include <DPDA/cfg.h>
#include <DPDA/utils.h>
#include <exception>

std::unordered_map<std::size_t, std::string> SymbolNames;

struct Symbol {
	std::size_t value;

	Symbol(std::size_t value) : value(value) {}

   public:
	Symbol(char value) : value(value) {}

	explicit constexpr operator std::size_t() const { return value; }
	explicit constexpr operator char() const { return value; }
	explicit constexpr operator bool() const { return value; }

	static std::size_t	size;
	static const Symbol eps;
	static const Symbol eof;

	static Symbol createSymbol(const std::string &name, std::size_t value = ++size) {
		SymbolNames[value] = name;
		return Symbol(value);
	}

	bool operator==(const Symbol &other) const { return value == other.value; }
};

std::ostream &operator<<(std::ostream &out, const Symbol &v) { return out << SymbolNames.find(v.value)->second; }

namespace std {
template <>
struct hash<Symbol> {
	size_t operator()(const Symbol &x) const { return hash<std::size_t>()(x.value); }
};
}	  // namespace std

template <>
struct std::formatter<Symbol> : ostream_formatter {};

std::size_t	 Symbol::size = 256;
const Symbol Symbol::eps  = Symbol::createSymbol("ε", 0);
const Symbol Symbol::eof  = Symbol::createSymbol("eof");

const Symbol Program	 = Symbol::createSymbol("Program");
const Symbol Number		 = Symbol::createSymbol("Number");
const Symbol Number_	 = Symbol::createSymbol("Number'");
const Symbol Identifier	 = Symbol::createSymbol("Identifier");
const Symbol Identifier_ = Symbol::createSymbol("Identifier'");

CFG<Symbol> g(Program, Symbol::eof);

void createCFG() {
	for (int i = 1; i < 128; ++i) {
		g.terminals.insert(Symbol::createSymbol(std::string(1, i), i));
	}
	g.terminals.insert(Symbol::eof);
	g.nonTerminals = {Program, Number, Number_, Identifier, Identifier_};

	g.addRule(Number, {'0'});
	for (int i = 0; i < 10; ++i) {
		if(i != 0)
			g.addRule(Number, {char('0' + i), Number_});
		g.addRule(Number_, {char('0' + i), Number_});
	}
	g.addRule(Number_, {});
	
	for(int i = 'A'; i <= 'Z'; ++ i) {
		g.addRule(Identifier, {char(i), Identifier_});
		g.addRule(Identifier_, {char(i), Identifier_});
	}
	for(int i = 'a'; i <= 'z'; ++ i) {
		g.addRule(Identifier, {char(i), Identifier_});
		g.addRule(Identifier_, {char(i), Identifier_});
	}
	g.addRule(Identifier_, {});

	g.addRule(Program, {Number, ' ', Program});
	g.addRule(Program, {Identifier, ' ', Program});
	g.addRule(Program, {});
}

int main() {
	createCFG();

	g.printParseTable();

	try {
		Parser<State, Symbol> parser(g);

		auto t = parser.parse("bobo 123 kalin pesho 234 ");
		std::cout << t << std::endl;
		deleteParseTree(t);

	} catch (const std::exception &e) { std::cerr << e << std::endl; }
}
