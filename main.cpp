#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "include/Scanner.h"
#include "include/Parser.h"
#include "include/Resolver.h"

using namespace std;
using namespace token;

static auto interp = make_shared<interpreter::Interpreter>();

void run(string source);
void run_file(const char *path);
void run_prompt();

int main(int argc, char **argv) {
    if (argc > 2) {
        cout << "Usage: mac [script]" << endl;
        return 1;
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
    }

    parser::Parser parser(tokens);
    auto statements = parser.parse<interpreter::MacValue>();
    if (statements.empty()) return;

    auto resolverInstance = make_shared<resolver::Resolver>(interp);
    resolverInstance->resolve(statements);

    interp->interpret(statements);
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
        if (!getline(cin, line)) break;
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line == "exit") break;
        run(line);
    } while (true);
}
