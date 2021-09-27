#pragma once

#include <string>

#include <switch.h>

#define MAX_INPUT_LENGTH 40

class Keyboard {
public:
    static Result show(std::string &output_str,
                        std::string title, 
                        std::string guide_msg, 
                        std::string initial_string = std::string());
    static std::string show(std::string title, 
                                std::string guide_msg, 
                                std::string initial_string = std::string());
};
