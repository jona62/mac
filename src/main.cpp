#include <cstring>                  // strcmp
#include <iostream>             // cout, endl
#include "MacRunner.h"              // runner::Runtime, getBinaryDir, runFile, runPrompt, analyzeFile

using namespace std;

int main(int argc, char **argv) {
    auto binaryDir = runner::getBinaryDir();
    meme::MacMeme::binaryDir() = binaryDir;

    // --analyze mode: output JSON analysis for LSP
    if (argc == 3 && strcmp(argv[1], "--analyze") == 0) {
        runner::analyzeFile(binaryDir, argv[2]);
        return 0;
    }

    // --uninstall: remove mac installation
    if (argc == 2 && strcmp(argv[1], "--uninstall") == 0) {
        string home = getenv("HOME") ? getenv("HOME") : ".";
        string installDir = home + "/.mac";
        string binLink = home + "/.local/bin/mac";
        string outputDir = home + "/mac/output";

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

        bool removed = removeDir(installDir) | removeFile(binLink) | removeDir(outputDir);
        if (removed) cout << "\n  \033[1m\033[32mDone.\033[0m Mac has been uninstalled." << endl;
        else cout << "  \033[33m!\033[0m Mac is not installed." << endl;
        cout << endl;
        return 0;
    }

    // --version
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        cout << "Mac v" << MAC_VERSION << endl;
        return 0;
    }

    if (argc > 2) {
        cout << "Usage: mac [script]" << endl;
        cout << "       mac --analyze <file>" << endl;
        cout << "       mac --uninstall" << endl;
        cout << "       mac --version" << endl;
        return 1;
    }

    runner::Runtime rt;
    rt.loadPrelude(binaryDir);

    if (argc == 2) {
        runner::runFile(rt, argv[1]);
    } else {
        runner::runPrompt(rt);
    }

    return 0;
}
