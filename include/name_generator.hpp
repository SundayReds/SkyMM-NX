#pragma once

#include <algorithm>
#include <string>
#include <vector>

class NameGenerator {
public:
    static std::string generateRandomAlphaString(size_t len);

    std::string generateNext();

private:
    static const std::vector<std::string> alphabet;
    unsigned long number_generated = 0;
};
