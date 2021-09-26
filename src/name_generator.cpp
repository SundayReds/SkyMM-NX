#include "name_generator.hpp"

const std::vector<std::string> NameGenerator::alphabet= {"a", "b", "c", "d", "e", "f", "g",
                                                        "h", "i", "j", "k", "l", "m", "n", 
                                                        "o", "p", "q", "r", "s", "t", "u",
                                                        "v", "w", "x", "y", "z"};

std::string NameGenerator::generateNext() {
    std::string name;

    if (this->number_generated < alphabet.size()) {
        name = this->alphabet.at(this->number_generated);
    } else {
        int temp = this->number_generated - 1;
        while (temp != 0) {
            temp--;
            name += this->alphabet.at((temp)% alphabet.size());
            temp /= 26;
        }
        reverse(name.begin(), name.end());
    }

    this->number_generated++;
    return name;
}