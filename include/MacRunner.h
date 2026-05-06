#ifndef MAC_RUNNER_H
#define MAC_RUNNER_H

#include <filesystem>           // path, exists, canonical, is_directory
#include <fstream>              // ifstream (file reading)
#include <iostream>             // cout (output)
#include <sstream>              // ostringstream (source buffering)
#include <string>               // string
#include <unistd.h>             // isatty, fileno (interactive detection)
#include <vector>               // vector (tokens, AST)

#include "Scanner.h"            // scanner::Scanner (lexical analysis)
#include "Parser.h"             // parser::Parser (syntactic analysis)
#include "Resolver.h"           // resolver::Resolver (variable resolution)
#include "MacAnalyzer.h"        // analyzer::MacAnalyzer (semantic analysis for LSP)
#include "MacCatalog.h"         // mac_catalog::templateEntries (shared template metadata)
#include "NativeFunctions.h"    // callable::cleanupTempFiles
#include "nlohmann/json.hpp"    // nlohmann::json (analyzer error output)

extern "C" {
#include "linenoise.h"          // linenoise, linenoiseHistoryAdd/Save/Load (REPL editing)
}
#include "MacMeme.h"            // meme::MacMeme (scriptDir, binaryDir, templateMap)

#ifdef __APPLE__
#include <mach-o/dyld.h>        // _NSGetExecutablePath (binary location)
#endif

namespace runner {

    using MV = value::MacValue;

    // Resolve the binary's own directory for finding assets/stdlib
    inline std::string getBinaryDir() {
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

    // Try path relative to binary first, then cwd
    inline std::string resolvePath(const std::string& binaryDir, const std::string& relative) {
        auto binPath = binaryDir + "/" + relative;
        if (std::filesystem::exists(binPath)) return binPath;
        return relative;
    }

    // Read entire file to string
    inline std::string readFile(const char* path) {
        std::ifstream f(path);
        if (!f.is_open()) return "";
        std::ostringstream ss;
        std::string buf;
        while (std::getline(f, buf)) ss << buf << '\n';
        return ss.str();
    }

    // Persistent runtime state shared across REPL lines and file execution
    struct Runtime {
        std::shared_ptr<interpreter::Interpreter> interp =
            std::make_shared<interpreter::Interpreter>();
        std::shared_ptr<resolver::Resolver> resolver =
            std::make_shared<resolver::Resolver>(interp);

        // Keep AST alive so MacFunction raw pointers remain valid
        std::vector<std::shared_ptr<stmt::Stmt<MV>>> preludeAST;
        std::vector<std::vector<std::shared_ptr<stmt::Stmt<MV>>>> sessionAST;

        void run(const std::string& source) {
            scanner::Scanner scanner(source);
            std::vector<token::Token> tokens;
            for (auto& token : scanner) tokens.push_back(token);

            parser::Parser parser(tokens);
            auto statements = parser.parse<MV>();
            if (statements.empty()) return;

            resolver->resolve(statements);
            interp->interpret(statements);

            sessionAST.push_back(std::move(statements));
        }

        void loadPrelude(const std::string& binaryDir) {
            std::string src = readFile(resolvePath(binaryDir, "stdlib/prelude.mac").c_str());
            if (src.empty()) return;

            scanner::Scanner scanner(src);
            std::vector<token::Token> tokens;
            for (auto& token : scanner) tokens.push_back(token);

            parser::Parser parser(tokens);
            preludeAST = parser.parse<MV>();
            if (preludeAST.empty()) return;

            resolver->resolve(preludeAST);
            interp->interpret(preludeAST);
        }
    };

    // Run a .mac file
    inline void runFile(Runtime& rt, const char* path) {
        auto scriptDir = std::filesystem::path(path).parent_path().string();
        if (scriptDir.empty()) scriptDir = ".";
        meme::MacMeme::scriptDir() = scriptDir;

        std::string source = readFile(path);
        if (source.empty()) {
            std::cout << "Could not open file for reading: " << path << std::endl;
            return;
        }
        rt.run(source);
        callable::cleanupTempFiles();
        if (rt.interp->hadError) std::exit(1);
    }

    // Interactive REPL or piped stdin
    inline void runPrompt(Runtime& rt) {
        bool interactive = isatty(fileno(stdin));

        // Non-interactive (piped) mode: read all input as a batch
        if (!interactive) {
            std::ostringstream ss;
            std::string line;
            while (std::getline(std::cin, line)) {
                line.erase(line.find_last_not_of(" \n\r\t") + 1);
                if (line == "exit") break;
                ss << line << '\n';
            }
            auto src = ss.str();
            // Auto-append semicolon for simple expressions
            auto trimmed = src;
            while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) trimmed.pop_back();
            if (!trimmed.empty() && trimmed.back() != ';' && trimmed.back() != '}') src += ";";
            rt.run(src);
            if (rt.interp->hadError) std::exit(1);
            return;
        }

        // Interactive mode with linenoise — arrow keys, history, Ctrl-C
        std::string historyPath;
        if (const char* home = std::getenv("HOME")) {
            historyPath = std::string(home) + "/.mac_history";
            linenoiseHistoryLoad(historyPath.c_str());
        }
        linenoiseSetMultiLine(1);

        std::string buffer;
        int braceDepth = 0, bracketDepth = 0, parenDepth = 0;

        while (true) {
            bool continuation = braceDepth > 0 || bracketDepth > 0 || parenDepth > 0;
            const char* prompt = continuation ? ".. " : "|> ";

            char* raw = linenoise(prompt);
            if (!raw) break; // EOF or Ctrl-D

            std::string line(raw);
            linenoiseFree(raw);
            line.erase(line.find_last_not_of(" \n\r\t") + 1);

            if (line == "exit" && !continuation) break;
            if (line == "clear" && !continuation) {
                linenoiseClearScreen();
                continue;
            }
            if (line.empty() && !continuation) continue;

            // Add each line to history individually so up-arrow recalls one line at a time
            if (!line.empty()) linenoiseHistoryAdd(line.c_str());

            if (!buffer.empty()) buffer += "\n";
            buffer += line;

            // Track nesting depth for multi-line input
            bool inString = false;
            for (char c : line) {
                if (c == '"') inString = !inString;
                if (inString) continue;
                if (c == '{') braceDepth++;
                else if (c == '}') braceDepth--;
                else if (c == '[') bracketDepth++;
                else if (c == ']') bracketDepth--;
                else if (c == '(') parenDepth++;
                else if (c == ')') parenDepth--;
            }

            if (braceDepth <= 0 && bracketDepth <= 0 && parenDepth <= 0) {
                braceDepth = 0;
                bracketDepth = 0;
                parenDepth = 0;
                // Auto-append semicolon for simple expressions
                auto trimBuf = buffer;
                while (!trimBuf.empty() && std::isspace(static_cast<unsigned char>(trimBuf.back()))) trimBuf.pop_back();
                if (!trimBuf.empty() && trimBuf.back() != ';' && trimBuf.back() != '}') buffer += ";";
                rt.run(buffer);
                buffer.clear();
            }
        }

        // Save history
        if (!historyPath.empty()) {
            linenoiseHistorySetMaxLen(500);
            linenoiseHistorySave(historyPath.c_str());
        }
    }

