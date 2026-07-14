#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <algorithm>

enum class TokenType {
    Number, Plus, Minus, Multiply, Divide, LParen, RParen, EndOfFile,
    Identifier, Comma
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

        if (std::isalpha(c)) {
            std::string idStr;
            while (std::isalnum(peek()) || peek() == '_') {
                idStr += get();
            }
            return {TokenType::Identifier, 0, idStr};
        }

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
            case ',': get(); return {TokenType::Comma, 0, ","};
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
class BuiltinFuncNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
public:
    BuiltinFuncNode(const std::string& name, std::vector<std::unique_ptr<ASTNode>> args)
        : name(name), args(std::move(args)) {}

    double evaluate() const override {
        if (name == "pow") {
            if (args.size() != 2) throw std::runtime_error("pow takes 2 arguments");
            return std::pow(args[0]->evaluate(), args[1]->evaluate());
        } else if (name == "abs") {
            if (args.size() != 1) throw std::runtime_error("abs takes 1 argument");
            return std::abs(args[0]->evaluate());
        } else if (name == "max") {
            if (args.size() != 2) throw std::runtime_error("max takes 2 arguments");
            return std::max(args[0]->evaluate(), args[1]->evaluate());
        } else if (name == "min") {
            if (args.size() != 2) throw std::runtime_error("min takes 2 arguments");
            return std::min(args[0]->evaluate(), args[1]->evaluate());
        }
        throw std::runtime_error("unknown built-in function: " + name);
    }
};
class Parser {
    std::vector<Token> tokens;
    size_t pos;

    const Token& current() const { return tokens[pos]; }

    void consume(TokenType type) {
        if (current().type == type) pos++;
        else throw std::runtime_error("unexpected token");
    }

    std::unique_ptr<ASTNode> parseFactor() {
        if (current().type == TokenType::Number) {
            double val = current().value;
            consume(TokenType::Number);
            return std::make_unique<NumberNode>(val);
        } else if (current().type == TokenType::Identifier) {
            std::string name = current().text;
            consume(TokenType::Identifier);

            if (current().type == TokenType::LParen) {
                consume(TokenType::LParen);
                std::vector<std::unique_ptr<ASTNode>> args;
                if (current().type != TokenType::RParen) {
                    args.push_back(parseExpr());
                    while (current().type == TokenType::Comma) {
                        consume(TokenType::Comma);
                        args.push_back(parseExpr());
                    }
                }
                consume(TokenType::RParen);
                return std::make_unique<BuiltinFuncNode>(name, std::move(args));
            }
            throw std::runtime_error("unexpected identifier: " + name);
        } else if (current().type == TokenType::LParen) {
            consume(TokenType::LParen);
            std::unique_ptr<ASTNode> node = parseExpr();
            consume(TokenType::RParen);
            return node;
        } else if (current().type == TokenType::Minus) {
            consume(TokenType::Minus);
            std::unique_ptr<ASTNode> node = parseFactor();
            return std::make_unique<BinaryOpNode>(
                TokenType::Minus,
                std::make_unique<NumberNode>(0.0),
                std::move(node)
            );
        }
        throw std::runtime_error("syntax error");
    }

    std::unique_ptr<ASTNode> parseTerm() {
        std::unique_ptr<ASTNode> node = parseFactor();
        while (current().type == TokenType::Multiply || current().type == TokenType::Divide) {
            TokenType op = current().type;
            consume(op);
            std::unique_ptr<ASTNode> right = parseFactor();
            node = std::make_unique<BinaryOpNode>(op, std::move(node), std::move(right));
        }
        return node;
    }

    std::unique_ptr<ASTNode> parseExpr() {
        std::unique_ptr<ASTNode> node = parseTerm();
        while (current().type == TokenType::Plus || current().type == TokenType::Minus) {
            TokenType op = current().type;
            consume(op);
            std::unique_ptr<ASTNode> right = parseTerm();
            node = std::make_unique<BinaryOpNode>(op, std::move(node), std::move(right));
        }
        return node;
    }

public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

    std::unique_ptr<ASTNode> parse() {
        if (current().type == TokenType::EndOfFile) return nullptr;
        std::unique_ptr<ASTNode> ast = parseExpr();
        if (current().type != TokenType::EndOfFile) {
            throw std::runtime_error("syntax error at the end");
        }
        return ast;
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

        try {
            Lexer lexer(input);
            std::vector<Token> tokens = lexer.tokenize();

            Parser parser(tokens);
            std::unique_ptr<ASTNode> ast = parser.parse();

            if (ast) {
                std::cout << ast->evaluate() << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "error: " << e.what() << std::endl;
        }
    }

    return 0;
}