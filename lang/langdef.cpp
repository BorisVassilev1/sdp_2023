#include <iostream>

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <lang/grammar_factory.hpp>
#include "Regex/FST.hpp"
#include "Regex/SSFT.hpp"
#include "Regex/TFSA.hpp"
#include "Regex/functionality.hpp"

int main() {

	using namespace ll1g;

	auto g = Seq(Letter('S'), {true, false, true},
		Letter('['),
		Optional(Letter('A'),
			Seq(Letter('B'),
				Letter('1'),
				Repeat(Letter('C'),
					Production<Letter>(
						toLetter(",1"), {true, false}
					), INT_MAX
				)
			)
		),
		Letter(']')
	);

	g.printParseTable();
	std::cout << "----" << std::endl;

	auto parser = Parser<Letter>(g);

	//parser.parse(toLetter("[]#"));
	//parser.parse(toLetter("[1]#"));
	//parser.parse(toLetter("[1,1]#"));
	//parser.parse(toLetter("[1,1,1]#"));
	//auto pt = parser.parse("[1,1,1,1,1]#");
	//std::cout << pt << std::endl;

	//pt = parser.ASTparse("[1,1,1,1,1]#");
	//std::cout << pt << std::endl;
	
	const Token If = Token::createToken("IF");
	const Token For = Token::createToken("FOR");
	const Token Id = Token::createToken("ID");

	auto tokenizeIF = BS_WordFSA<Token>({'i', 'f'},{If});
	auto tokenizeFor = BS_WordFSA<Token>({'f', 'o', 'r'},{For});

	BS_FSA<Token> tokenizeId = BS_WordFSA<Token>({'a'},{});
	for ( char c = 'b'; c <= 'z'; ++c ) {
		tokenizeId = std::move(BS_UnionFSA<Token>(std::move(tokenizeId), BS_WordFSA<Token>({c},{})));
	}
	tokenizeId = BS_KleeneStarFSA<Token>(std::move(tokenizeId), false);
	tokenizeId = BS_ConcatFSA<Token>(std::move(tokenizeId), BS_WordFSA<Token>({},{Id}));

	//auto tokenizer = StupidUnionFSA<Token>(std::move(tokenizeIF), std::move(tokenizeFor));
	//tokenizer = StupidUnionFSA<Token>(std::move(tokenizer), std::move(tokenizeId));
	//drawFSA(tokenizer);

	auto realtime = realtimeFST(std::move(tokenizeId));
	drawFSA(realtime);
	realtime = pseudoDeterminizeFST(std::move(realtime));
	drawFSA(realtime);

	if(isFunctional(realtime)) {
		std::cout << "is functional" << std::endl;
	} else {
		std::cout << "is NOT functional" << std::endl;
	}

	auto ssfst = SSFT(std::move(realtime), true);
	drawFSA(ssfst);


	return 0;

}