    // Run --analyze mode for LSP
    inline void analyzeFile(const std::string& binaryDir, const char* path) {
        auto scriptDir = std::filesystem::path(path).parent_path().string();
        if (scriptDir.empty()) scriptDir = ".";
        meme::MacMeme::scriptDir() = scriptDir;

        std::string source = readFile(path);
        if (source.empty()) {
            analyzer::AnalysisResult empty;
            empty.diagnostics.push_back({1, 1, 1, "Could not open file.", "error", "user"});
            std::cout << analyzer::toJson(empty) << std::endl;
            return;
        }

        analyzer::MacAnalyzer macAnalyzer;

        std::string preludeSource = readFile(resolvePath(binaryDir, "stdlib/prelude.mac").c_str());
        if (!preludeSource.empty()) {
            scanner::Scanner preludeScanner(preludeSource);
            std::vector<token::Token> preludeTokens;
            for (auto& token : preludeScanner) preludeTokens.push_back(token);

            parser::Parser preludeParser(preludeTokens);
            auto preludeStmts = preludeParser.parse<MV>();
            macAnalyzer.analyze(preludeStmts, "prelude");
        }

        scanner::Scanner scanner(source);
        std::vector<token::Token> tokens;
        for (auto& token : scanner) tokens.push_back(token);

        parser::Parser parser(tokens);
        auto statements = parser.parse<MV>();
        macAnalyzer.analyze(statements, "user");

        for (const auto& tmpl : mac_catalog::templateEntries(binaryDir)) {
            macAnalyzer.addTemplate(
                tmpl.id,
                tmpl.category,
                tmpl.description,
                tmpl.bestFor,
                tmpl.captionGuidance,
                tmpl.tags,
                tmpl.moods,
                tmpl.subjects,
                tmpl.aliases,
                mac_catalog::textZonesToJson(tmpl.textZones)
            );
        }

        std::cout << macAnalyzer.toJson() << std::endl;
    }

} // namespace runner

#endif
