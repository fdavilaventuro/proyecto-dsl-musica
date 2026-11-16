#include "parser.h"

#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

namespace dsl {
namespace {

[[noreturn]] void lexerError(int line, int column, const std::string& message) {
    throw std::runtime_error("error léxico (línea " + std::to_string(line) + ", columna " +
                             std::to_string(column) + "): " + message);
}

[[noreturn]] void parserError(int line, int column, const std::string& message) {
    throw std::runtime_error("error de sintaxis (línea " + std::to_string(line) + ", columna " +
                             std::to_string(column) + "): " + message);
}

TokenType classifyIdentifier(const std::string& text) {
    if (text == "tempo") return TokenType::Tempo;
    if (text == "play") return TokenType::Play;
    if (text == "rest") return TokenType::Rest;
    if (text == "loop") return TokenType::Loop;
    return TokenType::Identifier;
}

bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }
bool isAlpha(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0; }
bool isAlphaNumeric(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0; }
bool isWhitespace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

}  // namespace

bool Token::isInteger() const {
    return type == TokenType::Number && lexeme.find_first_of(".eE") == std::string::npos;
}

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

char Lexer::peek(std::size_t offset) const {
    const std::size_t index = pos_ + offset;
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() {
    const char c = source_[pos_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (true) {
        while (pos_ < source_.size() && isWhitespace(source_[pos_])) advance();

        if (peek() != '/' || peek(1) != '/') return;
        while (pos_ < source_.size() && peek() != '\n') advance();
    }
}

Token Lexer::next() {
    skipWhitespace();

    if (pos_ >= source_.size()) {
        return Token{TokenType::EndOfInput, "", 0.0, line_, column_};
    }

    const char c = peek();
    if (isDigit(c) || (c == '.' && isDigit(peek(1)))) return scanNumber();
    if (isAlpha(c) || c == '_') return scanIdentifierOrKeyword();
    if (c == '#') {
        lexerError(line_, column_, "el '#' debe seguir al nombre de la nota (por ejemplo, C#4)");
    }
    return scanSymbol();
}

Token Lexer::scanNumber() {
    const std::size_t start = pos_;
    const int startLine = line_;
    const int startColumn = column_;

    while (isDigit(peek())) advance();
    if (peek() == '.') {
        advance();
        while (isDigit(peek())) advance();
    }

    const std::string lexeme = source_.substr(start, pos_ - start);
    double value = 0.0;
    try {
        value = std::stod(lexeme);
    } catch (const std::out_of_range&) {
        lexerError(startLine, startColumn, "el número '" + lexeme + "' está fuera de rango");
    }
    return Token{TokenType::Number, lexeme, value, startLine, startColumn};
}

Token Lexer::scanIdentifierOrKeyword() {
    const std::size_t start = pos_;
    const int startLine = line_;
    const int startColumn = column_;

    // '#' permite sostenidos dentro del nombre de nota (C#4); los bemoles usan 'b' (Bb3).
    while (isAlphaNumeric(peek()) || peek() == '_' || peek() == '#') advance();

    const std::string lexeme = source_.substr(start, pos_ - start);
    return Token{classifyIdentifier(lexeme), lexeme, 0.0, startLine, startColumn};
}

Token Lexer::scanSymbol() {
    const int startLine = line_;
    const int startColumn = column_;
    const char c = advance();

    switch (c) {
        case ',': return Token{TokenType::Comma, ",", 0.0, startLine, startColumn};
        case ';': return Token{TokenType::Semicolon, ";", 0.0, startLine, startColumn};
        case '{': return Token{TokenType::LBrace, "{", 0.0, startLine, startColumn};
        case '}': return Token{TokenType::RBrace, "}", 0.0, startLine, startColumn};
        default:
            lexerError(startLine, startColumn, "carácter inesperado '" + std::string(1, c) + "'");
    }
}

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------

Parser::Parser(std::string source) : lexer_(std::move(source)) { advance(); }

void Parser::advance() { current_ = lexer_.next(); }

std::string Parser::describe(const Token& tok) {
    switch (tok.type) {
        case TokenType::Tempo: return "'tempo'";
        case TokenType::Play: return "'play'";
        case TokenType::Rest: return "'rest'";
        case TokenType::Loop: return "'loop'";
        case TokenType::Identifier: return "el identificador '" + tok.lexeme + "'";
        case TokenType::Number: return "el número '" + tok.lexeme + "'";
        case TokenType::Comma: return "','";
        case TokenType::Semicolon: return "';'";
        case TokenType::LBrace: return "'{'";
        case TokenType::RBrace: return "'}'";
        case TokenType::EndOfInput: return "el final de la entrada";
    }
    return "un token desconocido";
}

void Parser::error(const Token& tok, const std::string& message) const {
    parserError(tok.line, tok.column, message);
}

Token Parser::expect(TokenType type, const std::string& what) {
    if (current_.type != type) {
        error(current_, "se esperaba " + what + " pero se encontró " + describe(current_));
    }
    Token tok = current_;
    advance();
    return tok;
}

std::unique_ptr<Program> Parser::parseProgram() {
    NodeList statements;
    while (current_.type != TokenType::EndOfInput) {
        if (current_.type == TokenType::RBrace) {
            error(current_, "se encontró '}' sin un 'loop' abierto");
        }
        statements.push_back(parseStatement());
    }
    return std::make_unique<Program>(std::move(statements));
}

std::unique_ptr<Node> Parser::parseStatement() {
    switch (current_.type) {
        case TokenType::Tempo: return parseTempo();
        case TokenType::Play: return parsePlay();
        case TokenType::Rest: return parseRest();
        case TokenType::Loop: return parseLoop();
        default:
            error(current_,
                  "se esperaba una instrucción ('tempo', 'play', 'rest' o 'loop') pero se encontró " +
                      describe(current_));
    }
}

std::unique_ptr<Tempo> Parser::parseTempo() {
    advance();  // 'tempo'
    const Token bpmToken = expect(TokenType::Number, "un valor de BPM después de 'tempo'");
    const int bpm = requireInteger(bpmToken, "después de 'tempo'");
    expect(TokenType::Semicolon, "';' al final de 'tempo'");
    return std::make_unique<Tempo>(bpm);
}

std::unique_ptr<Play> Parser::parsePlay() {
    advance();  // 'play'
    const Token note =
        expect(TokenType::Identifier, "un nombre de nota después de 'play' (por ejemplo, C4)");
    expect(TokenType::Comma, "',' después del nombre de nota");
    const Token duration = expect(TokenType::Number, "una duración después de ','");
    expect(TokenType::Semicolon, "';' al final de 'play'");
    return std::make_unique<Play>(note.lexeme, static_cast<float>(duration.value));
}

std::unique_ptr<Rest> Parser::parseRest() {
    advance();  // 'rest'
    const Token duration = expect(TokenType::Number, "una duración después de 'rest'");
    expect(TokenType::Semicolon, "';' al final de 'rest'");
    return std::make_unique<Rest>(static_cast<float>(duration.value));
}

std::unique_ptr<Loop> Parser::parseLoop() {
    advance();  // 'loop'
    const Token repetitions =
        expect(TokenType::Number, "un número de repeticiones después de 'loop'");
    const int count = requireInteger(repetitions, "después de 'loop'");
    expect(TokenType::LBrace, "'{' para abrir el bloque de 'loop'");

    NodeList body = parseBlockBody();
    expect(TokenType::RBrace, "'}' para cerrar el bloque de 'loop'");
    return std::make_unique<Loop>(count, std::move(body));
}

NodeList Parser::parseBlockBody() {
    NodeList statements;
    while (current_.type != TokenType::RBrace) {
        if (current_.type == TokenType::EndOfInput) {
            error(current_, "se esperaba '}' para cerrar el bloque abierto con '{'");
        }
        statements.push_back(parseStatement());
    }
    return statements;
}

int Parser::requireInteger(const Token& tok, const std::string& what) {
    if (!tok.isInteger()) {
        error(tok, "se esperaba un número entero " + what + " pero se encontró el número '" +
                       tok.lexeme + "'");
    }
    if (tok.value > static_cast<double>(std::numeric_limits<int>::max())) {
        error(tok, "el número '" + tok.lexeme + "' no cabe en un entero");
    }
    return static_cast<int>(tok.value);
}

std::unique_ptr<Program> parse(const std::string& source) {
    Parser parser(source);
    return parser.parseProgram();
}

}  // namespace dsl
