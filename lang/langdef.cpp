#include <iostream>

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <lang/grammar_factory.hpp>
#include "Regex/FST.hpp"
#include "Regex/SSFT.hpp"
#include "Regex/TFSA.hpp"
#include "Regex/functionality.hpp"
#include "Regex/OutputFSA.hpp"

#include <thread>

int main() {
	using namespace ll1g;

	auto g =
		Seq(Letter('S'), {true, false, true}, Letter('['),
			Optional(Letter('A'), Seq(Letter('B'), Letter('1'),
									  Repeat(Letter('C'), Production<Letter>(toLetter(",1"), {true, false}), INT_MAX))),
			Letter(']'));

	g.printParseTable();
	std::cout << "----" << std::endl;

	auto parser = Parser<Letter>(g);

	// parser.parse(toLetter("[]#"));
	// parser.parse(toLetter("[1]#"));
	// parser.parse(toLetter("[1,1]#"));
	// parser.parse(toLetter("[1,1,1]#"));
	// auto pt = parser.parse("[1,1,1,1,1]#");
	// std::cout << pt << std::endl;

	// pt = parser.ASTparse("[1,1,1,1,1]#");
	// std::cout << pt << std::endl;

	const Token If	= Token::createToken("IF");
	const Token For = Token::createToken("FOR");
	const Token Id	= Token::createToken("ID");
	const Token WS	= Token::createToken("WS");

	auto tokenizeIF	 = BS_WordFSA<Token>({'i', 'f'}, {});
	auto tokenizeFor = BS_WordFSA<Token>({'f', 'o', 'r'}, {});

	BS_FSA<Token> tokenizeId = BS_WordFSA<Token>({'a'}, {});
	for (char c = 'b'; c <= 'z'; ++c) {
		tokenizeId = std::move(BS_UnionFSA<Token>(std::move(tokenizeId), BS_WordFSA<Token>({c}, {})));
	}
	tokenizeId = BS_KleeneStarFSA<Token>(std::move(tokenizeId), false);

	BS_FSA<Token> whitespace = BS_WordFSA<Token>({' '}, {});
	whitespace = BS_UnionFSA<Token>(std::move(whitespace), BS_WordFSA<Token>({'\n'}, {}));
	whitespace = BS_UnionFSA<Token>(std::move(whitespace), BS_WordFSA<Token>({'\t'}, {}));
	whitespace				 = BS_KleeneStarFSA<Token>(std::move(whitespace), false);

	auto TokenizeID	 = OutputFSA(realtimeFST(std::move(tokenizeId)), Id);
	auto TokenizeFor = OutputFSA(realtimeFST(std::move(tokenizeFor)), For);
	auto TokenizeIF	 = OutputFSA(realtimeFST(std::move(tokenizeIF)), If);
	auto TokenizeWS	 = OutputFSA(realtimeFST(std::move(whitespace)), WS);

	auto Tokenizer = UnionOutputFSA<Token>(std::move(TokenizeIF), std::move(TokenizeFor), std::move(TokenizeID),
										   std::move(TokenizeWS));
	drawFSA(Tokenizer);

	auto SSFTTokenizer = Tokenizer.determinizeToSSFT();
	drawFSA(SSFTTokenizer);

	auto traverser = SSFTTraverser(SSFTTokenizer);

	auto result = traverser.traverseOutputOnlyUntilCan(toLetter<Token>("if"));
	std::ranges::for_each(result, [](auto x) { std::cout << x << " "; });
	std::cout << std::endl;
	result = traverser.traverseOutputOnlyUntilCan(toLetter<Token>("for"));
	std::ranges::for_each(result, [](auto x) { std::cout << x << " "; });
	std::cout << std::endl;
	result = traverser.traverseOutputOnlyUntilCan(toLetter<Token>("abc"));
	std::ranges::for_each(result, [](auto x) { std::cout << x << " "; });
	std::cout << std::endl;

	auto data = std::vector<Token>{};
	while(std::cin.peek() != EOF) {
		char c = std::cin.get();
		data.emplace_back(c);
	}

	auto input = std::views::to_input(data) | std::views::cache_latest;

	auto it = input.begin();
	std::size_t position = 0;
	while (it != input.end()) {
		auto [result, len] = traverser.traverseOutputOnlyUntilCan(it, input.end());
		position += len;
		if(!result.has_value()) {
			std::cout << std::format("ERROR at positions {}-{}", position - len, position) << std::endl;
		} else {
			std::cout << std::format("{} at positions {}-{}", result.value(), position - len, position) << std::endl;
		}
	}


	return 0;
}
