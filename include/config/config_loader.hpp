#pragma once

#include <string>

struct AddConfig {
    int a = 0;
    int b = 0;
};

struct Config {
    std::string mode = "default";
    int timeout = 30;
    AddConfig add;
};
