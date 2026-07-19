#include <iostream>

#include "command/cli.hpp"
#include "myproject/core/core.hpp"

int main(int argc, char *argv[]) {
    std::cout << "Add(1, 2) = " << Add(1, 2) << "\n" << std::flush;
    return RunCli(argc, argv);
}
