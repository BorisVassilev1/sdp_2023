#include <Regex/FST.hpp>
#include <iostream>
#include "util/bench.hpp"
#include <DPDA/utils.h>


int main() {
	auto A = TH_WordFSA<Letter>(toLetter("abcde"), toLetter("fghij"));
	//A.print(std::cout);

	auto B = TH_WordFSA<Letter>(toLetter("fghij"), toLetter("🍄"));
	//B.print(std::cout);

	//auto C = TH_UnionFSA<Letter>(std::move(A), std::move(B));
	//C.print(std::cout);
	
	auto D = TH_ConcatFSA<Letter>(std::move(A), std::move(B));
	//D.print(std::cout);

	auto E = TH_KleeneStarFSA<Letter>(std::move(D));
	//E.print(std::cout);

	auto r = optionalReplace("<':)','😄'>+<'=D', '🍄'>");
	auto t = parseRegex(r);
	std::cout << "Optional replace: " << t << std::endl;
	
	//BENCH(makeFSA_BerriSethi<Letter>(*t), 100, "BENCH makeFSA: ");
	//auto fsa = makeFSA_BerriSethi<Letter>(*t);
	
	BENCH(makeFSA_Thompson<Letter>(*t), 100, "BENCH makeFSA: ");
	auto fsa = makeFSA_Thompson<Letter>(*t);
	std::cout << "FSA has " << fsa.N << " states and " << fsa.transitions.size() << " transitions." << std::endl;
	//fsa.print(std::cout);
	
	
}
