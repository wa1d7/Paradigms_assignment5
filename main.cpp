#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <algorithm>
#include <unordered_map>

enum class TokenType {
    Number, Plus, Minus, Multiply, Divide, LParen, RParen, EndOfFile,
    Identifier, Comma, Var, Equals, Def, LBrace, RBrace
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
            if (idStr == "var") return {TokenType::Var, 0, "var"};
            if (idStr == "def") return {TokenType::Def, 0, "def"};
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
            case '{': get(); return {TokenType::LBrace, 0, "{"};
            case '}': get(); return {TokenType::RBrace, 0, "}"};
            case ',': get(); return {TokenType::Comma, 0, ","};
            case '=': get(); return {TokenType::Equals, 0, "="};
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
class Environment;


class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual double evaluate(Environment& env) const = 0;

    virtual std::string getName() const { return ""; }
};

struct FunctionDef {
    std::vector<std::string> params;
    std::shared_ptr<ASTNode> body;
};


class Environment {
    std::unordered_map<std::string, double> vars;
    std::unordered_map<std::string, FunctionDef> funcs;
    Environment* parent;
public:
    Environment(Environment* parent = nullptr) : parent(parent) {}

    void defineVar(const std::string& name, double value) {
        if (vars.find(name) != vars.end()) {
            throw std::runtime_error("variable already declared: " + name);
        }
        vars[name] = value;
    }

    double getVar(const std::string& name) const {
        std::unordered_map<std::string, double>::const_iterator it = vars.find(name);
        if (it != vars.end()) return it->second;
        if (parent != nullptr) return parent->getVar(name);
        throw std::runtime_error("undefined variable: " + name);
    }

    void defineFunc(const std::string& name, const FunctionDef& def) {
        funcs[name] = def;
    }

    FunctionDef getFunc(const std::string& name) const {
        std::unordered_map<std::string, FunctionDef>::const_iterator it = funcs.find(name);
        if (it != funcs.end()) return it->second;
        if (parent != nullptr) return parent->getFunc(name);
        throw std::runtime_error("undefined function: " + name);
    }
};

class NumberNode : public ASTNode {
    double val;
public:
    NumberNode(double v) : val(v) {}
    double evaluate(Environment& env) const override { return val; }
};

class VariableNode : public ASTNode {
    std::string name;
public:
    VariableNode(const std::string& name) : name(name) {}

    double evaluate(Environment& env) const override {
        return env.getVar(name);
    }

    std::string getName() const override {
        return name;
    }
};

class VarDeclNode : public ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> expr;
public:
    VarDeclNode(const std::string& name, std::unique_ptr<ASTNode> expr)
        : name(name), expr(std::move(expr)) {}

    double evaluate(Environment& env) const override {
        double value = expr->evaluate(env);
        env.defineVar(name, value);
        return value;
    }
};

