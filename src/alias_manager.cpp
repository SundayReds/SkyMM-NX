#include "alias_manager.hpp"

alias_manager::alias_manager() {
    parser = alias_manager::alias_parser();
}

alias_manager* alias_manager::getInstance() {
    if (!instance) {
        instance = new alias_manager();
    }
    return instance;
}

bool alias_manager::has_alias(std::string filename) {
    return this->filename_to_alias_mapping.find(filename) == this->filename_to_alias_mapping.end();
}

std::string alias_manager::get_alias(std::string filename) {
    return has_alias(filename) ? filename_to_alias_mapping.at(filename)
                                    : std::string();
}

void alias_manager::set_alias(std::string filename, std::string alias) {
    if (alias.empty()) {
        remove_alias(filename);
        return;
    }

    if (!filename.empty()) {
        filename_to_alias_mapping[filename] = alias;
    }
}

void alias_manager::remove_alias(std::string filename) {
    if (has_alias(filename)) {
        this->filename_to_alias_mapping.erase(filename);
    }
}

void alias_manager::load_saved_alias(std::string filepath) {
    //try {
    if (!this->filename_to_alias_mapping.empty()) {
        this->filename_to_alias_mapping.clear();
    }

    std::string saved_text;
    saved_text = file_helper::get_file_as_string(filepath);
    this->parser.parse(saved_text);
    //} catch (std::exception& e) {
        // TODO: print relevant console stuff. 
        // alias is not core functionality,
        // so just catch all file handling exceptions here.
    //}
}

void alias_manager::alias_parser::parse(std::string text) {
    std::stringstream buffer = std::stringstream(text);

    std::vector<std::string> entries = split(text, NEWLINE);

    for (auto &entry : entries) {
        std::vector<std::string> tokens = split(entry, SEPARATOR_TOKEN);
        if (tokens.size() < 2 ||
            tokens.at(0).empty() ||
            tokens.at(1).empty()) {
            continue;
        }
        alias_manager::getInstance()->set_alias(tokens.at(0), tokens.at(1));
    }
}

void alias_manager::save_alias_list_to_disk(std::string dest) {
    //try {
    std::string plaintext = this->parser.convert_to_text(this->filename_to_alias_mapping);
    file_helper::save_string_to_file(dest, plaintext);
    //} catch (std::exception &e) {
        // TODO print relevant error message on console
    //}
}

std::string alias_manager::alias_parser::convert_to_text(std::unordered_map<std::string, std::string> &alias_list) {
    if (alias_list.empty()) {
        return std::string();
    }

    std::stringstream s;
    for (auto &entry : alias_list) {
        s << entry.first << SPACE << SEPARATOR_TOKEN << SPACE << entry.second << "\n";
    }
    return s.str();
}