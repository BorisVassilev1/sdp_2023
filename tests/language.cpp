#include "OutputFSA.hpp"
#include <cassert>
#include <cctype>
#include <cstring>
#include <exception>
#include <fstream>
#include <memory>
#include <ostream>
#include <string_view>

#include <parser.h>
#include <cfg.h>
#include <utils.h>
#include <token.h>
#include <earley.hpp>
#include <grammar_factory.hpp>

using namespace fl;

const Token Program				 = Token::createToken("Program");
const Token Number				 = Token::createToken("Number");
const Token Identifier			 = Token::createToken("Identifier");
const Token NL					 = Token::createToken("NL", '\n');
const Token Expression			 = Token::createToken("Expression");
const Token Assignment			 = Token::createToken("Assignment");
const Token Arithmetic			 = Token::createToken("Arithmetic");
const Token Term				 = Token::createToken("Term");
const Token Factor				 = Token::createToken("Factor");
const Token Conditional			 = Token::createToken("Conditional");
const Token Comparison			 = Token::createToken("Comparison");
const Token CommaSep			 = Token::createToken("CommaSep");
const Token ParamList			 = Token::createToken("ParamList");
const Token Scope				 = Token::createToken("Scope");
const Token ParenthesisExpr		 = Token::createToken("(Expr)");
const Token ParenthesisParamList = Token::createToken("(ParamList)");
const Token BracketParamList	 = Token::createToken("[ParamList]");

const Token If	  = Token::createToken("if");
const Token While = Token::createToken("while");
const Token For	  = Token::createToken("for");

CFG<Token> createCFGNew() {
	using namespace ll1g;

	Token current = Token::eps;
	auto  NT	  = [&]() {
		  assert(current != Token::eps && "Cannot create dependent token from eps");
		  return current = Token::createDependentToken(current);
	};

	current		 = Program;
	auto program = Repeat(Program, Choice(NT(), Expression, Conditional, Scope), INT_MAX);

	current			= Expression;
	auto expression = Seq(Expression, {false, true}, Optional(NT(), Assignment), Token(';'));
	expression.setIgnoreEmpty(false);
	// expression.setIgnoreSingleChild(false);

	current			= Assignment;
	auto assignment = Seq(Assignment, Comparison,
						  RepeatChoice(NT(), Production({Token('='), Comparison}, {true, false}, Token('='))));

	current			= Comparison;
	auto comparison = Seq(Comparison, Arithmetic,
						  RepeatChoice(NT(), Production({Token('<'), Arithmetic}, {true, false}, Token('<')),
									   Production({Token('>'), Arithmetic}, {true, false}, Token('>'))));

	current			= Arithmetic;
	auto arithmetic = Seq(Arithmetic, Term,
						  RepeatChoice(NT(), Production({Token('+'), Term}, {true, false}, Token('+')),
									   Production({Token('-'), Term}, {true, false}, Token('-'))));

	current	  = Term;
	auto term = Seq(Term, Factor,
					RepeatChoice(NT(), Production({Token('*'), Factor}, {true, false}, Token('*')),
								 Production({Token('/'), Factor}, {true, false}, Token('/'))));

	current				 = ParenthesisExpr;
	auto parenthesisExpr = Word(ParenthesisExpr, {Token('('), Assignment, Token(')')}, {true, false, true});
	parenthesisExpr.setIgnoreEmpty(false);
	parenthesisExpr.setIgnoreSingleChild(false);

	current		= Factor;
	auto factor = Choice(Factor, std::vector{Identifier, ParamList}, std::vector{Number},
						 std::move(parenthesisExpr)		// move the entire grammar
	);

	current	   = Scope;
	auto scope = Word(Scope, {'{', Program, '}'}, {true, false, true});
	scope.setIgnoreEmpty(false);
	scope.setIgnoreSingleChild(false);

	current = Conditional;
	auto conditional =
		Choice(Conditional, Production({If, '(', Assignment, ')', Scope}, {true, true, false, true, false}, If),
			   Production({While, '(', Assignment, ')', Scope}, {true, true, false, true, false}, While),
			   Production({For, '(', Expression, Expression, Assignment, ')', Scope},
						  {true, true, false, false, false, true, false}, For));
	conditional.setIgnoreEmpty(false);

	current = ParamList;
	auto paramList =
		RepeatChoice(NT(), Word(ParenthesisParamList, {'(', CommaSep, ')'}, {true, false, true}),
										 Word(BracketParamList, {'[', CommaSep, ']'}, {true, false, true}));
	paramList.getNonTerminalData(ParenthesisParamList).ignoreEmpty = false;	
	paramList.getNonTerminalData(ParenthesisParamList).ignoreSingleChild = false;
	paramList.getNonTerminalData(BracketParamList).ignoreEmpty = false;
	paramList.getNonTerminalData(BracketParamList).ignoreSingleChild = false;
	paramList.setUpwardSpillThreshold(INT_MAX);
	
	auto optionalParamList = Optional(ParamList, std::move(paramList));

	current = CommaSep;
	auto commaSep =
		Seq(NT(), Assignment, Repeat(NT(), Word(NT(), {',', Assignment}, {true, false}, true), INT_MAX));
	commaSep.setUpwardSpillThreshold(INT_MAX);
	
	auto optionalCommaSep = Optional(CommaSep, std::move(commaSep));

	auto g = Combine(program, expression, assignment, comparison, arithmetic, term, factor, scope, conditional,
					 optionalParamList, optionalCommaSep);

	// g.printParseTable();
	return g;
}

std::unique_ptr<CFG<Token>> g{nullptr};

