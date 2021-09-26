# SkyMM-NX

SkyMM-NX is a simple mod manager that runs on your Switch and provides an easy way to toggle mods on
and off.

SkyMM will attempt to discover all mods present in Skyrim's ROMFS on the SD card and present them through its interface.
Through the interface, you can toggle mods on or off, or change the load order by holding `Y`. Note that the load order
for pure replacement mods (lacking an ESP) will not be preserved when the respective mods are disabled.

When the save function is invoked, the INI and `Plugins` files will be modified accordingly and saved to the SD card.

### Changes Made To Original Work
1. Changed all suffixes to be single letter only. 
    - For example, `EnaiRim - Textures` should now be formatted as `EnaiRim - T`.
    - The `Skyrim.ini` file has a line limit of 1024 characters, which can be hit very quickly if we use the lengthy original suffixes scheme on NexusMods (I hit it around 16 mods, even with succinct base modnames).
    - This change hence aims to remove all of that unnecessary character bloat from the `skyrim.ini` configuration file so that more mods can be loaded.

2. Added a feature to give each mod an in-app alias. Pressing `X` while hovering over a mod prompts user to add an alias to the mod, which will then be saved to disk and then loaded up the next time. Entering an empty alias will delete the current alias, if any, associated with the selected mod. The aliases will be saved in a human-readable format in `skymm-alias.txt` within the same folder. which the user can opt to edit directly instead of going through the app.
    - eg. Mod `A` as displayed in-app can now be given the alias `Crossbows of Skyrim`, which will display in-app as `A (Crossbows of Skyrim)`. The alias will be saved in a human-readable format in `skymm-alias.txt` within the same folder. The user can opt to edit the `.txt` directly.
    - Allows user to shorten base names of mods as much as they want without runnning the risk 

3. Added a feature to auto-generate shortened forms of all mod filenames with auto-shortened suffixes, then give them an in-app alias so that the user can identify them. 
    - This will auto rename all modfiles as per the naming scheme defined below. eg. `Proper Crossbow Integration.esp` and `Proper Crossbow Integration - Textures.bsa` will be auto-renamed to something like `a.esp` and `a - T.bsa`. It will then be displayed in-app as: `a (Proper Crossbow Integration)`. ie. mod `a` with alias of `Proper Crossbow Integration`
    - If the mod had no alias prior to auto-renaming, its original filename will become its alias. Otherwise, it will retain whatever alias you chose to give it before activating the mass renaming function.
    - NOTE: Although this helps you with everything else, you still need to make sure all modfiles belonging to a single mod have the same base_name (See naming conventions below). Some mod authors don't enforce this in their uploads for some reason.
    - NOTE: Some mod authors hard-code their filenames, which means that changing the names of files will break them. This generally is bad practice and most good modders avoid it, but there are some authors who still do this anyway. There is no way of telling whether or not a mod contains hardcoded file links, so if one of your mods break after renaming, you might want to try renaming it back to their original names
    - IMPORTANT: backup your mods, `Skyrim.ini`, `Skyrim_en.ini`, and `Plugins` before attempting to use this function (plus it's generally good practice anyway). This function works but has not been rigorously tested.


4. Small fix to suffix checking for Animations.

The main application was not made by me. All credit for that goes to [caseif](https://github.com/caseif/SkyMM-NX).

### Naming Scheme (updated)

Currently, the app requires that all mods follow a standard naming scheme:

- All suffixes in filenames are to be truncated to one-letter only
  - `Mod - Animations.bsa` to be renamed `Mod - A.bsa`
  - `Mod - Meshes.bsa` to be renamed `Mod - M.bsa`
  - `Mod - Sounds.bsa` to be renamed `Mod - S.bsa`
  - `Mod - Textures.bsa` to be renamed `Mod - T.bsa`
  - `Mod - Voices.bsa` to be renamed `Mod - V.bsa`
  - Additional Tip: You can further replace the basename `Mod` with something even shorter like `M` - just use common sense and make sure it doesn't conflict with the basename of another mod.

- BSA files with a suffix must use a hyphen with one space on either side between the base name and the suffix
  - Example: `Static Mesh Improvement Mod - T.bsa`
  - Note that a mod may have exactly one non-suffixed BSA file
- BSA files with an associated ESP file must match the ESP's name, not including the suffix
  - Example: `Static Mesh Improvement Mod - T.bsa` matches `Static Mesh Improvement Mod.esp`
- All BSA files for a given mod must match each other in name
  - Example: `Static Mesh Improvement Mod - T.bsa` matches `Static Mesh Improvement Mod - M.bsa`

### To-do

* COMPLETED - Add ability to 'nickname' or give aliases to mods in-app or on PC via a `.txt` so that it's easier to identify truncated modnames (so that you don't come back to the game after 5 years and start wondering what `E.esp` does)
* COMPLETED - Add an in-app function to rename all `.bsa` and `.esp` files in the `Data/` folder to short names like `a.esp`, then automatically generating aliases for all renamed mods that did not previously have an alias, based on their original filename
    - eg 1. `Mod1` will be become `A (Mod1)` in-app, and its associated `.esp` file will be renamed from `Mod1.esp` to `A.esp` (along with its other files)
    - eg 2. `Mod2 (Aho Project)` will become `B (Aho Project)` in-app (no change to alias since it already has one), and its associated `.esp` file will be renamed from `Mod2.esp` to `B.esp` (along with its other files)
* Same as above, but just for one mod instead of all the mods. ie. find a shortened name that does not conflict, then rename the modfiles and shorten all relevant suffixes.
* Fix/Tweak `.ini` writing. According to the author of Skyrim-NX-Toolkit, textures and voices `.bsa` files are supposed to go under `sResourceArchiveList2=` in `Skyrim.ini` instead of `sArchiveToLoadInMemoryList=`. The current app just adds them all under the latter, but for some reason still works perfectly fine. Even so, by distributing the `.bsa` files as per the Skyrim-NX-Toolkit method, we can reduce more bloat from the `sArchiveToLoadInMemoryList=`, which should prevent us from hitting the 1024 limit earlier.

### Building

SkyMM-NX depends on `devkitA64`, `libnx`, and `switch-tools` to compile. These packages are installable through
[devkitPro pacman](https://devkitpro.org/wiki/devkitPro_pacman).

Once all dependencies have been satisfied, simply run `make` in the project directory.

### License

SkyMM-NX is made available under the
[MIT License](https://github.com/caseif/SkyrimNXModManager/blob/master/LICENSE). It may be used, modified, and
distributed within the bounds of this license.
