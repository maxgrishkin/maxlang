#include <iostream>
#include <fstream>

#include "eblang/state.h"
#include "eblang/stdlib.h"

using namespace maxlang;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << R"(Usage: maxlang <file>

file: path to program file.
)";
    }
    std::ifstream fis(argv[1], std::ios::binary);
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