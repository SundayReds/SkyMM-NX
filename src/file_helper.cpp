#include "file_helper.hpp"

std::string file_helper::get_file_as_string(std::string filepath) {
    std::ifstream t(filepath);

    if (t) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        t.close();
        return buffer.str();
    }

    return std::string();
}

void file_helper::save_string_to_file(std::string dest, std::string text) {
    std::ofstream t(dest);
    t << text;
    t.close();
}