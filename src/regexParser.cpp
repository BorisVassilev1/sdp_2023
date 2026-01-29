#include <regexParser.hpp>
#include <stack>

namespace rgx {

using fl::CFG;

const Token Identifier	   = Token::createToken("id");
const Token Tuple		   = Token::createToken("tuple");
const Token Concatenation  = Token::createToken("concat");
const Token Concatenation_ = Token::createToken("concat'");
const Token Union		   = Token::createToken("union");
const Token Union_		   = Token::createToken("union'");
const Token KleeneStar	   = Token::createToken("star");
const Token KleeneStar_	   = Token::createToken("star'");
const Token Braces		   = Token::createToken("braces");
const Token Null		   = Token::createToken("null");

TokenizedString tokenize(const std::string &text) {
	std::vector<Token> res;
	for (std::size_t i = 0; i < text.length(); ++i) {
		if (std::isspace(text[i])) continue;
		if (text[i] == '\'') {
			std::string name;
			bool		escape = false;
			++i;
			while (i < text.length() && (escape || text[i] != '\'')) {
				if (text[i] == '\\' && !escape) {
					escape = true;
				} else {
					name += text[i];
					escape = false;
				}
				++i;
			}
			char *s = new char[name.size() + 1];
			strcpy(s, name.c_str());
			res.push_back(Token(Identifier, reinterpret_cast<uint8_t *>(s)));
		} else if (std::string_view(text.begin() + i, text.end()).starts_with("null")) {
			res.push_back(Null);
			i += 3;
		} else res.push_back(text[i]);
	}
	res.push_back(Token::eof);
	return res;
}

CFG<Token> createRegexGrammar() {
	CFG<Token> g(Union, Token::eof);
	g.nonTerminals = {Concatenation, Concatenation_, Union, Union_, KleeneStar, KleeneStar_, Tuple, Braces};
	g.terminals	   = {Token::eof, Identifier, '.', '+', '*', '<', ',', '>', '(', ')', '!', Null};

	g.addRule(Union, {Concatenation, Union_});
	g.addRule(Union_, {'+', Concatenation, Union_});
	g.addRule(Union_, {});
	g.addRule(Concatenation, {KleeneStar, Concatenation_});
	g.addRule(Concatenation_, {'.', KleeneStar, Concatenation_});
	g.addRule(Concatenation_, {});
	g.addRule(KleeneStar, {Braces, KleeneStar_});
	g.addRule(KleeneStar_, {'*'});
	g.addRule(KleeneStar_, {'!'});
	g.addRule(KleeneStar_, {});
	g.addRule(Tuple, {'<', Identifier, ',', Identifier, '>'});
	g.addRule(Tuple, {Identifier});
	g.addRule(Tuple, {Null});
	g.addRule(Braces, {'(', Union, ')'});
	g.addRule(Braces, {Tuple});
	return g;
}

std::unique_ptr<Regex> generateRegex() {
	if (rand() % 2) return std::make_unique<TupleRegex<char>>(fl::gen_random_string(0, 3), fl::gen_random_string(0, 3));

	int r = rand() % 3;
	switch (r) {
		case 0: return std::make_unique<UnionRegex>(generateRegex(), generateRegex());
		case 1: return std::make_unique<ConcatRegex>(generateRegex(), generateRegex());
		case 2: return std::make_unique<KleeneStarRegex>(generateRegex());
	}
	return nullptr;		// Should never reach here
}

std::string generateRegexString(std::size_t min) {
	while (true) {
		std::stringstream ss;
		auto			  regex = generateRegex();
		regex->print(ss);
		auto str = ss.str();
		if (str.size() >= min) { return str; }
	}
}

std::unique_ptr<Regex> parseTreeToRegex(const ParseNode<Token> *root, TokenizedString &owner) {
	if (!root) return nullptr;

	if (root->value == Union || root->value == Union_) {
		return std::make_unique<UnionRegex>(parseTreeToRegex(root->children[0].get(), owner),
											parseTreeToRegex(root->children[1].get(), owner));
	} else if (root->value == Concatenation || root->value == Concatenation_) {
		return std::make_unique<ConcatRegex>(parseTreeToRegex(root->children[0].get(), owner),
											 parseTreeToRegex(root->children[1].get(), owner));
	} else if (root->value == KleeneStar) {
		if ((size_t)root->value.data == (size_t)'!')
			return std::make_unique<KleenePlusRegex>(parseTreeToRegex(root->children[0].get(), owner));
		else return std::make_unique<KleeneStarRegex>(parseTreeToRegex(root->children[0].get(), owner));

	} else if (root->value == Tuple) {
		if (root->children.size() == 1) {
			if(root->children[0]->value == Null) {
				return std::make_unique<TupleRegex<char>>(std::string(1ull, '\0'), std::string());
			} else
			return std::make_unique<TupleRegex<char>>(
				std::string(reinterpret_cast<char *>(root->children[0]->value.data)), std::string());
		} else if (root->children.size() == 2) {
			char *left	 = reinterpret_cast<char *>(root->children[0]->value.data);
			char *right	 = reinterpret_cast<char *>(root->children[1]->value.data);
			auto  left_s = std::string(left), right_s = std::string(right);
			return std::make_unique<TupleRegex<char>>(std::move(left_s), std::move(right_s));
		}
		return nullptr;
	}
	return nullptr;
}

std::unique_ptr<Regex> parseRegex(const std::string &text) {
	static RegexParser parser;
	auto			   tokens = tokenize(text);
	if (tokens.empty()) return nullptr;
	auto parseTree = parser.parse(tokens);
	auto r		   = parseTreeToRegex(parseTree.get(), tokens);
	return toLeftAssoc(std::move(r));
}

std::unique_ptr<Regex> toLeftAssoc(std::unique_ptr<Regex> &&regex) {
	if (auto *r = dynamic_cast<UnionRegex *>(regex.get())) {
		if (auto *right = dynamic_cast<UnionRegex *>(r->right.get())) {
			auto b		= std::move(right->left);
			right->left = std::move(regex);
			regex		= std::move(r->right);
			r->right	= std::move(b);

			regex = toLeftAssoc(std::move(regex));
		} else {
			r->left	 = toLeftAssoc(std::move(r->left));
			r->right = toLeftAssoc(std::move(r->right));
		}
	} else if (auto *r = dynamic_cast<ConcatRegex *>(regex.get())) {
		if (auto *right = dynamic_cast<ConcatRegex *>(r->right.get())) {
			auto b		= std::move(right->left);
			right->left = std::move(regex);
			regex		= std::move(r->right);
			r->right	= std::move(b);

			regex = toLeftAssoc(std::move(regex));
		} else {
			r->left	 = toLeftAssoc(std::move(r->left));
			r->right = toLeftAssoc(std::move(r->right));
		}
	} else if (auto *r = dynamic_cast<KleeneStarRegex *>(regex.get())) {
		r->child = toLeftAssoc(std::move(r->child));
	}
	return std::move(regex);
}

std::string Alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._?;:/!@#$%^&*()-+=<>[]{}|\\`~";
std::string lb		 = "«";
std::string rb		 = "»";
std::string cb		 = "†";

std::string identity(const std::string &alphabet) {
	std::string result;
	for (std::size_t i = 0; i < alphabet.size(); ++i) {
		if (i > 0) result += "+";
		char c = alphabet[i];
		result += std::format("<'{}', '{}'>", c, c);
	}
	return result;
}

std::string optionalReplace(const std::string &regex, const std::string &alphabet) {
	auto id = identity(alphabet);
	return std::format("({})*.(({}).({})*)*", id, regex, id);
}
};	   // namespace rgx

std::ostream &operator<<(std::ostream &out, const rgx::Regex &regex) {
	regex.print(out);
	return out;
}

std::ostream &operator<<(std::ostream &out, std::unique_ptr<rgx::Regex> &regex) {
	if (regex) {
		regex->print(out);
	} else {
		out << "nullptr";
	}
	return out;
}
