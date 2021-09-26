#include "keyboard_helper.hpp"

std::string Keyboard::show(std::string title, 
                            std::string guide_msg, 
                            std::string initial_string) {
    Result rc = 0;
    SwkbdConfig kbd;
    char tmpoutstr[MAX_INPUT_LENGTH] = {0};
    rc = swkbdCreate(&kbd, 0);

    if (R_SUCCEEDED(rc)) {
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetHeaderText(&kbd, title.c_str());
        swkbdConfigSetGuideText(&kbd, guide_msg.c_str());
        swkbdConfigSetInitialText(&kbd, initial_string.c_str());

        rc = swkbdShow(&kbd, tmpoutstr, sizeof(tmpoutstr));
        swkbdClose(&kbd);

        return R_SUCCEEDED(rc) ? std::string(tmpoutstr)
                                : std::string();
    }

    return std::string();
}