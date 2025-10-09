#include <Regex/FST.hpp>
#include <iostream>
#include "Regex/TFSA.hpp"
#include "Regex/ambiguity.hpp"
#include "Regex/functionality.hpp"
#include "Regex/regexParser.hpp"
#include "util/bench.hpp"
#include <DPDA/utils.h>
#include "Regex/SSFT.hpp"

void test_determinization() {
	TFSA<Letter> fsa;
	fsa.N		= 8;
	fsa.qFirsts = {0, 1};
	fsa.qFinals = {3, 6, 7};
	fsa.addTransition(0, 'a', toLetter("c"), 2);
	fsa.addTransition(0, 'a', toLetter("cc"), 3);
	fsa.addTransition(1, 'a', toLetter("cc"), 3);
	fsa.addTransition(1, 'a', toLetter("ccc"), 4);
	fsa.addTransition(2, 'b', toLetter("ccd"), 5);
	fsa.addTransition(3, 'b', toLetter("cd"), 5);
	fsa.addTransition(4, 'b', toLetter("dd"), 6);
	fsa.addTransition(5, 'a', toLetter("d"), 7);
	fsa.addTransition(6, 'a', toLetter(""), 7);

	drawFSA(fsa);

	bool functional = isFunctional(fsa);
	std::cout << "functional: " << functional << std::endl;
	if (!functional) {
		std::cout << "FSA is not functional." << std::endl;
		return;
	} else {
		std::cout << "FSA is functional." << std::endl;
	}

	SSFT<Letter> ssft(std::move(fsa));
	std::cout << "SSFT has " << ssft.N << " states and " << ssft.transitions.size() << " transitions and "
			  << ssft.words.size() << " words." << std::endl;

	std::cout << "draw SSFT" << std::endl;
	drawFSA(ssft);
}

void test_bounded_variation() {
	TFSA<Letter> fsa;
	fsa.N		= 4;
	fsa.qFirsts = {0};
	fsa.qFinals = {0, 1, 3};
	fsa.addTransition(0, 'a', toLetter("a"), 1);
	fsa.addTransition(1, 'a', toLetter("a"), 1);
	fsa.addTransition(0, 'a', toLetter(""), 2);
	fsa.addTransition(2, 'a', toLetter(""), 2);
	fsa.addTransition(2, 'b', toLetter("b"), 3);

	drawFSA(fsa);

	bool functional = isFunctional(fsa);
	if (!functional) {
		std::cout << "FSA is not functional." << std::endl;
		return;
	} else {
		std::cout << "FSA is functional." << std::endl;
	}

	try {
		SSFT<Letter> ssft(std::move(fsa));
		std::cout << "SSFT has " << ssft.N << " states and " << ssft.transitions.size() << " transitions and "
				  << ssft.words.size() << " words." << std::endl;

		std::cout << "draw SSFT" << std::endl;
		drawFSA(ssft);
	} catch (const std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl;
		return;
	}
}

void test_replace() {
	auto r = optionalReplace("<':)','😄'>+<'=D', '🍄'>", "abcd");
	auto t = parseRegex(r);

	std::cout << "Optional replace: " << t << std::endl;

	BENCH(makeFSA_BerriSethi<Letter>(*t), 100, "BENCH makeFSA Berry-Sethi: ");
	FST<Letter> fst = makeFSA_BerriSethi<Letter>(*t);
	// fst.print(std::cout);

	BENCH(makeFSA_Thompson<Letter>(*t), 100, "BENCH makeFSA Thompson: ");
	// auto fst = makeFSA_Thompson<Letter>(*t);
	// fsa.print(std::cout);
	std::cout << "FSA has " << fst.N << " states and " << fst.transitions.size() << " transitions and "
			  << fst.words.size() << " words." << std::endl;
	// fsa.print(std::cout);
	//drawFSA(fst);

	fst = removeEpsilonFST<Letter>(std::move(fst));
	fst = trimFSA<Letter>(std::move(fst));

	if (!testInfiniteAmbiguity(fst)) {
		std::cout << "FSA is not infinitely ambiguous." << std::endl;
	} else {
		std::cout << "FSA is infinitely ambiguous." << std::endl;
		return;
	}

	auto fsa = expandFST(std::move(fst));
	//drawFSA(fsa);
	fsa = removeUpperEpsilonFST(std::move(fsa));
	//drawFSA(fsa);
	fsa = trimFSA(std::move(fsa));
	std::cout << "FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
			  << fsa.words.size() << " words as REALTIME." << std::endl;

	bool functional = isFunctional(fsa);
	std::cout << "functional: " << functional << std::endl;
	if (!functional) {
		std::cout << "FSA is not functional." << std::endl;
		return;
	} else {
		std::cout << "FSA is functional." << std::endl;
	}

	SSFT<Letter> ssft(std::move(fsa));
	std::cout << "SSFT has " << ssft.N << " states and " << ssft.transitions.size() << " transitions and "
			  << ssft.words.size() << " words." << std::endl;

	std::cout << "draw SSFT" << std::endl;
	//drawFSA(ssft);

	auto input		 = toLetter("abbababb");
	auto [output, b] = ssft.f(input);
	std::cout << "Input: " << input << std::endl;
	std::cout << "Output: " << output << std::endl;

	input				 = toLetter("ab:)ab:)aaa:):)a=D=Dbab");
	std::tie(output, b) = ssft.f(input);
	std::cout << "Input: " << input << std::endl;
	std::cout << "Output: " << output << std::endl;
}

int main() {
	// test_determinization();
	// test_bounded_variation();
	test_replace();

	return 0;
}
