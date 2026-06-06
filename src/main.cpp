#include <cstring>              // strcmp
#include <iostream>             // cout, endl
#include <string>               // string
#include "MacCatalog.h"         // mac_catalog::printCatalog
#include "MacRunner.h"          // runner::Runtime, getBinaryDir, runFile, runPrompt, analyzeFile
#include "Platform.h"           // platform::enableAnsiEscapes, getHomeDir
#include "UpdateCheck.h"        // updateCheck::checkForUpdate

using namespace std;

int main(int argc, char **argv) {
    platform::enableAnsiEscapes();

    auto binaryDir = runner::getBinaryDir();
    meme::MacMeme::binaryDir() = binaryDir;

    auto hasFlag = [](int argc, char** argv, const char* flag) {
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], flag) == 0) return true;
        }
        return false;
    };

    auto wantsHelp = [&](int argc, char** argv) {
        return hasFlag(argc, argv, "--help");
    };

    // --analyze mode: output JSON analysis for LSP
    if (argc >= 2 && strcmp(argv[1], "--analyze") == 0) {
        if (wantsHelp(argc, argv)) {
            cout << "mac --analyze <file>\n\n"
                 << "Output a JSON analysis of the file to stdout.\n"
                 << "Used by the VS Code extension for IDE features\n"
                 << "(diagnostics, completions, hover, semantic highlighting).\n\n"
                 << "The JSON contains: symbols, references, diagnostics, properties,\n"
                 << "foldingRanges, semanticTokens, paramHints, chainHints, signatures,\n"
                 << "classes, templates.\n";
            return 0;
        }
        if (argc != 3) {
            cerr << "Usage: mac --analyze <file>" << endl;
            return 1;
        }
        runner::analyzeFile(binaryDir, argv[2]);
        return 0;
    }

    // --catalog mode: output runtime/catalog metadata as JSON
    if (argc >= 2) {
        std::string arg = argv[1];
        if (arg == "--catalog") {
            if (wantsHelp(argc, argv)) {
                cout << "mac --catalog[=<selectors>]\n\n"
                     << "Output runtime catalog metadata as JSON.\n"
                     << "Without selectors, returns the full catalog.\n\n"
                     << "Selectors (comma-separated):\n"
                     << "  templates          Built-in template metadata\n"
                     << "  assets             All meme assets\n"
                     << "  assets:<category>  Assets in a category (e.g. assets:meme)\n"
                     << "  effects            Available effects\n"
                     << "  effect_definitions Named effect definitions\n"
                     << "  layouts            Layout types and slot counts\n"
                     << "  style_presets      Built-in style presets\n"
                     << "  limits             Runtime limits (dimensions, durations, etc.)\n"
                     << "  allowed_names      Valid template and effect names\n\n"
                     << "Examples:\n"
                     << "  mac --catalog\n"
                     << "  mac --catalog=limits\n"
                     << "  mac --catalog=assets:meme\n"
                     << "  mac --catalog=layouts,effects\n";
                return 0;
            }
            return mac_catalog::printCatalog(binaryDir);
        }
        const std::string prefix = "--catalog=";
        if (arg.rfind(prefix, 0) == 0) {
            return mac_catalog::printCatalog(binaryDir, arg.substr(prefix.size()));
        }
    }

    // --uninstall [--purge]: remove mac installation
    if (argc >= 2 && strcmp(argv[1], "--uninstall") == 0) {
        if (wantsHelp(argc, argv)) {
            cout << "mac --uninstall [--purge]\n\n"
                 << "Remove the Mac installation:\n"
                 << "  ~/.mac/            Binary, assets, stdlib\n"
                 << "  ~/.local/bin/mac   Symlink\n"
                 << "  ~/.mac_history     REPL history\n\n"
                 << "Generated output in ~/mac/output/ is kept by default.\n"
                 << "Pass --purge to also remove generated output files.\n";
            return 0;
        }
        bool purge = hasFlag(argc, argv, "--purge");
        string installDir = platform::installDir();
        string binLink = platform::binLink();
        string outputDir = platform::outputDir();
        string historyFile = platform::historyFile();

        cout << "\033[1m\033[35m  Uninstalling Mac...\033[0m" << endl;

        auto removeDir = [](const string& path) {
            if (std::filesystem::exists(path)) {
                std::filesystem::remove_all(path);
                cout << "  \033[32m✓\033[0m Removed " << path << endl;
                return true;
            }
            return false;
        };
        auto removeFile = [](const string& path) {
            if (std::filesystem::exists(path)) {
                std::filesystem::remove(path);
                cout << "  \033[32m✓\033[0m Removed " << path << endl;
                return true;
            }
            return false;
        };

        bool removed = removeDir(installDir) | removeFile(binLink);
        if (!removed) {
            cout << "  \033[33m!\033[0m Mac is not installed." << endl;
            cout << endl;
            return 0;
        }

        removeFile(historyFile);

        if (purge) {
            removeDir(outputDir);
        } else if (std::filesystem::exists(outputDir) && !std::filesystem::is_empty(outputDir)) {
            cout << "  \033[2m-\033[0m Kept " << outputDir << " (use --purge to remove)" << endl;
        }

        cout << "\n  \033[1m\033[32mDone.\033[0m Mac has been uninstalled." << endl;
        cout << endl;
        return 0;
    }

    // --version
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        cout << "Mac v" << MAC_VERSION << endl;
        return 0;
    }

    // --help
    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        cout << "Mac v" << MAC_VERSION << " - Meme as Code\n\n"
             << "Usage:\n"
             << "  mac                             Start the interactive REPL\n"
             << "  mac <script.mac>                Run a .mac file\n"
             << "  mac --analyze <file>            Output JSON analysis for LSP\n"
             << "  mac --catalog[=<selectors>]     Output runtime catalog as JSON\n"
             << "  mac --uninstall [--purge]       Remove Mac installation\n"
             << "  mac --version, -v               Print version\n"
             << "  mac --help, -h                  Show this help\n"
             << "\nPass --help to any command for details (e.g. mac --catalog --help).\n"
             << "\nDocs: https://docs.macstudio.meme\n";
        return 0;
    }

    if (argc > 2) {
        cerr << "Usage: mac [script]" << endl;
        cerr << "Run 'mac --help' for all options." << endl;
        return 1;
    }

    updateCheck::checkForUpdate();

    runner::Runtime rt;
    rt.loadPrelude(binaryDir);

    if (argc == 2) {
        runner::runFile(rt, argv[1]);
    } else {
        runner::runPrompt(rt);
    }

    return 0;
}
