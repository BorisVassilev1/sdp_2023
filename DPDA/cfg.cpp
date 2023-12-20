#include "cfg.h"
#include <assert.h>

int main() {
	CFG<Letter> g;
	g.terminals	   = {'i', '(', ')', '.', '+', '#'};
	g.nonTerminals = {'e', 'E', 't', 'T', 'f'};
	g.addRule('e', "tE");
	g.addRule('E', "");
	g.addRule('E', "+tE");
	g.addRule('t', "fT");
	g.addRule('T', "");
	g.addRule('T', ".fT");
	g.addRule('f', "(e)");
	g.addRule('f', "i");
	g.start = 'e';
	g.eof	= '#';

	auto nullable = g.findNullables();

	auto first = g.findFirsts(nullable);

	assert(g.isFirst('(', {'T', 'e'}, nullable, first) == true);
	std::cout << g.first({'T', 'e'}, nullable, first) << std::endl;

	auto follow = g.findFollows(nullable, first);

	g.printParseTable(nullable, first, follow);
}
