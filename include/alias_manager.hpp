#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "file_helper.hpp"
#include "path_helper.hpp"
#include "string_helper.hpp"

#define SEPARATOR_TOKEN "," // token must be illegal in filenames
#define SPACE " "
#define NEWLINE "\n"

class AliasManager {

public:
    static AliasManager *get_instance();

    bool has_alias(std::string filename);
    std::string get_alias(std::string filename);
    void set_alias(std::string filename, std::string alias);
    void remove_alias(std::string filename);
    void load_saved_alias(std::string filepath=SKYMM_NX_ALIAS_TXT_FILE);
    void save_alias_list_to_disk(std::string dest=SKYMM_NX_ALIAS_TXT_FILE);
    

private:
    static AliasManager *instance;
    std::unordered_map<std::string, std::string> filename_to_alias_mapping;

    AliasManager();

    class AliasParser {
    public:
        void parse(std::string text);
        std::string convert_to_text(std::unordered_map<std::string, std::string> &alias_list);
    };

    AliasParser parser;

};