#ifndef MUSIC_DSL_PARSER_H
#define MUSIC_DSL_PARSER_H

#include <cstddef>
#include <memory>
#include <string>

#include "ast.h"

namespace dsl {

enum class TokenType {
    Tempo,
    Play,
    Rest,
    Loop,
    Identifier,
    Number,
    Comma,
    Semicolon,
    LBrace,
    RBrace,
    EndOfInput
};

struct Token {
    TokenType type = TokenType::EndOfInput;
    std::string lexeme;
    double value = 0.0;
    int line = 1;
    int column = 1;

    // Un NUMBER es entero si su lexema no tiene parte fraccionaria.
    bool isInteger() const;
};

class Lexer {
public:
    explicit Lexer(std::string source);

    // Ignora espacios y comentarios // hasta el final de línea.
    // Lanza std::runtime_error ante un carácter inválido.
    Token next();

private:
    char peek(std::size_t offset = 0) const;
    char advance();
    void skipWhitespace();

    Token scanNumber();
    Token scanIdentifierOrKeyword();
    Token scanSymbol();

    std::string source_;
    std::size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
};

class Parser {
public:
    explicit Parser(std::string source);

    // Lanza std::runtime_error con la posición del primer error de sintaxis.
    std::unique_ptr<Program> parseProgram();

private:
    void advance();
    Token expect(TokenType type, const std::string& what);

    std::unique_ptr<Node> parseStatement();
    std::unique_ptr<Tempo> parseTempo();
    std::unique_ptr<Play> parsePlay();
    std::unique_ptr<Rest> parseRest();
    std::unique_ptr<Loop> parseLoop();
    NodeList parseBlockBody();

    int requireInteger(const Token& tok, const std::string& what);
    static std::string describe(const Token& tok);
    [[noreturn]] void error(const Token& tok, const std::string& message) const;

    Lexer lexer_;
    Token current_;
};

// Atajo para el caso habitual: parsear un programa completo desde un string.
std::unique_ptr<Program> parse(const std::string& source);

}  // namespace dsl

#endif  // MUSIC_DSL_PARSER_H
