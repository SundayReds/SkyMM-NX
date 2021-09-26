#pragma once

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

class FileHelper {
public:
    static std::string get_file_as_string(std::string filepath);
    static void save_string_to_file(std::string dest, std::string text);
};

