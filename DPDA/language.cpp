#include <DPDA/parser.h>
#include <DPDA/cfg.h>
#include <DPDA/utils.h>
#include <DPDA/token.h>
#include <cctype>
#include <exception>
#include <fstream>

const Token Program		= Token::createToken("Program");
const Token Program_	= Token::createToken("Program'");
const Token Number		= Token::createToken("Number");
const Token Number_		= Token::createToken("Number'");
const Token Identifier	= Token::createToken("Identifier");
const Token Identifier_ = Token::createToken("Identifier'");
const Token NL			= Token::createToken("NL", '\n');
const Token Expression	= Token::createToken("Expression");
const Token Expression_ = Token::createToken("Expression'");
const Token Assignment	= Token::createToken("Assignment");
const Token Assignment_	= Token::createToken("Assignment'");
const Token Arithmetic	= Token::createToken("Arithmetic");
const Token Arithmetic_ = Token::createToken("Arithmetic'");
const Token Term		= Token::createToken("Term");
const Token Term_		= Token::createToken("Term'");
const Token Factor		= Token::createToken("Factor");

std::unique_ptr<CFG<Token>> g = nullptr;

void createCFG() {
	g = std::make_unique<CFG<Token>>(Program, Token::eof);
	for (int i = 1; i < 128; ++i) {
		g->terminals.insert(Token::createToken(std::string(1, i), i));
	}
	g->terminals.insert(Token::eof);
	g->nonTerminals = {Program,		Program_,	Number,		Number_,	 Identifier, Identifier_, Expression,
					   Expression_, Assignment, Assignment_, Arithmetic, Arithmetic_, Term,		 Term_,		  Factor};

	g->addRule(Number, {'0'});
	for (int i = 0; i < 10; ++i) {
		if (i != 0) g->addRule(Number, {char('0' + i), Number_});
		g->addRule(Number_, {char('0' + i), Number_});
	}
	g->addRule(Number_, {});

	for (int i = 'A'; i <= 'Z'; ++i) {
		g->addRule(Identifier, {char(i), Identifier_});
		g->addRule(Identifier_, {char(i), Identifier_});
	}
	for (int i = 'a'; i <= 'z'; ++i) {
		g->addRule(Identifier, {char(i), Identifier_});
		g->addRule(Identifier_, {char(i), Identifier_});
	}
	g->addRule(Identifier_, {});

	g->addRule(Program, {Expression, Program_});
	g->addRule(Program, {});
	g->addRule(Program_, {Expression, Program_});
	g->addRule(Program_, {});

	g->addRule(Expression, {Assignment, ';'});
	g->addRule(Assignment, {Arithmetic, Assignment_});
	g->addRule(Assignment_, {'=', Arithmetic, Assignment_});
	g->addRule(Assignment_, {});

	g->addRule(Arithmetic, {Term, Arithmetic_});
	g->addRule(Arithmetic_, {'+', Term, Arithmetic_});
	g->addRule(Arithmetic_, {'-', Term, Arithmetic_});
	g->addRule(Arithmetic_, {});

	g->addRule(Term, {Factor, Term_});
	g->addRule(Term_, {'*', Factor, Term_});
	g->addRule(Term_, {'/', Factor, Term_});
	g->addRule(Term_, {});

	g->addRule(Factor, {Identifier});
	g->addRule(Factor, {Number});
	g->addRule(Factor, {'(', Arithmetic, ')'});
}

std::string tokenize(std::string &text) {
	std::string res;
	for(const char c : text) {
		if(!std::isspace(c)) {
			res += c;
		}
	}
	return res;
}


int main() {
	createCFG();

	g->printParseTable();

	try {
		Parser<State, Token> parser(*g);

		std::stringstream buffer;
		std::ifstream	  file("test_file.txt");
		buffer << file.rdbuf();

		std::string text = buffer.str();
		std::cout << text << std::endl;

		std::string tokens = tokenize(text);

		auto t = parser.parse(tokens);
		std::cout << t << std::endl;

	} catch (const std::exception &e) { std::cerr << e << std::endl; }
}
