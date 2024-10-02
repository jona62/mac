#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "include/Scanner.h"
#include "include/Parser.h"
#include "include/AstPrinter.h"

using namespace std;
using namespace token;
using namespace expr;

void run(string source);

void run_file(const char *path);

void run_prompt();

int main(int argc, char **argv) {
    if (argc > 2) {
        cout << "Usage: mac [script]" << endl;
        system("exit(0)");
    } else if (argc == 2) {
        run_file(argv[1]);
        return 0;
    } else {
        run_prompt();
        return 0;
    }
}

void run(string source) {

    scanner::Scanner scanner(source);
    vector<Token> tokens;
    for (auto& token : scanner) {
        tokens.push_back(token);
        token.print();
    }

    parser::Parser parser(tokens);
    parser.parse();

    Token minusToken(TokenType::MINUS, std::make_shared<TokenValue>("-"), 1);
    Token starToken(TokenType::STAR, std::make_shared<TokenValue>("*"), 1);

    // Create the individual expressions
    auto stringLiteral = make_shared<expr::Literal<string>>("String literal");
    auto literal45_67 = make_shared<expr::Literal<string>>(123.45);
    auto unaryExpr = make_shared<expr::Unary<string>>(minusToken, literal45_67);
    auto groupingExpr = make_shared<expr::Grouping<string>>(stringLiteral);

    // Create the full expression
    auto expression = make_shared<expr::Binary<string>>(unaryExpr, starToken, groupingExpr);
    auto grouping = make_shared<expr::Grouping<string>>(expression);
    auto expression2 = make_shared<expr::Binary<string>>(grouping, starToken, literal45_67);

    // Print the AST
    auto printer = make_shared<printer::AstPrinter<string>>();
    cout << expression->visit(printer) << endl;
    cout << expression2->visit(printer) << endl;
}

void run_file(const char *path) {
    ifstream source_file(path);
    if (!source_file.is_open()) {
        cout << "Could not open file for reading: " << path << endl;
        return;
    }
    string buffer;
    ostringstream output_stream;

    while (getline(source_file, buffer)) {
        output_stream << buffer << '\n';
    }
    source_file.close();

    run(output_stream.str());
}

void run_prompt() {
    do {
        cout << "|> ";
        string line;
        getline(cin, line);
        // strip trailing spaces
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line == "exit") break;
        run(line);
    } while (true);
}