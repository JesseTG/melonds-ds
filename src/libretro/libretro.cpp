#include <libretro.h>

#include "environment.hpp"
#include "libretro.hpp"

namespace retro {

    static std::string _base_directory;
    static std::string _save_directory;
}


PUBLIC_SYMBOL void retro_init(void) {
    const char *dir = nullptr;

    srand(time(nullptr));
    if (retro::environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
        retro::_base_directory = dir;

    if (retro::environment(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
        retro::_save_directory = dir;
}

PUBLIC_SYMBOL void retro_deinit(void) {
}