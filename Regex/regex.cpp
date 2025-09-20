#include <cstring>
#include <fstream>
#include <iostream>
#include "Regex/FST.hpp"
#include "Regex/SSFT.hpp"
#include "Regex/TFSA.hpp"
#include "Regex/ambiguity.hpp"
#include "Regex/functionality.hpp"
#include "Regex/regexParser.hpp"
#include "util/bench.hpp"
#include "util/pipes.hpp"

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

	if (argc == 2 && std::string(argv[1]) == "--generate") {
		generate1M();
		std::cout << "Generated regex_1M.txt" << std::endl;
		return 0;
	}

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

	BENCH(tokenize(text), 100, "BENCH tokenize : ");
	auto tokens = tokenize(text);
	if (tokens.size() < 1000) { std::cout << "tokens: " << tokens << std::endl; }

	BENCH({ parseRegex(text); }, 10, "BENCH parsing regex: ");
	auto reg = parseRegex(text);
	std::cout << reg->size() << " tokens in regex." << std::endl;
	if (tokens.size() < 1000) {
		std::cout << "regex: " << std::endl;
		reg->print(std::cout);
		std::cout << std::endl;
	}
	// generate1M();
	//

	FST<Letter> fst;
	//BENCH(fst = makeFSA_BerriSethi<Letter>(*reg), 1, "BENCH makeFSA: ");
	BENCH(fst = makeFSA_BerriSethi<Letter>(*reg), 1, "BENCH makeFSA Thompson: ");
	std::cout << "FSA has " << fst.N << " states and " << fst.transitions.size() << " transitions and "
			  << fst.words.size() << " words." << std::endl;

	if (tokens.size() < 1000) { drawFSA(fst); }

	bool infAmbiguity;
	BENCH(infAmbiguity = testInfiniteAmbiguity(fst), 100, "BENCH testInfiniteAmbiguity: ");
	std::cout << "Testing infinite ambiguity: " << testInfiniteAmbiguity(fst) << std::endl;

	BENCH(fst = trimFSA<Letter>(std::move(fst));, 1, "BENCH trimFSA: ");
	BENCH(fst = removeEpsilonFST<Letter>(std::move(fst));, 1, "BENCH removeEpsilonFST: ");
	if (tokens.size() < 1000) drawFSA(fst);
	std::cout << "FSA has " << fst.N << " states and " << fst.transitions.size() << " transitions and "
			  << fst.words.size() << " words after removing epsilons." << std::endl;

	BENCH(fst = trimFSA<Letter>(std::move(fst));, 1, "BENCH trimFSA again: ");
	if (tokens.size() < 1000) drawFSA(fst);
	std::cout << "FSA has " << fst.N << " states and " << fst.transitions.size() << " transitions and "
			  << fst.words.size() << " words after trimming." << std::endl;

	TFSA<Letter> fsa;
	BENCH(fsa = expandFST<Letter>(std::move(fst));, 1, "BENCH expandFST: ");
	if (tokens.size() < 1000) drawFSA(fsa);
	std::cout << "Expanded FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
			  << fsa.words.size() << " words." << std::endl;

	if (!infAmbiguity) {
		BENCH(fsa = removeUpperEpsilonFST<Letter>(std::move(fsa));, 1, "BENCH realtimeFST: ");
		if (tokens.size() < 1000) drawFSA(fsa);
		std::cout << "Real-time FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
				  << fsa.words.size() << " words." << std::endl;

		BENCH(fsa = trimFSA<Letter>(std::move(fsa));, 1, "BENCH trimFSA again: ");
		if (tokens.size() < 1000) drawFSA(fsa);
		std::cout << "Trimmed FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
				  << fsa.words.size() << " words." << std::endl;
		bool isFunc = isFunctional(fsa);
		std::cout << "isFunctional: " << isFunc << std::endl;

		if(!isFunc) {
			std::cout << "The FSA is not functional!" << std::endl;
			return 1;
		}
		try {
			std::cout << "converting to SSFT..." << std::endl;
			auto ssfst = SSFT<Letter>(std::move(fsa));
			drawFSA(ssfst);

			std::cout << "SSFT has " << ssfst.N << " states and " << ssfst.transitions.size() << " transitions."
					  << std::endl;

			std::string input;
			std::cin >> input;
			auto [result, b] = ssfst.f(toLetter(input));
			if (b) {
				std::cout << "Input accepted: " << result << std::endl;
			} else {
				std::cout << "Input rejected." << std::endl;
			}
		} catch (const std::exception &e) {
			std::cerr << "Error: " << e.what() << std::endl;
		}
		
	}

}
