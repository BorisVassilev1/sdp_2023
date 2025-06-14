#pragma once

#include <DPDA/parser.h>
#include <DPDA/token.h>
#include <cstring>
#include "util/bench.hpp"

extern const Token Identifier;
extern const Token Tuple;
extern const Token Concatenation;
extern const Token Concatenation_;
extern const Token Union;
extern const Token Union_;
extern const Token KleeneStar;
extern const Token KleeneStar_;
extern const Token Braces;

std::vector<Token> tokenize(const std::string &text);

CFG<Token> createRegexGrammar();

class RegexParser : public Parser<Token> {
   public:
	RegexParser() : Parser<Token>(createRegexGrammar()) {}

	std::unique_ptr<ParseNode<Token>> makeParseTree(
		const std::vector<std::reference_wrapper<const Parser<Token>::DeltaMap::value_type>> &productions,
		const std::vector<Token> &word, int &k, int &j) {
		std::vector<std::unique_ptr<ParseNode<Token>>> children;
		auto										  &product = std::get<1>(productions[k].get().second);
		int											   old_k   = k;
		++k;

		bool skipped_star = false;
		for (size_t i = 0; i < product.size(); ++i) {
			if (product[i] == word[j]) {
				if (word[j] == Identifier || word[j] == '*') {
					children.push_back(std::make_unique<ParseNode<Token>>(word[j]));
				}
				++j;
			} else {
				auto child = makeParseTree(productions, word, k, j);
				if (child->value == '*') skipped_star = true;
				if (!child->children.empty()) children.push_back(std::move(child));
			}
		}
		if (children.size() == 1 && !skipped_star) { return std::move(children[0]); }
		return std::make_unique<ParseNode<Token>>(std::get<2>(productions[old_k].get().first), std::move(children));
	}

	auto parse(const std::vector<Token> &tokens) {
		auto prod = generateProductions(tokens);
		int	 i = 0, k = 0;
		return makeParseTree(prod.second, tokens, i, k);
	}
};

class Regex {
   public:
	virtual void print(std::ostream &out) const = 0;
	virtual ~Regex()							= default;
};

class UnionRegex : public Regex {
   public:
	std::unique_ptr<Regex> left;
	std::unique_ptr<Regex> right;
	UnionRegex(std::unique_ptr<Regex> left, std::unique_ptr<Regex> right)
		: left(std::move(left)), right(std::move(right)) {}

	void print(std::ostream &out) const override {
		out << "(";
		if (left) left->print(out);
		out << "+";
		if (right) right->print(out);
		out << ")";
	}
};

class ConcatRegex : public Regex {
   public:
	std::unique_ptr<Regex> left;
	std::unique_ptr<Regex> right;
	ConcatRegex(std::unique_ptr<Regex> left, std::unique_ptr<Regex> right)
		: left(std::move(left)), right(std::move(right)) {}

	void print(std::ostream &out) const override {
		out << "(";
		if (left) left->print(out);
		out << ".";
		if (right) right->print(out);
		out << ")";
	}
};

class KleeneStarRegex : public Regex {
   public:
	std::unique_ptr<Regex> child;

	KleeneStarRegex(std::unique_ptr<Regex> child) : child(std::move(child)) {}
	void print(std::ostream &out) const override {
		out << "(";
		if (child) child->print(out);
		out << ")*";
	}
};

class TupleRegex : public Regex {
   public:
	std::string left;
	std::string right;
	TupleRegex(std::string left, std::string right) : left(left), right(right) {}

	void print(std::ostream &out) const override {
		out << "<'" << left << "','" << right << "'>";
	}
};

std::unique_ptr<Regex> generateRegex();
std::string			   generateRegexString(std::size_t min);

std::unique_ptr<Regex> parseTreeToRegex(const ParseNode<Token> *root);
std::unique_ptr<Regex> parseRegex(const std::string &text);

std::unique_ptr<Regex> toLeftAssoc(std::unique_ptr<Regex> regex);

extern std::string Alphabet;

std::string identity(const std::string &alphabet);
std::string optionalReplace(const std::string &regex,const std::string &alphabet = Alphabet);

std::ostream &operator<<(std::ostream &out, const Regex &regex);
std::ostream &operator<<(std::ostream &out, std::unique_ptr<Regex> &regex);