std::vector<Token> tokenize(std::string &text) {
	std::vector<Token> res;
	for (std::size_t i = 0; i < text.length(); ++i) {
		if (std::isspace(text[i])) continue;
		if (std::isdigit(text[i])) {
			std::size_t num = 0;
			while (std::isdigit(text[i])) {
				num *= 10;
				num += text[i] - '0';
				++i;
			}
			--i;
			Token t = Token(Number, (uint8_t *)num);
			res.push_back(t);
		} else if (std::string_view(text).substr(i, 2) == "if" && !std::isalpha(text[i + 2])) {
			res.push_back(If);
			++i;
		} else if (std::string_view(text).substr(i, 5) == "while" && !std::isalpha(text[i + 5])) {
			res.push_back(While);
			i += 4;
		} else if (std::string_view(text).substr(i, 3) == "for" && !std::isalpha(text[i + 3])) {
			res.push_back(For);
			i += 2;
		} else if (std::isalpha(text[i])) {
			std::string name;
			while (std::isalpha(text[i])) {
				name += text[i];
				++i;
			}
			--i;
			char *s = new char[name.size() + 1];
			std::strcpy(s, name.data());
			res.push_back(Token(Identifier, reinterpret_cast<uint8_t *>(s)));
		} else {
			res.push_back(text[i]);
		}
	}
	res.push_back(Token::eof);
	return res;
}

auto createTokenizer() {
	OutputFSA<Token> if_ = OutputFSA("'if'", If);
	OutputFSA<Token> while_ = OutputFSA("'while'", While);
	OutputFSA<Token> for_ = OutputFSA("'for'", For);

	std::string_view digits = "('0'+'1'+'2'+'3'+'4'+'5'+'6'+'7'+'8'+'9')";
	std::string_view _az =
		"('a'+'b'+'c'+'d'+'e'+'f'+'g'+'h'+'i'+'j'+'k'+'l'+'m'+'n'+'o'+'p'+'q'+'r'+'s'+'t'+'u'+'v'+'w'+'x'+'y'+'z')";
	std::string_view _AZ =
		"('A'+'B'+'C'+'D'+'E'+'F'+'G'+'H'+'I'+'J'+'K'+'L'+'M'+'N'+'O'+'P'+'Q'+'R'+'S'+'T'+'U'+'V'+'W'+'X'+'Y'+'Z')";

	std::string numberRegex = std::format("({}!)", digits);
	OutputFSA<Token> number_ = OutputFSA(numberRegex, Number);
	OutputFSA<Token> identifier_ = OutputFSA(std::format("({}+{}+'_')!", _az,_AZ), Identifier);

	auto tokenizer = UnionOutputFSA<Token>(std::move(if_), std::move(while_), std::move(for_), std::move(number_),
									   std::move(identifier_));
	return tokenizer.determinizeToSSFT();
}

struct ASTNode {
	Token								  type;
	std::vector<std::unique_ptr<ASTNode>> children;

   public:
	ASTNode(Token type) : type(type) {}
};

std::ostream &operator<<(std::ostream &out, const ASTNode *node) {
	bits b;
	p_show<ASTNode>(out, node, b, [](std::ostream &out, const ASTNode *r) {
		out << " " << r->type << " ";
		if (r->type == Number) { out << reinterpret_cast<std::size_t>(r->type.data); }
		if (r->type == Identifier) { out << reinterpret_cast<char *>(r->type.data); }
		out << std::endl;
	});
	return out;
}
std::ostream &operator<<(std::ostream &out, const std::unique_ptr<ASTNode> &node) { return out << node.get(); }

template <>
struct std::formatter<ASTNode> : ostream_formatter {};

void random_tokens(std::vector<Token> &text, std::ostream &os) {
	for (Token &i : text) {
		if (i == Number) {
			os << rand();
		} else if (i == Identifier) {
			os << gen_random_string(rand() % 10 + 1) << " ";
		} else {
			os << i;
		}
	}
}

int main(int argc, char **argv) {
	g = std::make_unique<CFG<Token>>(createCFGNew());
	// g->printParseTable();
	if (argc >= 2 && std::string(argv[1]) == "generate") {
		srand(time(0));
		std::size_t tokens = -1;
		if (argc == 3) { tokens = std::stoi(argv[2]); }
		std::size_t cnt = 0;
		while (tokens > cnt) {
			auto v = g->generate(1000, 2000);
			cnt += v.size();
			random_tokens(v, std::cout);
		}
	} else {
		try {
			Parser<Token> parser(*g);
			//auto tokenizer = createTokenizer();

			// parser.enable_print = true;

			// EarleyParser<Token> earleyParser(*g);
			// earleyParser.expect_eof = true;

			std::stringstream buffer;
			std::string		  fileName = "test_file.txt";
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
			std::cout << "tokens: " << tokens.size() << std::endl;

			BENCH(parser.parse(tokens), 10, "BENCH building parse tree: ");
			auto t = parser.parse(tokens);
			//if (tokens.size() < 1000) std::cout << t << std::endl;

			// BENCH(earleyParser.recognize(tokens), 10, "BENCH earley parse: ");
			// assert(earleyParser.recognize(tokens));
			//  std::cout << t << std::endl;

			// BENCH(parser.ASTparse(tokens), 100, "BENCH building AST: ");
			if (tokens.size() < 1000) {
				auto ast = parser.ASTparse(tokens);
				std::cout << (ASTNode *)ast.get() << std::endl;
			}

		} catch (const std::exception &e) { std::cerr << e << std::endl; }
	}
}
