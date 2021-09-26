#include "file_helper.hpp"

std::string FileHelper::getFileAsString(std::string filepath) {
    std::ifstream t(filepath);

    if (t) {
        std::stringstream buffer;
        buffer << t.rdbuf();
        t.close();
        return buffer.str();
    }

    return std::string();
}

void FileHelper::saveStringToFile(std::string dest, std::string text) {
    std::ofstream t(dest);
    t << text;
    t.close();
}