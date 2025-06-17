#include <Regex/FST.hpp>
#include <iostream>
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

	auto r = optionalReplace("<':)','😄'>+<'=D', '🍄'>");
	auto t = parseRegex(r);

	std::cout << "Optional replace: " << t << std::endl;
	
	BENCH(makeFSA_BerriSethi<Letter>(*t), 100, "BENCH makeFSA Berry-Sethi: ");
	//auto fsa = makeFSA_BerriSethi<Letter>(*t);
	//fsa.print(std::cout);
	
	BENCH(makeFSA_Thompson<Letter>(*t), 100, "BENCH makeFSA Thompson: ");
	//auto fsa = makeFSA_Thompson<Letter>(*t);
	//fsa.print(std::cout);
	//std::cout << "FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions and "
	//		  << fsa.words.size() << " words." << std::endl;
	//fsa.print(std::cout);
	
	
}
