// WebAssembly entry: one persistent UCI session driven by handle_command.
// The browser worker calls uci_command(line) and forwards each returned line.
#include "uci.hpp"
#include <string>

static UciState g_state;

extern "C" {
const char* uci_command(const char* line) {
    static std::string out;      // kept alive until the next call; JS copies it out immediately
    out = handle_command(g_state, std::string(line ? line : ""));
    return out.c_str();
}
}
