#include <iostream>
#include <fstream>

#include "maxlang/state.h"
#include "maxlang/stdlib.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << R"(Usage: maxlang <file>

file: path to program file.
)";
    }
    std::string file  = argv[1];
    file += ".mpp";
    std::ifstream fis(file, std::ios::binary);
    if (!fis.good()) {
        std::cout << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }
    std::string code((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());

    maxlang::State state;
    maxlang::stdlib::init(state);
    state.run(code);
    return 0;
}