#include <iostream>
#include <string>
#include "uci.hpp"

int main() {
    UciState state;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        std::string out = handle_command(state, line);
        if (!out.empty())
            std::cout << out << std::endl;   // endl flushes so the GUI sees it
    }
    return 0;
}
