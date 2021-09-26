#include "name_generator.hpp"

const std::vector<std::string> NameGenerator::alphabet= {"a", "b", "c", "d", "e", "f", "g",
                                                        "h", "i", "j", "k", "l", "m", "n", 
                                                        "o", "p", "q", "r", "s", "t", "u",
                                                        "v", "w", "x", "y", "z"};

std::string NameGenerator::generateNext() {
    std::string name;

    if (this->number_generated < alphabet.size()) {
        name = alphabet.at(this->number_generated);
    } else {
        int temp = this->number_generated - 1;
        while (temp != 0) {
            temp--;
            name += alphabet.at((temp)% alphabet.size());
            temp /= 26;
        }
        reverse(name.begin(), name.end());
    }

    this->number_generated++;
    return name;
}

std::string NameGenerator::generateRandomAlphaString(size_t len) {
    std::string ret;
    ret.reserve(len);

    for (unsigned long i = 0; i < len; i ++) {
        unsigned long random_idx = rand() % (alphabet.size());
        ret += alphabet.at(random_idx);
    }

    return ret;
}
