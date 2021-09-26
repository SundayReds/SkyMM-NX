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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define EXT_ESP "esp"
#define EXT_ESM "esm"
#define EXT_BSA "bsa"

#define SUFFIX_ANIMATIONS_LONG "Animations"
#define SUFFIX_MESHES_LONG "Meshes"
#define SUFFIX_SOUNDS_LONG "Sounds"
#define SUFFIX_TEXTURES_LONG "Textures"
#define SUFFIX_VOICES_LONG "Voices"

#define SUFFIX_ANIMATIONS "A"
#define SUFFIX_MESHES "M"
#define SUFFIX_NONE ""
#define SUFFIX_SOUNDS "S"
#define SUFFIX_TEXTURES "T"
#define SUFFIX_VOICES "V"

static const std::unordered_set<std::string> LONG_SUFFIXES = {
    SUFFIX_ANIMATIONS_LONG, SUFFIX_MESHES_LONG,
    SUFFIX_SOUNDS_LONG, SUFFIX_TEXTURES_LONG,
    SUFFIX_VOICES_LONG,
};

static const std::unordered_set<std::string> SHORT_SUFFIXES = {
    SUFFIX_ANIMATIONS, SUFFIX_MESHES, SUFFIX_SOUNDS,
    SUFFIX_TEXTURES, SUFFIX_VOICES
};

static const std::unordered_map<std::string, std::string> LONG_TO_SHORT_SUFFIXES = {
    {SUFFIX_ANIMATIONS_LONG, SUFFIX_ANIMATIONS},
    {SUFFIX_MESHES_LONG, SUFFIX_MESHES},
    {SUFFIX_SOUNDS_LONG, SUFFIX_SOUNDS},
    {SUFFIX_TEXTURES_LONG, SUFFIX_TEXTURES},
    {SUFFIX_VOICES_LONG, SUFFIX_VOICES}
};

enum class ModStatus {
    ENABLED,
    DISABLED,
    PARTIAL
};

enum class ModFileType {
    BSA,
    ESP,
    ESM,
    UNKNOWN
};

struct ModFile {
    ModFileType type;
    std::string base_name;
    std::string suffix;

    static ModFile fromFileName(std::string const &file_name);
};

struct SkyrimMod {
    std::string base_name;
    bool has_esp;
    bool is_master;
    bool esp_enabled;
    std::vector<std::string> bsa_suffixes;
    std::map<std::string, int> enabled_bsas;

    SkyrimMod(std::string const &base_name):
            base_name(base_name),
            has_esp(false),
            is_master(false),
            esp_enabled(false),
            bsa_suffixes(),
            enabled_bsas() {
    }

    ModStatus getStatus(void);

    void enable(void);

    void disable(void);

    void loadSooner(void);

    void loadLater(void);
};

typedef std::vector<std::shared_ptr<SkyrimMod>> ModList;

ModList &getGlobalModList(void);

std::shared_ptr<SkyrimMod> find_mod(ModList const &mod_list, std::string const &name);
