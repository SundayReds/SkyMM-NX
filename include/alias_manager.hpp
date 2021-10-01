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
    static AliasManager *getInstance();

    bool hasAlias(std::string filename);
    std::string getAlias(std::string filename);
    void setAlias(std::string filename, std::string alias);
    void removeAlias(std::string filename);
    void loadSavedAlias(std::string filepath=SKYMM_NX_ALIAS_TXT_FILE);
    void saveAliasListToDisk(std::string dest=SKYMM_NX_ALIAS_TXT_FILE);
    

private:
    static AliasManager *instance;
    std::unordered_map<std::string, std::string> filename_to_alias_mapping;

    AliasManager();

    class AliasParser {
    public:
        void parse(std::string text);
        std::string convertToText(std::unordered_map<std::string, std::string> &alias_list);
    };

    AliasParser parser;

};