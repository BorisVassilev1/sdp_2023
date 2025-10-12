#include <DPDA/token.h>
#include <memory>
#include <unordered_map>

using NamesMap = std::unordered_map<std::size_t, std::string>;
static std::unique_ptr<NamesMap> TokenNames = nullptr;

static std::unordered_map<std::size_t, std::string> &getTokenNames() {
	if(!TokenNames) return *(TokenNames = std::make_unique<NamesMap>());
	else return *TokenNames;
}

// registers a Token with a name
Token Token::createToken(const std::string &name, std::size_t value) {
	getTokenNames().insert({value, name});
	return Token(value);
}

Token Token::createDependentToken(const Token &base) {
	getTokenNames().insert({++Token::size, getTokenNames().find(base.value)->second + "'" });
	return Token(Token::size);
}

std::ostream &operator<<(std::ostream &out, const Token &v) {
	if(v.value && v.value < 128) return out << static_cast<char>(v.value);
	auto it = getTokenNames().find(v.value);
	if(it == getTokenNames().end()) {
		return out << v.value;
	}
	return out << it->second;
}

std::size_t Token::size = 256;

const Token Token::eps	= Token::createToken("ε", 0);
const Token Token::eof	= Token::createToken("eof");
