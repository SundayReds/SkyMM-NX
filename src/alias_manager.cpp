#include "alias_manager.hpp"

AliasManager* AliasManager::instance = 0;

AliasManager::AliasManager() {
    parser = AliasManager::AliasParser();
}

AliasManager* AliasManager::get_instance() {
    if (!instance) {
        instance = new AliasManager();
    }
    return instance;
}

bool AliasManager::has_alias(std::string filename) {
    return this->filename_to_alias_mapping.find(filename) != this->filename_to_alias_mapping.end();
}

std::string AliasManager::get_alias(std::string filename) {
    return has_alias(filename) ? filename_to_alias_mapping.at(filename)
                                    : std::string();
}

void AliasManager::set_alias(std::string filename, std::string alias) {
    // programmer error, empty base_name passed in
    if (filename.empty()) {
        return;
    }

    // remove alias
    if (alias.empty()) {
        remove_alias(filename);
        return;
    }

    // alias remains the same
    if (this->has_alias(filename) 
        && this->filename_to_alias_mapping.at(filename) == alias) {
        return;
    }

    // otherwise, change the alias and rewrite saved txt file
    filename_to_alias_mapping[filename] = alias;
    this->save_alias_list_to_disk();
}

void AliasManager::remove_alias(std::string filename) {
    if (has_alias(filename)) {
        this->filename_to_alias_mapping.erase(filename);
    }
}

void AliasManager::load_saved_alias(std::string filepath) {
    // maybe print relevant message if loading fails
    if (!this->filename_to_alias_mapping.empty()) {
        this->filename_to_alias_mapping.clear();
    }

    std::string saved_text;
    saved_text = file_helper::get_file_as_string(filepath);
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

        AliasManager::get_instance()->set_alias(mod_name, alias);
    }
}

void AliasManager::save_alias_list_to_disk(std::string dest) {
    // maybe print relevant console message if saving fails
    std::string plaintext = this->parser.convert_to_text(this->filename_to_alias_mapping);
    file_helper::save_string_to_file(dest, plaintext);
}

std::string AliasManager::AliasParser::convert_to_text(std::unordered_map<std::string, std::string> &alias_list) {
    if (alias_list.empty()) {
        return std::string();
    }

    std::stringstream s;
    for (auto &entry : alias_list) {
        s << entry.first << SPACE << SEPARATOR_TOKEN << SPACE << entry.second << "\n";
    }
    return s.str();
}