#include <DPDA/parser.h>
#include <DPDA/cfg.h>
#include <DPDA/utils.h>
#include <DPDA/token.h>
#include <cassert>
#include <cctype>
#include <cstring>
#include <exception>
#include <fstream>
#include <memory>
#include <ostream>
#include <string_view>
#include <DPDA/earley.hpp>
#include <util/bench.hpp>

const Token Program		= Token::createToken("Program");
const Token Number		= Token::createToken("Number");
const Token Number_		= Token::createToken("Number'");
const Token Identifier	= Token::createToken("Identifier");
const Token Identifier_ = Token::createToken("Identifier'");
const Token NL			= Token::createToken("NL", '\n');
const Token Expression	= Token::createToken("Expression");
const Token Expression_ = Token::createToken("Expression'");
const Token Assignment	= Token::createToken("Assignment");
const Token Assignment_ = Token::createToken("Assignment'");
const Token Arithmetic	= Token::createToken("Arithmetic");
const Token Arithmetic_ = Token::createToken("Arithmetic'");
const Token Term		= Token::createToken("Term");
const Token Term_		= Token::createToken("Term'");
const Token Factor		= Token::createToken("Factor");
const Token Conditional = Token::createToken("Conditional");
const Token Comparison	= Token::createToken("Comparison");
const Token Comparison_ = Token::createToken("Comparison'");
const Token CommaSep	= Token::createToken("CommaSep");
const Token CommaSep_	= Token::createToken("CommaSep'");
const Token ParamList	= Token::createToken("ParamList");
const Token Scope		= Token::createToken("Scope");

const Token If	  = Token::createToken("if");
const Token While = Token::createToken("while");
const Token For	  = Token::createToken("for");

std::unique_ptr<CFG<Token>> g = nullptr;

void createCFG() {
	g = std::make_unique<CFG<Token>>(Program, Token::eof);

	g->terminals.insert({NL, '.', ',', ';', '(', ')', '[', ']', '{', '}', '=', '+', '-', '*', '/', '<', '>'});
	g->terminals.insert(Token::eof);
	g->terminals.insert(If);
	g->terminals.insert(While);
	g->terminals.insert(For);
	g->terminals.insert(Number);
	g->terminals.insert(Identifier);

	g->nonTerminals = {Program,		Expression, Expression_, Assignment, Assignment_, Arithmetic,
					   Arithmetic_, Term,		Term_,		 Factor,	 Conditional, Comparison,
					   Comparison_, CommaSep,	CommaSep_,	 ParamList,	 Scope};

	g->addRule(Program, {Expression, Program});
	g->addRule(Program, {Conditional, ';', Program});
	g->addRule(Program, {Scope, Program});
	g->addRule(Program, {});

	g->addRule(Scope, {'{', Program, '}'});

	g->addRule(Expression, {Assignment, ';'});
	g->addRule(Expression, {';'});
	g->addRule(Assignment, {Comparison, Assignment_});
	g->addRule(Assignment_, {'=', Comparison, Assignment_});
	g->addRule(Assignment_, {});

	g->addRule(Comparison, {Arithmetic, Comparison_});
	g->addRule(Comparison_, {'<', Arithmetic, Comparison_});
	g->addRule(Comparison_, {'>', Arithmetic, Comparison_});
	g->addRule(Comparison_, {});

	g->addRule(Arithmetic, {Term, Arithmetic_});
	g->addRule(Arithmetic_, {'+', Term, Arithmetic_});
	g->addRule(Arithmetic_, {'-', Term, Arithmetic_});
	g->addRule(Arithmetic_, {});

	g->addRule(Term, {Factor, Term_});
	g->addRule(Term_, {'*', Factor, Term_});
	g->addRule(Term_, {'/', Factor, Term_});
	g->addRule(Term_, {});

	g->addRule(Factor, {Identifier, ParamList});
	g->addRule(Factor, {Number});
	g->addRule(Factor, {'(', Assignment, ')'});

	g->addRule(Conditional, {If, '(', Assignment, ')', '{', Program, '}'});
	g->addRule(Conditional, {While, '(', Assignment, ')', '{', Program, '}'});
	g->addRule(Conditional, {For, '(', Expression, Expression, Assignment, ')', '{', Program, '}'});

	g->addRule(ParamList, {});
	g->addRule(ParamList, {'(', CommaSep, ')', ParamList});
	g->addRule(ParamList, {'[', CommaSep, ']', ParamList});

	g->addRule(CommaSep, {Assignment, CommaSep_});
	g->addRule(CommaSep, {});
	g->addRule(CommaSep_, {',', Assignment, CommaSep_});
	g->addRule(CommaSep_, {});
}

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