class BinaryOpNode : public ASTNode {
    TokenType op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
public:
    BinaryOpNode(TokenType op, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(op), left(std::move(l)), right(std::move(r)) {}

    double evaluate(Environment& env) const override {
        double lval = left->evaluate(env);
        double rval = right->evaluate(env);

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

class FunctionCallNode : public ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> args;
public:
    FunctionCallNode(const std::string& name, std::vector<std::unique_ptr<ASTNode>> args)
        : name(name), args(std::move(args)) {}

    double evaluate(Environment& env) const override {

        if (name == "pow") {
            if (args.size() != 2) throw std::runtime_error("pow takes 2 arguments");
            return std::pow(args[0]->evaluate(env), args[1]->evaluate(env));
        } else if (name == "abs") {
            if (args.size() != 1) throw std::runtime_error("abs takes 1 argument");
            return std::abs(args[0]->evaluate(env));
        } else if (name == "max") {
            if (args.size() != 2) throw std::runtime_error("max takes 2 arguments");
            return std::max(args[0]->evaluate(env), args[1]->evaluate(env));
        } else if (name == "min") {
            if (args.size() != 2) throw std::runtime_error("min takes 2 arguments");
            return std::min(args[0]->evaluate(env), args[1]->evaluate(env));
        }

        else if (name == "sum" || name == "integral") {
            if (args.size() != 3) throw std::runtime_error(name + " takes 3 arguments");

            std::string funcName = args[0]->getName();
            if (funcName.empty()) throw std::runtime_error("first argument must be a function name");

            FunctionDef def = env.getFunc(funcName);
            if (def.params.size() != 1) throw std::runtime_error(name + " requires a 1-argument function");

            double a = args[1]->evaluate(env);
            double b = args[2]->evaluate(env);

            auto evalFunc = [&](double x) {
                Environment localEnv(&env);
                localEnv.defineVar(def.params[0], x);
                return def.body->evaluate(localEnv);
            };

            if (name == "sum") {
                double total = 0.0;
                for (double i = a; i <= b; i += 1.0) {
                    total += evalFunc(i);
                }
                return total;
            } else {
                int n = 1000;
                double dx = (b - a) / n;
                double total = (evalFunc(a) + evalFunc(b)) / 2.0;

                for (int i = 1; i < n; ++i) {
                    total += evalFunc(a + i * dx);
                }
                return total * dx;
            }
        }

        FunctionDef def = env.getFunc(name);
        if (args.size() != def.params.size()) {
            throw std::runtime_error("argument count mismatch for function: " + name);
        }

        Environment localEnv(&env);
        for (size_t i = 0; i < args.size(); ++i) {
            double argVal = args[i]->evaluate(env);
            localEnv.defineVar(def.params[i], argVal);
        }

        return def.body->evaluate(localEnv);
    }
};

class FuncDeclNode : public ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<ASTNode> body;
public:
    FuncDeclNode(const std::string& name, std::vector<std::string> params, std::shared_ptr<ASTNode> body)
        : name(name), params(std::move(params)), body(std::move(body)) {}

    double evaluate(Environment& env) const override {
        FunctionDef def;
        def.params = params;
        def.body = body;
        env.defineFunc(name, def);
        return 0.0;
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
                return std::make_unique<FunctionCallNode>(name, std::move(args));
            }

            return std::make_unique<VariableNode>(name);

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

        std::unique_ptr<ASTNode> ast;

        if (current().type == TokenType::Var) {
            consume(TokenType::Var);
            if (current().type != TokenType::Identifier) {
                throw std::runtime_error("expected identifier after 'var'");
            }
            std::string varName = current().text;
            consume(TokenType::Identifier);
            consume(TokenType::Equals);
            std::unique_ptr<ASTNode> expr = parseExpr();
            ast = std::make_unique<VarDeclNode>(varName, std::move(expr));
        } else if (current().type == TokenType::Def) {
            consume(TokenType::Def);
            if (current().type != TokenType::Identifier) {
                throw std::runtime_error("expected function name");
            }
            std::string funcName = current().text;
            consume(TokenType::Identifier);

            consume(TokenType::LParen);
            std::vector<std::string> params;
            if (current().type != TokenType::RParen) {
                if (current().type != TokenType::Identifier) throw std::runtime_error("expected parameter name");
                params.push_back(current().text);
                consume(TokenType::Identifier);
                while (current().type == TokenType::Comma) {
                    consume(TokenType::Comma);
                    if (current().type != TokenType::Identifier) throw std::runtime_error("expected parameter name");
                    params.push_back(current().text);
                    consume(TokenType::Identifier);
                }
            }
            consume(TokenType::RParen);

            consume(TokenType::LBrace);
            std::shared_ptr<ASTNode> body = parseExpr();
            consume(TokenType::RBrace);

            ast = std::make_unique<FuncDeclNode>(funcName, std::move(params), std::move(body));
        } else {
            ast = parseExpr();
        }

        if (current().type != TokenType::EndOfFile) {
            throw std::runtime_error("syntax error at the end");
        }
        return ast;
    }
};

int main() {
    std::string input;
    Environment env;

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
                double result = ast->evaluate(env);

                VarDeclNode* varDecl = dynamic_cast<VarDeclNode*>(ast.get());
                FuncDeclNode* funcDecl = dynamic_cast<FuncDeclNode*>(ast.get());

                if (varDecl == nullptr && funcDecl == nullptr) {
                    std::cout << result << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "error: " << e.what() << std::endl;
        }
    }

    return 0;
}