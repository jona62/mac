#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include "Scanner.h"
#include "Parser.h"
#include "Resolver.h"
#include "MacAnalyzer.h"
#include "MacMeme.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

using namespace std;
using namespace token;

// Resolve paths relative to the binary's own directory
// so assets/stdlib work regardless of where the binary is invoked from
static string binaryDir;

static string getBinaryDir() {
    std::filesystem::path exe;
#ifdef __APPLE__
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        exe = std::filesystem::canonical(buf);
    }
#elif defined(__linux__)
    exe = std::filesystem::canonical("/proc/self/exe");
#endif
    if (!exe.empty()) return exe.parent_path().string();
    return ".";
}

static string resolvePath(const string& relative) {
    // Try relative to binary first, then cwd
    auto binPath = binaryDir + "/" + relative;
    if (std::filesystem::exists(binPath)) return binPath;
    return relative; // fallback to cwd-relative
}

static auto interp = make_shared<interpreter::Interpreter>();

// Keep prelude AST alive so resolver entries (raw pointers) remain valid
static vector<shared_ptr<stmt::Stmt<interpreter::MacValue>>> preludeStatements;

void run(string source);
void run_file(const char *path);
void run_prompt();
void analyze_file(const char *path);

string readFile(const char *path) {
    ifstream f(path);
    if (!f.is_open()) return "";
    ostringstream ss;
    string buf;
    while (getline(f, buf)) ss << buf << '\n';
    return ss.str();
}

void loadPrelude() {
    string src = readFile(resolvePath("stdlib/prelude.mac").c_str());
    if (src.empty()) return;

    scanner::Scanner scanner(src);
    vector<Token> tokens;
    for (auto& token : scanner) tokens.push_back(token);

    parser::Parser parser(tokens);
    preludeStatements = parser.parse<interpreter::MacValue>();
    if (preludeStatements.empty()) return;

    auto resolverInstance = make_shared<resolver::Resolver>(interp);
    resolverInstance->resolve(preludeStatements);
    interp->interpret(preludeStatements);
}

int main(int argc, char **argv) {
    binaryDir = getBinaryDir();
    meme::MacMeme::binaryDir() = binaryDir;

    // --analyze mode: output JSON analysis for LSP
    if (argc == 3 && strcmp(argv[1], "--analyze") == 0) {
        analyze_file(argv[2]);
        return 0;
    }

    if (argc > 2) {
        cout << "Usage: mac [script]" << endl;
        cout << "       mac --analyze <file>" << endl;
        return 1;
    }

    loadPrelude();

    if (argc == 2) {
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

void analyze_file(const char *path) {
    string source = readFile(path);
    if (source.empty()) {
        cout << "{\"symbols\":[],\"references\":[],\"diagnostics\":[{\"line\":1,\"col\":1,\"endCol\":1,\"message\":\"Could not open file.\",\"severity\":\"error\",\"source\":\"user\"}],\"properties\":[],\"foldingRanges\":[],\"semanticTokens\":[],\"paramHints\":[],\"chainHints\":[],\"signatures\":[],\"classes\":[],\"templates\":[]}" << endl;
        return;
    }

    analyzer::MacAnalyzer macAnalyzer;

    string preludeSource = readFile(resolvePath("stdlib/prelude.mac").c_str());
    if (!preludeSource.empty()) {
        scanner::Scanner preludeScanner(preludeSource);
        vector<Token> preludeTokens;
        for (auto& token : preludeScanner) preludeTokens.push_back(token);

        parser::Parser preludeParser(preludeTokens);
        auto preludeStatements = preludeParser.parse<value::MacValue>();
        macAnalyzer.analyze(preludeStatements, "prelude");
    }

    scanner::Scanner scanner(source);
    vector<Token> tokens;
    for (auto& token : scanner) tokens.push_back(token);

    parser::Parser parser(tokens);
    auto statements = parser.parse<value::MacValue>();
    macAnalyzer.analyze(statements, "user");

    // Populate built-in templates for LSP completions
    for (auto& [name, path] : meme::MacMeme::templateMap()) {
        macAnalyzer.addTemplate(name, "", "Built-in template");
    }
    // Scan meme subdirectory for dotted templates
    auto memeDir = resolvePath("assets/templates/meme");
    if (std::filesystem::is_directory(memeDir)) {
        for (auto& entry : std::filesystem::directory_iterator(memeDir)) {
            if (!entry.is_regular_file()) continue;
            auto stem = entry.path().stem().string();
            macAnalyzer.addTemplate("meme." + stem, "meme", "Meme image");
        }
    }

    cout << macAnalyzer.toJson() << endl;
}
