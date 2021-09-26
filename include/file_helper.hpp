#pragma once

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

class FileHelper {
public:
    static std::string getFileAsString(std::string filepath);
    static void saveStringToFile(std::string dest, std::string text);
};

