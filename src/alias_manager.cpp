#include "alias_manager.hpp"

AliasManager* AliasManager::instance = 0;

AliasManager::AliasManager() {
    parser = AliasManager::AliasParser();
}

AliasManager* AliasManager::getInstance() {
    if (!instance) {
        instance = new AliasManager();
    }
    return instance;
}

bool AliasManager::hasAlias(std::string filename) {
    return this->filename_to_alias_mapping.find(filename) != this->filename_to_alias_mapping.end();
}

std::string AliasManager::getAlias(std::string filename) {
    return hasAlias(filename) ? filename_to_alias_mapping.at(filename)
                                    : std::string();
}

void AliasManager::setAlias(std::string filename, std::string alias) {
    // programmer error, empty base_name passed in
    if (filename.empty()) {
        return;
    }

    // remove alias
    if (alias.empty()) {
        this->removeAlias(filename);
        return;
    }

    // alias remains the same
    if (this->hasAlias(filename) 
        && this->filename_to_alias_mapping.at(filename) == alias) {
        return;
    }

    // otherwise, change the alias and rewrite saved txt file
    filename_to_alias_mapping[filename] = alias;
    this->saveAliasListToDisk();
}

void AliasManager::removeAlias(std::string filename) {
    if (filename.empty()) {
        return;
    }

    if (this->hasAlias(filename)) {
        this->filename_to_alias_mapping.erase(filename);
    }
}

void AliasManager::updateBaseName(std::string old_base_name, std::string new_base_name) {
    std::string alias = this->getAlias(old_base_name);

    if (!alias.empty()) {
        this->removeAlias(old_base_name);
        this->setAlias(new_base_name, alias);
    }
}

void AliasManager::loadSavedAlias(std::string filepath) {
    // maybe print relevant message if loading fails
    if (!this->filename_to_alias_mapping.empty()) {
        this->filename_to_alias_mapping.clear();
    }

    std::string saved_text;
    saved_text = FileHelper::getFileAsString(filepath);
    if (!saved_text.empty()) {
        this->parser.parse(saved_text);
    }
}

void AliasManager::AliasParser::parse(std::string text) {
    std::stringstream buffer = std::stringstream(text);

    std::vector<std::string> entries = split(text, NEWLINE);

    for (auto &entry : entries) {
        std::vector<std::string> tokens = split(entry, SEPARATOR_TOKEN);
        if (tokens.size() < 2 ||
            tokens.at(0).empty() ||
            tokens.at(1).empty()) {
            continue;
        }

        std::string mod_name = tokens.at(0);
        std::string alias = tokens.at(1);

        // for the case when user uses a separator token in alias
        if (tokens.size() > 2) {
            for (unsigned long i = 2; i < tokens.size(); i++) {
                alias += tokens.at(i);
            }
        }

        AliasManager::getInstance()->setAlias(mod_name, alias);
    }
}

void AliasManager::saveAliasListToDisk(std::string dest) {
    // maybe print relevant console message if saving fails
    std::string plaintext = this->parser.convertToText(this->filename_to_alias_mapping);
    FileHelper::saveStringToFile(dest, plaintext);
}

std::string AliasManager::AliasParser::convertToText(std::unordered_map<std::string, std::string> &alias_list) {
    if (alias_list.empty()) {
        return std::string();
    }

    std::stringstream s;
    for (auto &entry : alias_list) {
        s << entry.first << SPACE << SEPARATOR_TOKEN << SPACE << entry.second << "\n";
    }
    return s.str();
}