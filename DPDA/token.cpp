#include <DPDA/token.h>
#include <memory>
#include <unordered_map>

using NamesMap = std::unordered_map<std::size_t, std::string>;
static std::unique_ptr<NamesMap> TokenNames = nullptr;

static std::unordered_map<std::size_t, std::string> &getTokenNames() {
	if(!TokenNames) return *(TokenNames = std::make_unique<NamesMap>());
	else return *TokenNames;
}

Token Token::createToken(const std::string &name, std::size_t value) {
	getTokenNames().insert({value, name});
	return Token(value);
}

std::ostream &operator<<(std::ostream &out, const Token &v) {
	return out << getTokenNames().find(v.value)->second;
}

std::size_t Token::size = 256;

const Token Token::eps	= Token::createToken("ε", 0);
const Token Token::eof	= Token::createToken("eof");
