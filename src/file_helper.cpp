#include "file_helper.hpp"

std::string file_helper::get_file_as_string(std::string filepath) {
    std::stringstream buffer;
    std::ifstream t(filepath);

    buffer << t.rdbuf();
    t.close();
    return buffer.str();
}

void file_helper::save_string_to_file(std::string dest, std::string text) {
    std::ofstream t(dest);
    t << text;
    t.close();
}