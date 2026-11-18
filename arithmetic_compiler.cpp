/*
 * Simple Arithmetic Compiler in C++
 * =================================================================
 * Supported Operations:
 *   - Addition (+)
 *   - Subtraction (-)
 *   - Multiplication (*)
 *   - Division (/)
 *   - Parentheses ( ... ) and Unary +/- (e.g., -5 + 3)
 *   - Integer and floating-point numbers
 *
 * Compiler Pipeline Stages Demonstrated:
 *   1. Lexical Analysis (Scanner / Tokenizer)
 *   2. Syntax Analysis (Recursive Descent Parser -> Abstract Syntax Tree)
 *   3. Intermediate Code Generation (Three-Address Code / TAC)
 *   4. Target Code Generation (Stack-Machine Assembly Instructions)
 *   5. Execution (Stack Virtual Machine Evaluator)
 *
 * How to Compile & Run (GCC / MinGW / Terminal):
 *   g++ -std=c++11 arithmetic_compiler.cpp -o arithmetic_compiler
 *   ./arithmetic_compiler      (Linux / macOS)
 *   arithmetic_compiler.exe    (Windows)
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <stdexcept>

// ============================================================================
// STAGE 1: LEXICAL ANALYZER (TOKENIZER / SCANNER)
// Breaks raw source text like "3 + 4 * (10 - 2)" into meaningful tokens.
// ============================================================================

enum class TokenType {
    NUMBER,
    PLUS,
    MINUS,
    MUL,
    DIV,
    LPAREN,
    RPAREN,
    END_OF_FILE
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::MUL:         return "MUL";
        case TokenType::DIV:         return "DIV";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}

struct Token {
    TokenType type;
    std::string lexeme;
    double numValue;
    size_t position;

    Token(TokenType t, const std::string& lex, double val, size_t pos)
        : type(t), lexeme(lex), numValue(val), position(pos) {}
};

class Lexer {
private:
    std::string text;
    size_t pos;

public:
    explicit Lexer(const std::string& input) : text(input), pos(0) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (pos < text.length()) {
            char current = text[pos];

            if (std::isspace(static_cast<unsigned char>(current))) {
                pos++;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(current)) || current == '.') {
                tokens.push_back(readNumber());
                continue;
            }

            size_t startPos = pos;
            switch (current) {
                case '+':
                    tokens.emplace_back(TokenType::PLUS, "+", 0.0, startPos);
                    break;
                case '-':
                    tokens.emplace_back(TokenType::MINUS, "-", 0.0, startPos);
                    break;
                case '*':
                    tokens.emplace_back(TokenType::MUL, "*", 0.0, startPos);
                    break;
                case '/':
                    tokens.emplace_back(TokenType::DIV, "/", 0.0, startPos);
                    break;
                case '(':
                    tokens.emplace_back(TokenType::LPAREN, "(", 0.0, startPos);
                    break;
                case ')':
                    tokens.emplace_back(TokenType::RPAREN, ")", 0.0, startPos);
                    break;
                default: {
                    std::ostringstream oss;
                    oss << "Lexical Error: Unexpected character '" << current
                        << "' at position " << startPos;
                    throw std::runtime_error(oss.str());
                }
            }
            pos++;
        }

        tokens.emplace_back(TokenType::END_OF_FILE, "", 0.0, pos);
        return tokens;
    }

private:
    Token readNumber() {
        size_t startPos = pos;
        std::string numStr;
        int dotCount = 0;

        while (pos < text.length() &&
               (std::isdigit(static_cast<unsigned char>(text[pos])) || text[pos] == '.')) {
            if (text[pos] == '.') {
                dotCount++;
                if (dotCount > 1) {
                    std::ostringstream oss;
                    oss << "Lexical Error: Multiple decimal points in number at position " << pos;
                    throw std::runtime_error(oss.str());
                }
            }
            numStr += text[pos];
            pos++;
        }

        if (numStr == ".") {
            std::ostringstream oss;
            oss << "Lexical Error: Standalone decimal point at position " << startPos;
            throw std::runtime_error(oss.str());
        }

        double val = std::stod(numStr);
        return Token(TokenType::NUMBER, numStr, val, startPos);
    }
};

// ============================================================================
// STAGE 2: SYNTAX ANALYZER (PARSER & ABSTRACT SYNTAX TREE)
// Grammar (enforces standard PEMDAS / BODMAS operator precedence):
//   expr   -> term ((PLUS | MINUS) term)*
//   term   -> factor ((MUL | DIV) factor)*
//   factor -> (PLUS | MINUS) factor | NUMBER | LPAREN expr RPAREN
// ============================================================================

enum class NodeKind {
    NUMBER_NODE,
    UNARY_OP_NODE,
    BIN_OP_NODE
};

struct ASTNode {
    NodeKind kind;
    double numberValue;
    std::string op;
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;

    // Constructor for a Number leaf node
    explicit ASTNode(double val)
        : kind(NodeKind::NUMBER_NODE), numberValue(val), op(""), left(nullptr), right(nullptr) {}

    // Constructor for a Unary operator node (+x or -x)
    ASTNode(const std::string& unaryOp, std::shared_ptr<ASTNode> operand)
        : kind(NodeKind::UNARY_OP_NODE), numberValue(0.0), op(unaryOp), left(operand), right(nullptr) {}

    // Constructor for a Binary operator node (left op right)
    ASTNode(std::shared_ptr<ASTNode> l, const std::string& binOp, std::shared_ptr<ASTNode> r)
        : kind(NodeKind::BIN_OP_NODE), numberValue(0.0), op(binOp), left(l), right(r) {}
};

std::string formatNumber(double val) {
    if (std::floor(val) == val) {
        std::ostringstream oss;
        oss << static_cast<long long>(val);
        return oss.str();
    }
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;

    const Token& currentToken() const {
        return tokens[pos];
    }

    Token eat(TokenType expected) {
        if (currentToken().type == expected) {
            Token consumed = currentToken();
            if (pos + 1 < tokens.size()) {
                pos++;
            }
            return consumed;
        }
        std::ostringstream oss;
        oss << "Syntax Error at position " << currentToken().position
            << ": Expected " << tokenTypeToString(expected)
            << ", but got " << tokenTypeToString(currentToken().type);
        throw std::runtime_error(oss.str());
    }

    std::shared_ptr<ASTNode> factor() {
        Token tok = currentToken();

        if (tok.type == TokenType::PLUS || tok.type == TokenType::MINUS) {
            eat(tok.type);
            return std::make_shared<ASTNode>(tok.lexeme, factor());
        }

        if (tok.type == TokenType::NUMBER) {
            eat(TokenType::NUMBER);
            return std::make_shared<ASTNode>(tok.numValue);
        }

        if (tok.type == TokenType::LPAREN) {
            eat(TokenType::LPAREN);
            std::shared_ptr<ASTNode> node = expr();
            eat(TokenType::RPAREN);
            return node;
        }

        std::ostringstream oss;
        oss << "Syntax Error at position " << tok.position
            << ": Unexpected token " << tokenTypeToString(tok.type);
        throw std::runtime_error(oss.str());
    }

    std::shared_ptr<ASTNode> term() {
        std::shared_ptr<ASTNode> node = factor();

        while (currentToken().type == TokenType::MUL ||
               currentToken().type == TokenType::DIV) {
            Token opTok = eat(currentToken().type);
            node = std::make_shared<ASTNode>(node, opTok.lexeme, factor());
        }

        return node;
    }

    std::shared_ptr<ASTNode> expr() {
        std::shared_ptr<ASTNode> node = term();

        while (currentToken().type == TokenType::PLUS ||
               currentToken().type == TokenType::MINUS) {
            Token opTok = eat(currentToken().type);
            node = std::make_shared<ASTNode>(node, opTok.lexeme, term());
        }

        return node;
    }

public:
    explicit Parser(const std::vector<Token>& tokenList) : tokens(tokenList), pos(0) {}

    std::shared_ptr<ASTNode> parse() {
        std::shared_ptr<ASTNode> root = expr();
        if (currentToken().type != TokenType::END_OF_FILE) {
            std::ostringstream oss;
            oss << "Syntax Error at position " << currentToken().position
                << ": Unexpected trailing token '" << currentToken().lexeme << "'";
            throw std::runtime_error(oss.str());
        }
        return root;
    }
};

void printAST(const std::shared_ptr<ASTNode>& node, const std::string& indent = "", bool isLast = true) {
    if (!node) return;

    std::string marker = isLast ? "`-- " : "|-- ";
    std::string nextIndent = indent + (isLast ? "    " : "|   ");

    if (node->kind == NodeKind::NUMBER_NODE) {
        std::cout << indent << marker << "Number(" << formatNumber(node->numberValue) << ")\n";
    } else if (node->kind == NodeKind::UNARY_OP_NODE) {
        std::cout << indent << marker << "UnaryOp(" << node->op << ")\n";
        printAST(node->left, nextIndent, true);
    } else if (node->kind == NodeKind::BIN_OP_NODE) {
        std::cout << indent << marker << "BinOp(" << node->op << ")\n";
        printAST(node->left, nextIndent, false);
        printAST(node->right, nextIndent, true);
    }
}

// ============================================================================
// STAGE 3: INTERMEDIATE CODE GENERATOR (THREE-ADDRESS CODE / TAC)
// Converts the AST into Three-Address Code instructions (t1 = a op b).
// ============================================================================

class TACGenerator {
private:
    std::vector<std::string> instructions;
    int tempCounter = 0;

    std::string newTemp() {
        tempCounter++;
        return "t" + std::to_string(tempCounter);
    }

public:
    std::string generate(const std::shared_ptr<ASTNode>& node) {
        if (node->kind == NodeKind::NUMBER_NODE) {
            return formatNumber(node->numberValue);
        }

        if (node->kind == NodeKind::UNARY_OP_NODE) {
            std::string operandPlace = generate(node->left);
            std::string temp = newTemp();
            instructions.push_back(temp + " = " + node->op + operandPlace);
            return temp;
        }

        if (node->kind == NodeKind::BIN_OP_NODE) {
            std::string leftPlace = generate(node->left);
            std::string rightPlace = generate(node->right);
            std::string temp = newTemp();
            instructions.push_back(temp + " = " + leftPlace + " " + node->op + " " + rightPlace);
            return temp;
        }

        return "";
    }

    const std::vector<std::string>& getInstructions() const {
        return instructions;
    }
};

// ============================================================================
// STAGE 4: TARGET CODE GENERATOR (STACK MACHINE ASSEMBLY / BYTECODE)
// Translates the AST into low-level stack assembly instructions.
// ============================================================================

struct Instruction {
    std::string opcode;
    double operand;
    bool hasOperand;

    Instruction(const std::string& op, double val = 0.0, bool hasArg = false)
        : opcode(op), operand(val), hasOperand(hasArg) {}
};

class AssemblyGenerator {
private:
    std::vector<Instruction> bytecode;

    void visit(const std::shared_ptr<ASTNode>& node) {
        if (node->kind == NodeKind::NUMBER_NODE) {
            bytecode.emplace_back("PUSH", node->numberValue, true);
        } else if (node->kind == NodeKind::UNARY_OP_NODE) {
            visit(node->left);
            if (node->op == "-") {
                bytecode.emplace_back("NEG");
            }
        } else if (node->kind == NodeKind::BIN_OP_NODE) {
            visit(node->left);
            visit(node->right);
            if (node->op == "+")      bytecode.emplace_back("ADD");
            else if (node->op == "-") bytecode.emplace_back("SUB");
            else if (node->op == "*") bytecode.emplace_back("MUL");
            else if (node->op == "/") bytecode.emplace_back("DIV");
        }
    }

public:
    std::vector<Instruction> generate(const std::shared_ptr<ASTNode>& root) {
        bytecode.clear();
        visit(root);
        return bytecode;
    }
};

// ============================================================================
// STAGE 5: STACK VIRTUAL MACHINE (EXECUTES GENERATED ASSEMBLY)
// ============================================================================

class StackVM {
public:
    static double execute(const std::vector<Instruction>& instructions) {
        std::vector<double> stack;

        for (const auto& instr : instructions) {
            if (instr.opcode == "PUSH") {
                stack.push_back(instr.operand);
            } else if (instr.opcode == "NEG") {
                double val = stack.back();
                stack.pop_back();
                stack.push_back(-val);
            } else {
                double right = stack.back();
                stack.pop_back();
                double left = stack.back();
                stack.pop_back();

                if (instr.opcode == "ADD") {
                    stack.push_back(left + right);
                } else if (instr.opcode == "SUB") {
                    stack.push_back(left - right);
                } else if (instr.opcode == "MUL") {
                    stack.push_back(left * right);
                } else if (instr.opcode == "DIV") {
                    if (right == 0.0) {
                        throw std::runtime_error("Runtime Error: Division by zero");
                    }
                    stack.push_back(left / right);
                }
            }
        }

        return stack.empty() ? 0.0 : stack.back();
    }
};

// ============================================================================
// COMPILER DRIVER & INTERACTIVE CLI
// ============================================================================

void compileAndRun(const std::string& sourceCode) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "SOURCE EXPRESSION: " << sourceCode << "\n";
    std::cout << std::string(60, '=') << "\n";

    // Phase 1: Lexical Analysis
    Lexer lexer(sourceCode);
    std::vector<Token> tokens = lexer.tokenize();
    std::cout << "\n[Phase 1] Lexical Analysis (Tokens):\n";
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::NUMBER) {
            std::cout << "  Token(NUMBER, " << formatNumber(tok.numValue) << ")\n";
        } else if (tok.type == TokenType::END_OF_FILE) {
            std::cout << "  Token(EOF)\n";
        } else {
            std::cout << "  Token(" << tokenTypeToString(tok.type) << ", '" << tok.lexeme << "')\n";
        }
    }

    // Phase 2: Syntax Analysis (Parsing)
    Parser parser(tokens);
    std::shared_ptr<ASTNode> ast = parser.parse();
    std::cout << "\n[Phase 2] Syntax Analysis (Abstract Syntax Tree):\n";
    printAST(ast);

    // Phase 3: Intermediate Code Generation (Three-Address Code)
    TACGenerator tacGen;
    std::string finalTemp = tacGen.generate(ast);
    std::cout << "\n[Phase 3] Intermediate Code Generation (Three-Address Code):\n";
    const auto& tacLines = tacGen.getInstructions();
    if (!tacLines.empty()) {
        for (size_t i = 0; i < tacLines.size(); ++i) {
            std::cout << "  " << std::setw(2) << std::setfill('0') << (i + 1)
                      << std::setfill(' ') << ": " << tacLines[i] << "\n";
        }
        std::cout << "  Result held in: " << finalTemp << "\n";
    } else {
        std::cout << "  01: t1 = " << finalTemp << "\n";
    }

    // Phase 4: Target Assembly Generation
    AssemblyGenerator asmGen;
    std::vector<Instruction> bytecode = asmGen.generate(ast);
    std::cout << "\n[Phase 4] Target Code Generation (Stack Assembly):\n";
    for (size_t i = 0; i < bytecode.size(); ++i) {
        std::cout << "  " << std::setw(2) << std::setfill('0') << (i + 1)
                  << std::setfill(' ') << ": " << bytecode[i].opcode;
        if (bytecode[i].hasOperand) {
            std::cout << " " << formatNumber(bytecode[i].operand);
        }
        std::cout << "\n";
    }

    // Phase 5: Execution on Stack Virtual Machine
    double result = StackVM::execute(bytecode);
    std::cout << "\n[Phase 5] Final Answer: " << formatNumber(result) << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int main() {
    std::cout << "=== Simple Arithmetic Compiler (C++) ===\n";
    std::cout << "Supported operators: +  -  *  /  and parentheses ( )\n";
    std::cout << "Type an expression and press Enter (or type 'q' to quit).\n\n";

    std::string userInput;
    while (true) {
        std::cout << "Enter expression > " << std::flush;
        if (!std::getline(std::cin, userInput)) {
            break;
        }

        // Trim leading/trailing whitespace
        size_t first = userInput.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = userInput.find_last_not_of(" \t\r\n");
        std::string trimmed = userInput.substr(first, (last - first + 1));

        if (trimmed == "q" || trimmed == "Q" || trimmed == "quit" || trimmed == "exit") {
            std::cout << "Exiting compiler.\n";
            break;
        }

        try {
            compileAndRun(trimmed);
        } catch (const std::exception& e) {
            std::cout << "\n" << e.what() << "\n\n";
        }
    }

    return 0;
}