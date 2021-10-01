/*
 * This file is a part of SkyMM-NX.
 * Copyright (c) 2019, Max Roncace <mproncace@gmail.com>
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "alias_manager.hpp"
#include "console_helper.hpp"
#include "error_defs.hpp"
#include "gui.hpp"
#include "ini_helper.hpp"
#include "keyboard_helper.hpp"
#include "mod.hpp"
#include "name_generator.hpp"
#include "path_helper.hpp"
#include "string_helper.hpp"

#include <inipp/inipp.h>
#include <switch.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstdio>
#include <dirent.h>

#define STRINGIZE0(x) #x
#define STRINGIZE(x) STRINGIZE0(x)

#ifndef __VERSION
#define __VERSION "Unknown"
#endif

#define HEADER_HEIGHT 3
#define FOOTER_HEIGHT 5

#define HRULE "--------------------------------"

#define SCROLL_INTERVAL 100000000
#define SCROLL_INITIAL_DELAY 400000000

static HidNpadButton g_key_edit_lo = HidNpadButton_Y;

static bool g_dirty = false;
static bool g_dirty_warned = false;
static bool mass_renaming_first_warned = false;
static bool mass_renaming_second_warned = false;
static bool mass_renaming_final_warned = false;
static bool single_renaming_warned = false;

static std::string g_status_msg = "";
static bool g_tmp_status = false;

static ModList g_mod_list_tmp;

static std::string g_plugins_header;

static int g_scroll_dir = 0;
static u64 g_last_scroll_time = 0;
static bool g_scroll_initial_cooldown;

static bool g_edit_load_order = false;

static u64 _nanotime(void) {
    return armTicksToNs(armGetSystemTick());
}

int discoverMods() {
    DIR *dir = opendir(getRomfsPath(SKYRIM_DATA_DIR).c_str());

    if (!dir) {
        FATAL("No Skyrim data folder found!\nSearched in %s", getBaseRomfsPath());
        return -1;
    }

    std::vector<std::string> files;

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_type != DT_REG) {
            continue;
        }

        files.insert(files.end(), ent->d_name);
    }

    closedir(dir);

    printf("Found %lu mod files\n", files.size());

    for (std::string file_name : files) {
        ModFile mod_file = ModFile::fromFileName(file_name);

        if (mod_file.type == ModFileType::UNKNOWN) {
            continue;
        } 

        std::shared_ptr<SkyrimMod> mod = find_mod(g_mod_list_tmp, mod_file.base_name);
        if (!mod) {
            mod = std::shared_ptr<SkyrimMod>(new SkyrimMod(mod_file.base_name));
            // everything gets loaded into a temp buffer so we can rebuild it with the proper order later
            g_mod_list_tmp.insert(g_mod_list_tmp.end(), mod);
        }

        if (mod_file.type == ModFileType::ESP) {
            mod->has_esp = true;
        } else if (mod_file.type == ModFileType::ESM) {
            mod->has_esp = true;
            mod->is_master = true;
        } else if (mod_file.type == ModFileType::BSA) {
            mod->bsa_suffixes.insert(mod->bsa_suffixes.end(), mod_file.suffix);
        } else {
            PANIC();
            return -1;
        }
    }

    return 0;
}

int processPluginsFile() {
    std::ifstream plugins_stream = std::ifstream(getRomfsPath(SKYRIM_PLUGINS_FILE), std::ios::in);
    if (!plugins_stream.good()) {
        FATAL("Failed to open Plugins file");
        return -1;
    }

    bool in_header = true;
    std::stringstream header_stream;
    std::string line;
    while (std::getline(plugins_stream, line)) {
        if (line.length() == 0 || line.at(0) == '#') {
            if (in_header) {
                header_stream << line << '\n';
            }
            continue;
        }

        if (in_header) {
            g_plugins_header = header_stream.str();
            in_header = false;
        }

        bool enable = line.at(0) == '*';

        std::string file_name = enable ? line.substr(1) : line;
        ModFile file_def = ModFile::fromFileName(file_name);
        if (file_def.type != ModFileType::ESP && file_def.type != ModFileType::ESM) {
            continue;
        }

        std::shared_ptr<SkyrimMod> mod = find_mod(getGlobalModList(), file_def.base_name);
        if (!mod) {
            mod = find_mod(g_mod_list_tmp, file_def.base_name);
            if (mod) {
                getGlobalModList().insert(getGlobalModList().end(), mod);
            } else {
                continue;
            }
        }

        mod->esp_enabled = enable;
    }

    return 0;
}

int writePluginsFile(void) {
    std::ofstream plugins_stream = std::ofstream(getRomfsPath(SKYRIM_PLUGINS_FILE),
            std::ios::out | std::ios::trunc | std::ios::binary);
    if (!plugins_stream.good()) {
        FATAL("Failed to open Plugins file");
        return -1;
    }

    // write header that we loaded earlier
    plugins_stream << g_plugins_header;

    for (std::shared_ptr<SkyrimMod> mod : getGlobalModList()) {
        if (mod->has_esp) {
            if (mod->esp_enabled) {
                plugins_stream << '*';
            }
            if (mod->is_master) {
                plugins_stream << mod->base_name << ".esm";
            } else {
                plugins_stream << mod->base_name << ".esp";
            }
            plugins_stream << '\n';
        }
    }

    return 0;
}

int initialize(void) {
    int rc;

    CONSOLE_SET_POS(0, 0);
    CONSOLE_CLEAR_SCREEN();
    CONSOLE_SET_ATTRS(CONSOLE_ATTR_BOLD);
    printf("Attempting to retrieve mod aliases, if any...\n");

    // load alias file
    AliasManager::getInstance()->loadSavedAlias(SKYMM_NX_ALIAS_TXT_FILE);

    printf("Discovering available mods...\n");

    if (RC_FAILURE(rc = discoverMods())) {
        return rc;
    }

    if (RC_FAILURE(rc = processPluginsFile())) {
        return rc;
    }

    if (RC_FAILURE(rc = parseInis(getGlobalModList(), g_mod_list_tmp))) {
        return rc;
    }

    for (std::shared_ptr<SkyrimMod> mod : g_mod_list_tmp) {
        std::string mod_name = mod->base_name;
        if (!find_mod(getGlobalModList(), mod_name)) {
            getGlobalModList().insert(getGlobalModList().end(), mod);
        }
    }

    printf("Identified %lu mods\n", getGlobalModList().size());

    CONSOLE_MOVE_DOWN(3);
    printf("Mod listing:\n\n");
    for (auto it = getGlobalModList().cbegin(); it != getGlobalModList().cend(); it++) {
        ModStatus status = (*it)->getStatus();
        const char *status_str;
        switch (status) {
            case ModStatus::ENABLED:
                status_str = "Enabled";
                break;
            case ModStatus::DISABLED:
                status_str = "Disabled";
                break;
            case ModStatus::PARTIAL:
                status_str = "Partial";
                break;
            default:
                PANIC();
                return -1;
        }
        printf("  - %s (%s)\n", (*it)->base_name.c_str(), status_str);
    }

    return 0;
}

static void redrawHeader(void) {
    CONSOLE_SET_POS(0, 0);
    CONSOLE_CLEAR_LINE();
    CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_CYAN);
    printf("SkyMM-NX v" STRINGIZE(__VERSION) " by caseif");
    CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_MAGENTA);
    // make it clear that this is a modified version
    // in case anyone somehow downloads this by accident before
    // i private the repo or update README
    printf(", modified by SundayReds");
    CONSOLE_MOVE_DOWN(1);
    CONSOLE_MOVE_LEFT(255);
    CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_WHITE);
    printf("NOTE: Shorten suffixes eg. 'Mod - Meshes.bsa' should be 'Mod - M.bsa'.");


    CONSOLE_MOVE_DOWN(1);
    CONSOLE_MOVE_LEFT(255);
    printf(HRULE);
}

static void redrawFooter() {
    CONSOLE_SET_POS(39, 0);
    CONSOLE_CLEAR_LINE();
    printf(HRULE);
    CONSOLE_MOVE_LEFT(255);
    CONSOLE_MOVE_DOWN(1);

    CONSOLE_MOVE_DOWN(1);
    CONSOLE_CLEAR_LINE();
    if (!g_status_msg.empty()) {
        CONSOLE_SET_ATTRS(CONSOLE_ATTR_NONE);
        CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_YELLOW);
        printf(g_status_msg.c_str());
        CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_WHITE);
        CONSOLE_SET_ATTRS(CONSOLE_ATTR_BOLD);
    }
    CONSOLE_MOVE_LEFT(255);

    CONSOLE_MOVE_DOWN(1);
    CONSOLE_CLEAR_LINE();

    CONSOLE_MOVE_DOWN(1);
    CONSOLE_CLEAR_LINE();
    CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_GREEN);
    printf("(A) Toggle | (Y) (hold) Change Load Order | (-) Save | (+) Exit");
    CONSOLE_MOVE_LEFT(255);
    CONSOLE_MOVE_DOWN(2);
    printf("(B) Rename | (X) Nickname | (ZR) Auto-rename | (R+L) Auto-rename ALL");
    CONSOLE_SET_COLOR(CONSOLE_COLOR_FG_WHITE);
}

static void clearTempEffects(void) {
    g_dirty_warned = false;
    mass_renaming_first_warned = false;
    mass_renaming_second_warned = false;
    mass_renaming_final_warned = false;
    single_renaming_warned = false;

    if (g_tmp_status) {
        g_status_msg = "";
        g_tmp_status = false;
        redrawFooter();
    }
}

void handleScrollHold(u64 kDown, u64 kHeld, HidNpadButton key, ModGui &gui) {
    if (kHeld & key && !(kDown & key)) {
        u64 period = g_scroll_initial_cooldown ? SCROLL_INITIAL_DELAY : SCROLL_INTERVAL;

        if (_nanotime() - g_last_scroll_time >= period) {
            g_last_scroll_time = _nanotime();
            g_scroll_initial_cooldown = false;

            if (g_edit_load_order) {
                if (gui.getSelectedIndex() < getGlobalModList().size() - 1) {
                    if (g_scroll_dir == 1) {
                        gui.getSelectedMod()->loadLater();
                    } else {
                        gui.getSelectedMod()->loadSooner();
                    }
                    g_dirty = true;
                }
            }

            gui.scrollSelection(g_scroll_dir);

            clearTempEffects();
        }
    }
}

void renameModFiles(std::shared_ptr<SkyrimMod> mod, std::string &new_base_name) {

    // ESP
    std::string data_dir = getRomfsPath(SKYRIM_DATA_DIR) + DIR_SEP;
    if (mod->has_esp) {
        rename((data_dir + mod->base_name + DOT + EXT_ESP).c_str(), 
                (data_dir + new_base_name + DOT + EXT_ESP).c_str());
    }

    // ESM
    if (mod->is_master) {
        rename((data_dir + mod->base_name + DOT + EXT_ESM).c_str(),
                (data_dir + new_base_name + DOT + EXT_ESM).c_str());
    }

    // PLAIN BSA
    if (std::filesystem::exists(data_dir + mod->base_name + DOT + EXT_BSA)) {
        rename((data_dir + mod->base_name + DOT + EXT_BSA).c_str(),
                (data_dir + new_base_name + DOT + EXT_BSA).c_str());
    }

    std::vector<std::string> new_bsa_suffixes;

    // ALL SUFFIXED BSA's
    for (auto &suffix : mod->bsa_suffixes) {

        // exclude empty suffix, already covered
        if (suffix.empty()) {
            new_bsa_suffixes.emplace_back(SUFFIX_NONE);
            continue;
        }

        // if is long suffix, change it to short suffix while renaming
        if (LONG_SUFFIXES.count(suffix)) {
            new_bsa_suffixes.emplace_back(LONG_TO_SHORT_SUFFIXES.at(suffix));
            rename((data_dir + mod->base_name + SP + DASH + SP + suffix
                        + DOT + EXT_BSA).c_str(),
                    (data_dir + new_base_name + SP + DASH + SP + LONG_TO_SHORT_SUFFIXES.at(suffix)
                        + DOT + EXT_BSA).c_str());
            continue;

        } else { // else, it's either a short suffix or unknown. we leave it alone
            new_bsa_suffixes.emplace_back(suffix);
            rename((data_dir + mod->base_name + SP + DASH + SP + suffix
                        + DOT + EXT_BSA).c_str(),
                    (data_dir + new_base_name + SP + DASH + SP + suffix
                        + DOT + EXT_BSA).c_str());
        }
    }

    // re-construct mod->enabled_bsas with shortened suffixes
    std::map<std::string, int> new_enabled_bsas;
    for (std::pair<std::string, int> suffix_pair : mod->enabled_bsas) {
        if (LONG_SUFFIXES.count(suffix_pair.first)) {
            new_enabled_bsas.insert(std::pair(LONG_TO_SHORT_SUFFIXES.at(suffix_pair.first), suffix_pair.second));
        } else {
            new_enabled_bsas.insert(std::pair(suffix_pair.first, suffix_pair.second));
        }
    }

    // lastly, reflect the base_name change in SkyrimMod
    mod->base_name = new_base_name;
    mod->bsa_suffixes = new_bsa_suffixes;
    mod->enabled_bsas = new_enabled_bsas;
}

bool hasNamingConflicts(std::string base_name) {
    std::string lower_base_name = base_name;
    transform(lower_base_name.begin(), lower_base_name.end(), lower_base_name.begin(), ::tolower);
    for (std::shared_ptr<SkyrimMod> mod : getGlobalModList()) {
        std::string lower_mod_name = mod->base_name;
        transform(lower_mod_name.begin(), lower_mod_name.end(), lower_mod_name.begin(), ::tolower);
        if (lower_base_name == lower_mod_name) {
            return true;
        }
    }
    return false;
}

void autoRenameMod(std::shared_ptr<SkyrimMod> mod) {
    NameGenerator name_generator = NameGenerator();
    std::string new_name = name_generator.generateNext();

    while (hasNamingConflicts(new_name)) {
        new_name = name_generator.generateNext();
    }

    std::string original_base = mod->base_name;

    renameModFiles(mod, new_name);
    // if it had an old alias, update the base_name only and keep old alias
    // else, set old base_name as an alias of the newly generated name
    if (AliasManager::getInstance()->hasAlias(original_base)) {
        AliasManager::getInstance()->updateBaseName(original_base, new_name);
    } else {
        AliasManager::getInstance()->setAlias(new_name, original_base);
    }
}

void massAutoRenameMods() {
    // randomly generate base_names for each mod. prevents conflicts when renaming.
    std::unordered_map<std::string, std::string> tmpname_to_basename;
    NameGenerator name_generator = NameGenerator();

    for (std::shared_ptr<SkyrimMod> mod : getGlobalModList()) {

        g_status_msg = "DO NOT CLOSE: Renaming mod '" + mod->base_name + "' ...";
        redrawFooter();
        consoleUpdate(NULL);

        // get randomized temp name
        std::string tmp_name_str = NameGenerator::generateRandomAlphaString(20);

        // store original base_name, and remember which random name it was assigned
        tmpname_to_basename[tmp_name_str] = mod->base_name;

        // change all associated files to temp name
        renameModFiles(mod, tmp_name_str);
    }

    // start renaming them to shortened names
    // at this point they all have randomized names, so
    // astronomically low chance of conflict.
    for (std::shared_ptr<SkyrimMod> mod : getGlobalModList()) {

        // generate next shortest name
        std::string new_name = name_generator.generateNext();

        // retrieve the original name of this mod
        std::string original_base = tmpname_to_basename.at(mod->base_name);
        
        // rename to the newly generated shortname
        renameModFiles(mod, new_name);

        // if it had an old alias, update the base_name only and keep old alias
        // else, set old base_name as an alias of the newly generated name
        if (AliasManager::getInstance()->hasAlias(original_base)) {
            AliasManager::getInstance()->updateBaseName(original_base, new_name);
        } else {
            AliasManager::getInstance()->setAlias(new_name, original_base);
        }
    }
}

int main(int argc, char **argv) {
    consoleInit(NULL);

    ModGui gui = ModGui(getGlobalModList(), HEADER_HEIGHT, CONSOLE_LINES - HEADER_HEIGHT - FOOTER_HEIGHT);
    
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState defaultPad;
    padInitializeDefault(&defaultPad);

    int init_status = initialize();
    if (RC_SUCCESS(init_status)) {
        CONSOLE_CLEAR_SCREEN();

        redrawHeader();
        gui.redraw();
        redrawFooter();
    }

    while (appletMainLoop()) {
        padUpdate(&defaultPad);

        u64 kDown = padGetButtonsDown(&defaultPad);
        u64 kUp = padGetButtonsUp(&defaultPad);
        u64 kHeld = padGetButtons(&defaultPad);

        if (kDown & HidNpadButton_Plus) {
            if (g_dirty && !g_dirty_warned) {
                g_status_msg = "Press (+) to exit without saving changes";
                g_tmp_status = true;
                g_dirty_warned = true;
                redrawFooter();
            } else {
                break;
            }
        }

        if (RC_FAILURE(init_status) || fatal_occurred()) {
            consoleUpdate(NULL);
            continue;
        }

        if (kDown & g_key_edit_lo) {
            g_edit_load_order = true;
            g_status_msg = "Editing load order";
            redrawFooter();
        }
        
        if (kUp & g_key_edit_lo) {
            g_edit_load_order = false;
            g_status_msg = "";
            redrawFooter();
        }

        if ((kUp & HidNpadButton_AnyDown) && g_scroll_dir == 1) {
            g_scroll_dir = 0;
        } else if ((kUp & HidNpadButton_AnyUp) && g_scroll_dir == -1) {
            g_scroll_dir = 0;
        }

        switch (g_scroll_dir) {
            case -1:
                handleScrollHold(kDown, kHeld, HidNpadButton_AnyUp, gui);
                break;
            case 1:
                handleScrollHold(kDown, kHeld, HidNpadButton_AnyDown, gui);
                break;
            default:
                break;
        }

        if (kDown & HidNpadButton_AnyDown) {
            if (g_edit_load_order) {
                if (gui.getSelectedIndex() < getGlobalModList().size() - 1) {
                    gui.getSelectedMod()->loadLater();
                    g_dirty = true;
                }
            }

            g_last_scroll_time = _nanotime();
            g_scroll_initial_cooldown = true;
            g_scroll_dir = 1;

            gui.scrollSelection(1);

            clearTempEffects();
        } else if (kDown & HidNpadButton_AnyUp) {
            if (g_edit_load_order) {
                if (gui.getSelectedIndex() > 0) {
                    gui.getSelectedMod()->loadSooner();
                    g_dirty = true;
                }
            }

            g_last_scroll_time = _nanotime();
            g_scroll_initial_cooldown = true;
            g_scroll_dir = -1;

            gui.scrollSelection(-1);

            clearTempEffects();
        } else if (kDown & HidNpadButton_A) {
            std::shared_ptr<SkyrimMod> mod = gui.getSelectedMod();
            switch (mod->getStatus()) {
                case ModStatus::ENABLED:
                    mod->disable();
                    break;
                case ModStatus::PARTIAL:
                case ModStatus::DISABLED:
                    mod->enable();
                    break;
                default:
                    PANIC();
            }
            g_dirty = true;

            gui.redrawCurrentRow();

            clearTempEffects();
        }

        if (kDown & HidNpadButton_Minus) {
            g_status_msg = "Saving changes...";
            redrawFooter();
            consoleUpdate(NULL);

            writePluginsFile();
            writeIniChanges();
            g_dirty = false;

            g_status_msg = "Wrote changes to SDMC!";
            g_tmp_status = true;
            redrawFooter();
        }

        if (kDown & HidNpadButton_X) {
            //find out which mod was selected
            std::shared_ptr<SkyrimMod> mod = gui.getSelectedMod();
            bool currently_has_alias = AliasManager::getInstance()->hasAlias(mod->base_name);
            //bring up keyboard and capture input
            std::string retstr;
            Result rc = Keyboard::show(retstr,
                                        // title
                                        "Enter new alias for '"
                                            + mod->base_name
                                            + ((currently_has_alias) ? " (" 
                                                                        + AliasManager::getInstance()
                                                                            ->getAlias(mod->base_name) 
                                                                        + ")'"
                                                                    : "'"),
                                        // guide text                
                                         "New Alias (MAX: " 
                                        + std::to_string(MAX_INPUT_LENGTH) 
                                        + " characters)",
                                        // initial starting text
                                        (currently_has_alias) ? AliasManager::getInstance()
                                                                ->getAlias(mod->base_name)
                                                            : std::string());

            if (R_SUCCEEDED(rc)) {
                //save alias
                AliasManager::getInstance()->setAlias(mod->base_name, retstr);

                //push updates to display
                gui.redrawCurrentRow();
                g_status_msg = (retstr.empty())? "Alias successfully removed."
                                                : "Alias successfully set.";
                redrawFooter();
            }
            clearTempEffects();
        }

        // Rename modfiles with name provided by user
        if (kDown & HidNpadButton_B) {
            std::shared_ptr<SkyrimMod> mod = gui.getSelectedMod();
            std::string new_name = Keyboard::show("Enter new name for '" // title
                                                    + mod->base_name
                                                    + ((AliasManager::getInstance()->hasAlias(mod->base_name)) ? " (" + 
                                                                    AliasManager::getInstance()
                                                                                ->getAlias(mod->base_name) 
                                                                    + ")'"
                                                                : "'"),

                                                // guide text                
                                                "New Mod Name (MAX: " 
                                                + std::to_string(MAX_INPUT_LENGTH) 
                                                + " characters)",

                                                // initial starting text
                                                mod->base_name);
            if (!new_name.empty()) {
                if (hasNamingConflicts(new_name)) {
                    g_status_msg = "Mod '" + new_name + "' already exists.";
                    redrawFooter();
                } else {
                    std::string original_base = mod->base_name;
                    renameModFiles(mod, new_name);
                    if (AliasManager::getInstance()->hasAlias(original_base)) {
                        AliasManager::getInstance()->updateBaseName(original_base, new_name);
                    } else {
                        AliasManager::getInstance()->setAlias(new_name, original_base);
                    }
                    writePluginsFile();
                    writeIniChanges();
                    clearTempEffects();
                    gui.redrawCurrentRow();
                    g_status_msg = "Mod successfully auto-renamed.";
                    redrawFooter();
                }
            }
        }

        // warning for single-auto-renaming function
        if (kDown & HidNpadButton_ZR) {
            if (!single_renaming_warned) {
                g_status_msg = "WARNING: Will auto-rename to next shortest name. (LStick) to cont.";
                g_tmp_status = true;
                single_renaming_warned = true;
                redrawFooter();
            }
        }

        // execute single-auto-rename function
        if (kDown & HidNpadButton_StickL) {
            if (getGlobalModList().empty()) {
                    clearTempEffects();
                    g_status_msg = "No Mods were detected. Auto-renaming process halted.";
                    redrawFooter();
            } else if (single_renaming_warned) {
                std::shared_ptr<SkyrimMod> mod = gui.getSelectedMod();
                g_status_msg = "Auto-renaming mod in progress...";
                redrawFooter();
                consoleUpdate(NULL);
                autoRenameMod(mod);
                writePluginsFile();
                writeIniChanges();
                clearTempEffects();
                gui.redrawCurrentRow();
                g_status_msg = "Mod successfully auto-renamed.";
                redrawFooter();
            }
        }

        // warnings for mass-auto-renaming function
        if ((kDown & HidNpadButton_R) 
            && (kDown & HidNpadButton_L)) {
            // first warning
            if (!mass_renaming_first_warned) {
                g_status_msg = "WARNING: Will auto-rename ALL modfiles and auto assign alias. (R + L) to cont.";
                g_tmp_status = true;
                mass_renaming_first_warned = true;
                redrawFooter();
            // second warning
            } else if (!mass_renaming_second_warned) {
                g_status_msg = "WARNING: Renaming modfiles can affect current saves. (R + L) to cont.";
                g_tmp_status = true;
                mass_renaming_second_warned = true;
                redrawFooter();
            // final warning, different button combination needed to progress
            } else if (!mass_renaming_final_warned) {
                g_status_msg = "FINAL: Make sure you have backups. (RStick + LStick) to start.";
                g_tmp_status = true;
                mass_renaming_final_warned = true;
                redrawFooter();
            }
        }

        // execute mass auto renaming function
        if ((kDown & HidNpadButton_StickR) 
            && (kDown & HidNpadButton_StickL)) {
            if (mass_renaming_final_warned) {
                if (getGlobalModList().empty()) {
                    clearTempEffects();
                    g_status_msg = "No Mods were detected. Auto-renaming process halted.";
                    redrawFooter();
                } else {
                    // mass rename and auto alias generation
                    massAutoRenameMods();

                    // rewrite plugins and inis
                    writePluginsFile();
                    writeIniChanges();

                    // redraw lists
                    gui.redraw();

                    clearTempEffects();
                    g_status_msg = "ALl Modfiles successfully auto-renamed and aliases auto-assigned.";
                    redrawFooter();
                }
            }
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
