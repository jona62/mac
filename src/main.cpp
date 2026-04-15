#include <cstring>
#include <iostream>
#include "MacRunner.h"

using namespace std;

int main(int argc, char **argv) {
    auto binaryDir = runner::getBinaryDir();
    meme::MacMeme::binaryDir() = binaryDir;

    // --analyze mode: output JSON analysis for LSP
    if (argc == 3 && strcmp(argv[1], "--analyze") == 0) {
        runner::analyzeFile(binaryDir, argv[2]);
        return 0;
    }

    if (argc > 2) {
        cout << "Usage: mac [script]" << endl;
        cout << "       mac --analyze <file>" << endl;
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
