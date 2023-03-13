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

#include <Platform.h>
#include <frontend/qt_sdl/Config.h>

// TODO: Send a PR to split up the contents of <frontend/qt_sdl/Config.cpp>
// into pieces that don't use Qt, then compile those pieces instead of copying them here

int Platform::GetConfigInt(ConfigEntry entry)
{
    const int imgsizes[] = {0, 256, 512, 1024, 2048, 4096};

    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_MaxBlockSize: return Config::JIT_MaxBlockSize;
#endif

        case DLDI_ImageSize: return imgsizes[Config::DLDISize];

        case DSiSD_ImageSize: return imgsizes[Config::DSiSDSize];

        case Firm_Language: return Config::FirmwareLanguage;
        case Firm_BirthdayMonth: return Config::FirmwareBirthdayMonth;
        case Firm_BirthdayDay: return Config::FirmwareBirthdayDay;
        case Firm_Color: return Config::FirmwareFavouriteColour;

        case AudioBitrate: return Config::AudioBitrate;
    }

    return 0;
}

bool Platform::GetConfigBool(ConfigEntry entry)
{
    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_Enable: return Config::JIT_Enable != 0;
        case JIT_LiteralOptimizations: return Config::JIT_LiteralOptimisations != 0;
        case JIT_BranchOptimizations: return Config::JIT_BranchOptimisations != 0;
        case JIT_FastMemory: return Config::JIT_FastMemory != 0;
#endif

        case ExternalBIOSEnable: return Config::ExternalBIOSEnable != 0;

        case DLDI_Enable: return Config::DLDIEnable != 0;
        case DLDI_ReadOnly: return Config::DLDIReadOnly != 0;
        case DLDI_FolderSync: return Config::DLDIFolderSync != 0;

        case DSiSD_Enable: return Config::DSiSDEnable != 0;
        case DSiSD_ReadOnly: return Config::DSiSDReadOnly != 0;
        case DSiSD_FolderSync: return Config::DSiSDFolderSync != 0;

        case Firm_OverrideSettings: return Config::FirmwareOverrideSettings != 0;
    }

    return false;
}

std::string Platform::GetConfigString(ConfigEntry entry)
{
    switch (entry)
    {
        case BIOS9Path: return Config::BIOS9Path;
        case BIOS7Path: return Config::BIOS7Path;
        case FirmwarePath: return Config::FirmwarePath;

        case DSi_BIOS9Path: return Config::DSiBIOS9Path;
        case DSi_BIOS7Path: return Config::DSiBIOS7Path;
        case DSi_FirmwarePath: return Config::DSiFirmwarePath;
        case DSi_NANDPath: return Config::DSiNANDPath;

        case DLDI_ImagePath: return Config::DLDISDPath;
        case DLDI_FolderPath: return Config::DLDIFolderPath;

        case DSiSD_ImagePath: return Config::DSiSDPath;
        case DSiSD_FolderPath: return Config::DSiSDFolderPath;

        case Firm_Username: return Config::FirmwareUsername;
        case Firm_Message: return Config::FirmwareMessage;
    }

    return "";
}

bool Platform::GetConfigArray(ConfigEntry entry, void* data)
{
    switch (entry)
    {
        case Firm_MAC:
        {
            std::string& mac_in = Config::FirmwareMAC;
            u8* mac_out = (u8*)data;

            int o = 0;
            u8 tmp = 0;
            for (int i = 0; i < 18; i++)
            {
                char c = mac_in[i];
                if (c == '\0') break;

                int n;
                if      (c >= '0' && c <= '9') n = c - '0';
                else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
                else continue;

                if (!(o & 1))
                    tmp = n;
                else
                    mac_out[o >> 1] = n | (tmp << 4);

                o++;
                if (o >= 12) return true;
            }
        }
            return false;
    }

    return false;
}