std::unordered_set<Token> binaryOpsPri = {
	Assignment,
	Arithmetic,
	Term,
	Comparison,
	//CommaSep,
};
std::unordered_set<Token> binaryOpsSec		  = {Assignment_, Arithmetic_, Term_, Comparison_, ParamList};
std::unordered_set<Token> punctuation		  = {'(', ')', '[', ']', '{', '}', ';'};
std::unordered_set<Token> preservePunctuation = {Scope};
std::unordered_set<Token> Params			  = {ParamList};

struct ASTNode {
	std::vector<std::unique_ptr<ASTNode>> children;
	Token								  type;

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

std::unique_ptr<ASTNode> makeAST(const std::unique_ptr<ParseNode<Token>> &parseNode) {
	auto node = std::make_unique<ASTNode>(parseNode->value);
	if(parseNode->value == Token::eps) {
		return nullptr;
	}

	if (node->type == Program) {
		std::function<void(const std::unique_ptr<ParseNode<Token>> &)> cut;
		cut = [&](const std::unique_ptr<ParseNode<Token>> &parseNode) -> void {
			if (parseNode->value != Program) return;
			if (parseNode->children.size() < 2) return;

			auto newChild = makeAST(parseNode->children[0]);
			if (newChild) node->children.push_back(std::move(newChild));

			for (std::size_t i = 1; i < parseNode->children.size(); ++i) {
				cut(parseNode->children[i]);
			}
		};
		cut(parseNode);
	} else if (punctuation.contains(node->type)) {
		return nullptr;
	} else if (binaryOpsPri.contains(node->type)) {
		if(parseNode->children.size() == 0) {
			return node;
		}
		if(parseNode->children.size() == 1) {
			auto newChild = makeAST(parseNode->children[0]);
			if (newChild) node->children.push_back(std::move(newChild));
			return node;
		}
		if (parseNode->children[1]->children[0]->value == Token::eps) {
			auto newChild = makeAST(parseNode->children[0]);
			if (newChild) node->children.push_back(std::move(newChild));
		} else {
			node->type = parseNode->children[1]->children[0]->value;
			auto a	   = makeAST(parseNode->children[0]);
			auto b	   = makeAST(parseNode->children[1]);
			if (a) node->children.push_back(std::move(a));
			if (b) node->children.push_back(std::move(b));

			std::function<void(std::unique_ptr<ASTNode> &)> cut;
			cut = [&node, &cut](std::unique_ptr<ASTNode> &Node) -> void {
				if (Node->type != node->type) return;
				if (Node->children.size() < 2) return;
				if (&Node == &node) return cut(Node->children[1]);

				ASTNode *child = Node.get();

				node->children.push_back(std::move(child->children[0]));
				std::swap(node->children.back(), node->children[node->children.size() - 2]);

				if (child->children[1]->type != node->type) {
					auto last			  = std::move(child->children[1]);
					node->children.back() = std::move(last);
					return;
				}

				cut(child->children[1]);
			};
			cut(node);
		}
	} else if (binaryOpsSec.contains(node->type)) {
		if (parseNode->children.size() == 2) return makeAST(parseNode->children[1]);
		else if (parseNode->children.size() == 3) {
			node->type = parseNode->children[0]->value;
			auto a	   = makeAST(parseNode->children[1]);
			auto b	   = makeAST(parseNode->children[2]);
			if (a) node->children.push_back(std::move(a));
			if (b) node->children.push_back(std::move(b));
		} else return nullptr;
	} else {
		if (parseNode->children.size() > 0 && parseNode->children[0]->value == Token::eps) return nullptr;

		for (const auto &child : parseNode->children) {
			auto newChild = makeAST(child);
			if (newChild) node->children.push_back(std::move(newChild));
		}
	}

	if (node->children.size() == 1 && !preservePunctuation.contains(node->type)) {
		return std::move(node->children[0]);
	}

	return node;
}

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
	createCFG();

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
			// parser.enable_print = true;

			EarleyParser<Token> earleyParser(*g);
			earleyParser.expect_eof = true;

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

			// BENCH(earleyParser.recognize(tokens), 10, "BENCH earley parse: ");
			// assert(earleyParser.recognize(tokens));
			//  std::cout << t << std::endl;

			BENCH(makeAST(t), 100, "BENCH building AST: ");
			if (tokens.size() < 1000) {
				auto ast = makeAST(t);
				std::cout << ast << std::endl;
			}

		} catch (const std::exception &e) { std::cerr << e << std::endl; }
	}
}
