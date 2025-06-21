#include <Regex/FST.hpp>
#include <iostream>
#include "Regex/TFSA.hpp"
#include "Regex/functionality.hpp"
#include "Regex/regexParser.hpp"
#include "util/bench.hpp"
#include <DPDA/utils.h>


int main() {
	auto A = BS_WordFSA<Letter>(toLetter("abcde"), toLetter("fghij"));
	//A.print(std::cout);

	auto B = BS_WordFSA<Letter>(toLetter("fghij"), toLetter("🍄"));
	//B.print(std::cout);

	//auto C = BS_UnionFSA<Letter>(std::move(A), std::move(B));
	//C.print(std::cout);
	
	auto D = BS_ConcatFSA<Letter>(std::move(A), std::move(B));
	//D.print(std::cout);

	auto E = BS_KleeneStarFSA<Letter>(std::move(D));
	//E.print(std::cout);

	auto r = optionalReplace("<':)','😄'>+<'=D', '🍄'>", "ab");
	auto t = parseRegex(r);

	std::cout << "Optional replace: " << t << std::endl;
	
	BENCH(makeFSA_BerriSethi<Letter>(*t), 100, "BENCH makeFSA Berry-Sethi: ");
	auto fst = makeFSA_BerriSethi<Letter>(*t);
	//fst.print(std::cout);
	
	BENCH(makeFSA_Thompson<Letter>(*t), 100, "BENCH makeFSA Thompson: ");
	//auto fst = makeFSA_Thompson<Letter>(*t);
	//fsa.print(std::cout);
	std::cout << "FSA has " << fst.N << " states and " << fst.transitions.size() << " transitions and "
			  << fst.words.size() << " words." << std::endl;
	//fsa.print(std::cout);
	drawFSA(fst);
	
	fst = removeEpsilonFST<Letter>(std::move(fst));
	fst = trimFSA<Letter>(std::move(fst));

	auto fsa = expandFST(std::move(fst));
	drawFSA(fsa);

	//std::cout << "functional: " << isFunctional(fsa) << std::endl;
	
	
	
}
