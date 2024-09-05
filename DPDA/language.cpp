#include <DPDA/parser.h>
#include <DPDA/cfg.h>
#include <DPDA/utils.h>
#include <DPDA/token.h>
#include <cctype>
#include <exception>
#include <fstream>
#include <memory>
#include <ostream>
#include <string_view>

const Token Program = Token::createToken("Program");
// const Token Program_	= Token::createToken("Program'");
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
const Token Comparison_	= Token::createToken("Comparison'");

const Token If	  = Token::createToken("if");
const Token While = Token::createToken("while");
const Token For	  = Token::createToken("for");

std::unique_ptr<CFG<Token>> g = nullptr;

void createCFG() {
	g = std::make_unique<CFG<Token>>(Program, Token::eof);
	for (int i = 1; i < 128; ++i) {
		g->terminals.insert(Token::createToken(std::string(1, i), i));
	}
	g->terminals.insert(Token::eof);
	g->terminals.insert(If);
	g->terminals.insert(While);
	g->terminals.insert(For);
	g->nonTerminals = {Program,		Number,		Number_,	 Identifier, Identifier_, Expression,
					   Expression_, Assignment, Assignment_, Arithmetic, Arithmetic_, Term,
					   Term_,		Factor,		Conditional, Comparison, Comparison_};

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

	g->addRule(Program, {Expression, Program});
	g->addRule(Program, {Conditional, ';', Program});
	g->addRule(Program, {});

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

	g->addRule(Factor, {Identifier});
	g->addRule(Factor, {Number});
	g->addRule(Factor, {'(', Assignment, ')'});

	g->addRule(Conditional, {If, '(', Assignment, ')', '{', Program, '}'});
	g->addRule(Conditional, {While, '(', Assignment, ')', '{', Program, '}'});
	g->addRule(Conditional, {For, '(', Expression, Expression, Assignment, ')', '{', Program, '}'});
}

std::vector<Token> tokenize(std::string &text) {
	std::vector<Token> res;
	for (std::size_t i = 0; i < text.length(); ++i) {
		if (std::isspace(text[i])) continue;
		if (std::string_view(text).substr(i, 2) == "if") {
			res.push_back(If);
			++i;
		} else if (std::string_view(text).substr(i, 5) == "while") {
			res.push_back(While);
			i += 4;
		} else if (std::string_view(text).substr(i, 3) == "for") {
			res.push_back(For);
			i += 2;
		} else {
			res.push_back(text[i]);
		}
	}
	res.push_back(Token::eof);
	return res;
}

std::unordered_set<Token> binaryOpsPri = {Assignment, Arithmetic, Term, Comparison};
std::unordered_set<Token> binaryOpsSec = {Assignment_, Arithmetic_, Term_, Comparison_};
std::unordered_set<Token> punctuation  = {'(', ')', '[', ']', '{', '}', ';'};

struct ASTNode {
	std::vector<std::unique_ptr<ASTNode>> children;
	std::vector<Token>					  value;
	Token								  type;

   public:
	ASTNode(Token type) : type(type) {}
};

std::ostream &operator<<(std::ostream &out, const std::unique_ptr<ASTNode> &node) {
	bits b;
	p_show<ASTNode>(out, node.get(), b,
					[](std::ostream &out, const ASTNode *r) { out << " " << r->type << " " << r->value << std::endl; });
	return out;
}

template <>
struct std::formatter<ASTNode> : ostream_formatter {};

std::unique_ptr<ASTNode> makeAST(const std::unique_ptr<ParseNode<Token>> &parseNode) {
	auto node = std::make_unique<ASTNode>(parseNode->value);

	if (node->type == Number || node->type == Identifier) {
		std::vector<Token>											   res;
		std::function<void(const std::unique_ptr<ParseNode<Token>> &)> cut;
		cut = [&](const std::unique_ptr<ParseNode<Token>> &parseNode) -> void {
			if (parseNode->children.size() < 2) return;
			char letter = (char)parseNode->children[0]->value;
			res.push_back(letter);
			cut(parseNode->children[1]);
		};
		cut(parseNode);
		node->value = res;
	} else if (node->type == Program) {
		std::function<void(const std::unique_ptr<ParseNode<Token>> &)> cut;
		cut = [&](const std::unique_ptr<ParseNode<Token>> &parseNode) -> void {
			if (parseNode->value != Program) return;
			if (parseNode->children.size() < 2) return;

			auto newChild = makeAST(parseNode->children[0]);
			if (newChild) node->children.push_back(std::move(newChild));

			for (uint i = 1; i < parseNode->children.size(); ++i) {
				cut(parseNode->children[i]);
			}
		};
		cut(parseNode);
	} else if (punctuation.contains(node->type)) {
		return nullptr;
	} else if (binaryOpsPri.contains(node->type)) {
		// node->type = parseNode->children[1]->children[0]->value;
		if (parseNode->children[1]->children[0]->value == Token::eps) {
			auto newChild = makeAST(parseNode->children[0]);
			if (newChild) node->children.push_back(std::move(newChild));
		} else {
			node->type = parseNode->children[1]->children[0]->value;
			auto a	   = makeAST(parseNode->children[0]);
			auto b	   = makeAST(parseNode->children[1]);
			if (a) node->children.push_back(std::move(a));
			if (b) node->children.push_back(std::move(b));
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

	if (node->children.size() == 1) { return std::move(node->children[0]); }

	return node;
}

int main() {
	createCFG();

	// g->printParseTable();

	try {
		Parser<State, Token> parser(*g);
		// parser.enable_print = true;

		std::stringstream buffer;
		std::ifstream	  file("test_file.txt");
		buffer << file.rdbuf();

		std::string text = buffer.str();
		// std::cout << text << std::endl;

		auto tokens = tokenize(text);
		// std::cout << tokens << std::endl;

		auto t = parser.parse(tokens);
		// std::cout << t << std::endl;

		auto ast = makeAST(t);
		std::cout << ast << std::endl;

	} catch (const std::exception &e) { std::cerr << e << std::endl; }
}
