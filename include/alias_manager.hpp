#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "file_helper.hpp"
#include "string_helper.hpp"

#define SEPARATOR_TOKEN ","
#define SPACE " "
#define NEWLINE "\n"

class alias_manager {

public:
    static alias_manager *getInstance();

    bool has_alias(std::string filename);
    std::string get_alias(std::string filename);
    void set_alias(std::string filename, std::string alias);
    void remove_alias(std::string filename);
    void load_saved_alias(std::string filepath);
    void save_alias_list_to_disk(std::string dest);
    

private:
    static alias_manager *instance;
    std::unordered_map<std::string, std::string> filename_to_alias_mapping;

    alias_manager();

    class alias_parser {
    public:
        void parse(std::string text);
        std::string convert_to_text(std::unordered_map<std::string, std::string> &alias_list);
    };

    alias_parser parser;

};