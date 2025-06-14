#include <cstring>
#include <fstream>
#include <iostream>
#include "Regex/FST.hpp"
#include "Regex/regexParser.hpp"
#include "util/bench.hpp"

#include <DPDA/cfg.h>
#include <DPDA/parser.h>
#include <DPDA/token.h>

void generate1M() {
	srand(std::chrono::system_clock::now().time_since_epoch().count());
	std::ofstream out("regex_1M.txt");
	for (int i = 0; i < 1000; ++i) {
		out << generateRegexString(1000) << std::endl;
		if (i != 999) {
			out << " + ";
		} else {
			out << std::endl;
		}
	}
}

int main(int argc, char **argv) {
	RegexParser parser;

	std::stringstream buffer;
	std::string		  fileName = "test_file_regex.txt";
	if (argc == 2) fileName = argv[1];

	std::ifstream file(fileName);
	if (!file) {
		std::cout << "error: " << strerror(errno) << std::endl;
		return 1;
	}
	std::cout << "parsing file: " << fileName << std::endl;
	buffer << file.rdbuf();

	std::string text = buffer.str();
	// std::cout << text << std::endl;

	BENCH(tokenize(text), 100, "BENCH tokenize : ");
	auto tokens = tokenize(text);
	if (tokens.size() < 1000) { std::cout << "tokens: " << tokens << std::endl; }

	// BENCH({ parser.parse(tokens); }, 100, "BENCH parsing regex: ");
	// auto parseTree = parser.parse(tokens);
	// if (tokens.size() < 1000) { std::cout << "parse tree: " << std::endl << parseTree << std::endl; }

	// auto reg = parseTreeToRegex(parseTree.get());
	BENCH({ parseRegex(text); }, 100, "BENCH parsing regex: ");
	auto reg = parseRegex(text);
	if (tokens.size() < 1000) {
		std::cout << "regex: " << std::endl;
		reg->print(std::cout);
		std::cout << std::endl;
	}
	// generate1M();
	//

	// auto r = optionalReplace("<':)','😄'>+<'=D', '🍄'>");
	// auto t = parseRegex(r);
	// std::cout << "Optional replace: " << t << std::endl;

	TFSA<Letter> fsa;
	BENCH(fsa = makeFSA_BerriSethi<Letter>(*reg), 1, "BENCH makeFSA: ");
	std::cout << "FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
			  << fsa.words.size() << " words." << std::endl;
	BENCH(fsa = makeFSA_Thompson<Letter>(*reg), 1, "BENCH makeFSA Thompson: ");
	std::cout << "FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
			  << fsa.words.size() << " words." << std::endl;
	// fsa.print(std::cout);
}
