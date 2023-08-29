/*
    Copyright 2023 Jesse Talavera-Greenberg

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef MELONDS_DS_CONFIG_DEFINITIONS_FIRMWARE_HPP
#define MELONDS_DS_CONFIG_DEFINITIONS_FIRMWARE_HPP

#include <initializer_list>
#include <libretro.h>

#include "../constants.hpp"

namespace melonds::config::definitions {
    // If you ever get these translated, turn the variable into a template and
    // make the translated strings variable templates too.
    // Here's an example:
    template<retro_language L>
    constexpr const char* EnglishLabel = "English";

    template<retro_language L>
    constexpr std::initializer_list<retro_core_option_v2_definition> FirmwareOptionDefinitions {
        retro_core_option_v2_definition {
            config::firmware::FIRMWARE_PATH,
            "Firmware Path",
            nullptr,
            "Select a firmware image to use for DS mode. "
            "Files listed here must be:\n"
            "\n"
            "- Placed inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB).\n"
            "\n"
            "Defaults to Built-In if no firmware image is available. "
            "Built-In firmware cannot be booted and lacks GBA connectivity support. "
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
            "Ignored in DSi mode. "
            "Takes effect at the next boot (not reset).",
            nullptr,
            config::firmware::CATEGORY,
            {
                {melonds::config::values::BUILT_IN, "Built-In"},
                {nullptr, nullptr},
            },
            melonds::config::values::BUILT_IN
        },
        retro_core_option_v2_definition {
            config::firmware::FIRMWARE_DSI_PATH,
            "Firmware Path (DSi)",
            nullptr,
            "Select a firmware image to use for DSi mode. "
            "Files listed here must be:\n"
            "\n"
            "- Placed inside the frontend's system directory, or a subdirectory named \"melonDS DS\".\n"
            "- Exactly 131,072 bytes (128KB), 262,144 bytes (256KB), or 524,288 bytes (512KB).\n"
            "\n"
            "Defaults to Built-In if no firmware image is available. "
            "Built-In firmware cannot be booted. "
            "Nintendo WFC IDs are saved to firmware, "
            "so switching firmware images may result in the loss of some WFC data. "
            "Ignored in DS mode. "
            "Takes effect at the next boot (not reset).",
            nullptr,
            config::firmware::CATEGORY,
            {
                {melonds::config::values::BUILT_IN, "Built-In"},
                {nullptr, nullptr},
            },
            melonds::config::values::BUILT_IN
        },
        retro_core_option_v2_definition {
            config::firmware::WFC_DNS,
            "DNS Override",
            nullptr,
            "The DNS server to use instead of the default. "
            "Use this to play online with custom servers that reimplement "
            "the discontinued Nintendo Wi-Fi Connection service. "
            "Supported games depends on the server. "
            "If unsure, leave on Kaeru WFC. "
            "Changes take effect at next restart.\n"
            "\n"
            "To use other servers, open \"wfc.cfg\" in the \"melonDS DS\" system directory and follow the instructions. "
            "The listed servers are not affiliated with libretro, Nintendo, the melonDS team, or the core's author. "
            "Please respect other players and follow the server's rules.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {config::values::wfc::KAERU, "Kaeru WFC (178.62.43.212)"},
                {config::values::wfc::ALTWFC, "AltWFC (172.104.88.237)"},
                {config::values::wfc::WIIMMFI, "Wiimmfi (95.217.77.181)"},
                {config::values::wfc::DEFAULT, "Force Default DNS"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::wfc::KAERU
        },
        retro_core_option_v2_definition {
            config::firmware::LANGUAGE,
            "Language",
            nullptr,
            "Overrides the language mode of the emulated console.\n"
            "\n"
            "Use Host: Uses the frontend's language if supported by the DS, or English if not.\n"
            "Don't Override: Doesn't override firmware defines.\n"
            "\n"
            "Not every game honors this setting. "
            "Takes effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {config::values::AUTO, "Use Host"},
                {config::values::ENGLISH, EnglishLabel<L>},
                {config::values::JAPANESE, "Japanese"},
                {config::values::FRENCH, "French"},
                {config::values::GERMAN, "German"},
                {config::values::ITALIAN, "Italian"},
                {config::values::SPANISH, "Spanish"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::AUTO
        },
        retro_core_option_v2_definition {
            config::firmware::USERNAME,
            "Username",
            nullptr,
            "Overrides the username given to the emulated console.\n"
            "\n"
            "Don't Override: Uses whatever name is defined on the firmware.\n"
            "Guess: Try to get your username from the frontend. "
            "If that fails, try the USER, USERNAME, and LOGNAME environment variables.\n"
            "melonDS DS: Uses \"melonDS DS\". "
            "This is the default if nothing else works.\n"
            "\n"
            "Names are truncated to 10 characters. "
            "Changes take effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                // Backslashes aren't allowed in DS usernames, so we use them for special values
                {config::values::firmware::GUESS_USERNAME, "Guess"},
                {config::values::firmware::DEFAULT_USERNAME, nullptr},
                {config::values::firmware::FIRMWARE_USERNAME, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::firmware::GUESS_USERNAME
        },
        retro_core_option_v2_definition {
            config::firmware::FAVORITE_COLOR,
            "Favorite Color",
            nullptr,
            "The theme (\"favorite color\") of the emulated console. "
            "Changes take effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {"0", "Gray"},
                {"1", "Brown"},
                {"2", "Red"},
                {"3", "Light Pink"},
                {"4", "Orange"},
                {"5", "Yellow"},
                {"6", "Lime"},
                {"7", "Light Green"},
                {"8", "Dark Green"},
                {"9", "Turquoise"},
                {"10", "Light Blue"},
                {"11", "Blue"},
                {"12", "Dark Blue"},
                {"13", "Dark Purple"},
                {"14", "Light Purple"},
                {"15", "Dark Pink"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            "0"
        },
        retro_core_option_v2_definition {
            config::firmware::BIRTH_MONTH,
            "Birth Month",
            nullptr,
            "The month of your birthday. "
            "Takes effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {"1", "January"},
                {"2", "February"},
                {"3", "March"},
                {"4", "April"},
                {"5", "May"},
                {"6", "June"},
                {"7", "July"},
                {"8", "August"},
                {"9", "September"},
                {"10", "October"},
                {"11", "November"},
                {"12", "December"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::DEFAULT
        },
        retro_core_option_v2_definition {
            config::firmware::BIRTH_DAY,
            "Birth Day",
            nullptr,
            "The day within the month of your birthday. "
            "Takes effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {"1", "1st"},
                {"2", "2nd"},
                {"3", "3rd"},
                {"4", "4th"},
                {"5", "5th"},
                {"6", "6th"},
                {"7", "7th"},
                {"8", "8th"},
                {"9", "9th"},
                {"10", "10th"},
                {"11", "11th"},
                {"12", "12th"},
                {"13", "13th"},
                {"14", "14th"},
                {"15", "15th"},
                {"16", "16th"},
                {"17", "17th"},
                {"18", "18th"},
                {"19", "19th"},
                {"20", "20th"},
                {"21", "21st"},
                {"22", "22nd"},
                {"23", "23rd"},
                {"24", "24th"},
                {"25", "25th"},
                {"26", "26th"},
                {"27", "27th"},
                {"28", "28th"},
                {"29", "29th"},
                {"30", "30th"},
                {"31", "31st"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::DEFAULT
        },
        retro_core_option_v2_definition {
            config::firmware::ENABLE_ALARM,
            "Enable Alarm",
            nullptr,
            "Whether the emulated console's alarm is enabled. "
            "Changes take effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {config::values::DISABLED, "Disabled"},
                {config::values::ENABLED, "Enabled"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::DEFAULT
        },
        retro_core_option_v2_definition {
            config::firmware::ALARM_HOUR,
            "Alarm Hour",
            nullptr,
            "The hour of the configured alarm. "
            "Changes take effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {"0", "12 AM"},
                {"1", "1 AM"},
                {"2", "2 AM"},
                {"3", "3 AM"},
                {"4", "4 AM"},
                {"5", "5 AM"},
                {"6", "6 AM"},
                {"7", "7 AM"},
                {"8", "8 AM"},
                {"9", "9 AM"},
                {"10", "10 AM"},
                {"11", "11 AM"},
                {"12", "12 PM"},
                {"13", "1 PM"},
                {"14", "2 PM"},
                {"15", "3 PM"},
                {"16", "4 PM"},
                {"17", "5 PM"},
                {"18", "6 PM"},
                {"19", "7 PM"},
                {"20", "8 PM"},
                {"21", "9 PM"},
                {"22", "10 PM"},
                {"23", "11 PM"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::DEFAULT
        },
        retro_core_option_v2_definition {
            config::firmware::ALARM_MINUTE,
            "Alarm Minute",
            nullptr,
            "The minute of the configured alarm. "
            "Changes take effect at next restart.",
            nullptr,
            config::firmware::CATEGORY,
            {
                {"0", "00"},
                {"1", "01"},
                {"2", "02"},
                {"3", "03"},
                {"4", "04"},
                {"5", "05"},
                {"6", "06"},
                {"7", "07"},
                {"8", "08"},
                {"9", "09"},
                {"10", "10"},
                {"11", "11"},
                {"12", "12"},
                {"13", "13"},
                {"14", "14"},
                {"15", "15"},
                {"16", "16"},
                {"17", "17"},
                {"18", "18"},
                {"19", "19"},
                {"20", "20"},
                {"21", "21"},
                {"22", "22"},
                {"23", "23"},
                {"24", "24"},
                {"25", "25"},
                {"26", "26"},
                {"27", "27"},
                {"28", "28"},
                {"29", "29"},
                {"30", "30"},
                {"31", "31"},
                {"32", "32"},
                {"33", "33"},
                {"34", "34"},
                {"35", "35"},
                {"36", "36"},
                {"37", "37"},
                {"38", "38"},
                {"39", "39"},
                {"40", "40"},
                {"41", "41"},
                {"42", "42"},
                {"43", "43"},
                {"44", "44"},
                {"45", "45"},
                {"46", "46"},
                {"47", "47"},
                {"48", "48"},
                {"49", "49"},
                {"50", "50"},
                {"51", "51"},
                {"52", "52"},
                {"53", "53"},
                {"54", "54"},
                {"55", "55"},
                {"56", "56"},
                {"57", "57"},
                {"58", "58"},
                {"59", "59"},
                {config::values::DEFAULT, "Don't Override"},
                {nullptr, nullptr},
            },
            config::values::DEFAULT
        },
    };
}

#endif //MELONDS_DS_CONFIG_DEFINITIONS_FIRMWARE_HPP
