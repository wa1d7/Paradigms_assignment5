#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <memory>
enum class TokenType {
    Number, Plus, Minus, Multiply, Divide, LParen, RParen, EndOfFile
};

struct Token {
    TokenType type;
    double value = 0.0;
    std::string text = "";
};

class Lexer {
    std::string input;
    size_t pos;

    char peek() const {
        if (pos >= input.length()) return '\0';
        return input[pos];
    }

    char get() {
        if (pos >= input.length()) return '\0';
        return input[pos++];
    }

public:
    Lexer(const std::string& input) : input(input), pos(0) {}

    Token nextToken() {
        while (std::isspace(peek())) get();

        char c = peek();
        if (c == '\0') return {TokenType::EndOfFile};

        if (std::isdigit(c) || c == '.') {
            std::string numStr;
            while (std::isdigit(peek()) || peek() == '.') {
                numStr += get();
            }
            return {TokenType::Number, std::stod(numStr), numStr};
        }

        switch (c) {
            case '+': get(); return {TokenType::Plus, 0, "+"};
            case '-': get(); return {TokenType::Minus, 0, "-"};
            case '*': get(); return {TokenType::Multiply, 0, "*"};
            case '/': get(); return {TokenType::Divide, 0, "/"};
            case '(': get(); return {TokenType::LParen, 0, "("};
            case ')': get(); return {TokenType::RParen, 0, ")"};
            default:
                throw std::runtime_error(std::string("unknown character: ") + c);
        }
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        Token t = nextToken();
        while (t.type != TokenType::EndOfFile) {
            tokens.push_back(t);
            t = nextToken();
        }
        tokens.push_back({TokenType::EndOfFile});
        return tokens;
    }
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual double evaluate() const = 0;
};

class NumberNode : public ASTNode {
    double val;
public:
    NumberNode(double v) : val(v) {}
    double evaluate() const override { return val; }
};

class BinaryOpNode : public ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
public:
    BinaryOpNode(TokenType op, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(op), left(std::move(l)), right(std::move(r)) {}

    double evaluate() const override {
        double lval = left->evaluate();
        double rval = right->evaluate();

        switch (op) {
            case TokenType::Plus: return lval + rval;
            case TokenType::Minus: return lval - rval;
            case TokenType::Multiply: return lval * rval;
            case TokenType::Divide:
                if (rval == 0.0) throw std::runtime_error("division by zero");
                return lval / rval;
            default:
                throw std::runtime_error("invalid operator");
        }
    }
};
int main() {
    std::string input;

    while (std::getline(std::cin, input)) {
        if (input == "exit" || input == "quit") {
            break;
        }
        if (input.empty()) {
            continue;
        }

        // TODO
    }

    return 0;
}