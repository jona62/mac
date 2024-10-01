#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "include/Scanner.h"
#include "include/Parser.h"
#include "include/AstPrinter.h"

using namespace std;
using std::string;

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
    std::vector<token::Token> tokens;
    for (auto& token : scanner) {
        tokens.push_back(token);
        // token.print();
    }

    parser::Parser parser(tokens);
    parser.parse();

    token::Token minusToken(token::TokenType::MINUS, std::monostate {}, "-", 1);
    token::Token starToken(token::TokenType::STAR, std::monostate {}, "*", 1);

    // Create the individual expressions
    expr::Literal<string> *stringLiteral = new expr::Literal<string>("String literal");

    expr::Literal<string> *literal45_67 = new expr::Literal<string>(123.45);
    expr::Unary<string> *unaryExpr = new expr::Unary<string>(minusToken, literal45_67);
    expr::Grouping<string> *groupingExpr = new expr::Grouping<string>(stringLiteral);

    // Create the full expression
    expr::Expr<string>* expression = new expr::Binary<string>(unaryExpr, starToken, groupingExpr);
    expr::Grouping<string> *grouping = new expr::Grouping<string>(expression);
    expr::Expr<string>* expression2 = new expr::Binary<string>(grouping, starToken, literal45_67);

    // Print the AST
    printer::AstPrinter<string> *printer = new printer::AstPrinter<string>();
    cout << expression->visit(printer) << endl;
    cout << expression2->visit(printer) << endl;
    delete grouping;
    delete printer;
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