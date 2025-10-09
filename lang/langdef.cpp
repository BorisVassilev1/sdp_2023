#include <iostream>

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <lang/grammar_factory.hpp>

int main() {

	using namespace ll1g;

	auto g = Seq(Letter('S'),
		Letter('['),
		Optional(Letter('A'),
			Seq(Letter('B'),
				Letter('1'),
				Repeat(Letter('C'),
					Word(Letter('D'),
						toLetter(",1")
					)
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
	auto pt = parser.parse(toLetter("[1,1,1,1,1]#"));
	std::cout << pt << std::endl;

	return 0;

}
