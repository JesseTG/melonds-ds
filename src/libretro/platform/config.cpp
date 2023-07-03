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

#include "../config.hpp"

using namespace melonds::config;

int Platform::GetConfigInt(ConfigEntry entry)
{
    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_MaxBlockSize: return jit::MaxBlockSize();
#endif

        case DLDI_ImageSize: return save::DldiImageSize();

        case DSiSD_ImageSize: return save::DsiSdImageSize();

        case Firm_Language: return static_cast<int>(firmware::Language());
        case Firm_BirthdayMonth: firmware::BirthdayMonth();
        case Firm_BirthdayDay: return firmware::BirthdayDay();
        case Firm_Color: return firmware::FavoriteColour();

        case AudioBitDepth: return static_cast<int>(audio::BitDepth());
        default: return 0;
    }
}

bool Platform::GetConfigBool(ConfigEntry entry)
{
    switch (entry)
    {
#ifdef JIT_ENABLED
        case JIT_Enable: return jit::Enable();
        case JIT_LiteralOptimizations: return jit::LiteralOptimizations();
        case JIT_BranchOptimizations: return jit::BranchOptimizations();
        case JIT_FastMemory: return jit::FastMemory();
#endif

case ExternalBIOSEnable: return system::ExternalBiosEnable();

        case DLDI_Enable: return save::DldiEnable();
        case DLDI_ReadOnly: return save::DldiReadOnly();
        case DLDI_FolderSync: return save::DldiFolderSync();

        case DSiSD_Enable: return save::DsiSdEnable();
        case DSiSD_ReadOnly: return save::DsiSdReadOnly();
        case DSiSD_FolderSync: return save::DsiSdFolderSync();

        case Firm_OverrideSettings: return firmware::OverrideFirmwareSettings();
        default: return false;
    }
}

std::string Platform::GetConfigString(ConfigEntry entry)
{
    switch (entry)
    {
        case BIOS9Path: return system::Bios9Path();
        case BIOS7Path: return system::Bios7Path();
        case FirmwarePath: return system::FirmwarePath();

        case DSi_BIOS9Path: return system::DsiBios9Path();
        case DSi_BIOS7Path: return system::DsiBios7Path();
        case DSi_FirmwarePath: return system::DsiFirmwarePath();
        case DSi_NANDPath: return system::DsiNandPath();

        case DLDI_ImagePath: return save::DldiImagePath();
        case DLDI_FolderPath: return save::DldiFolderPath();

        case DSiSD_ImagePath: return save::DsiSdImagePath();
        case DSiSD_FolderPath: return save::DsiSdFolderPath();

        case Firm_Username: return firmware::Username();
        case Firm_Message: return firmware::Message();
        default: return "";
    }
}

bool Platform::GetConfigArray(ConfigEntry entry, void* data)
{
    using melonds::MacAddress;
    switch (entry)
    {
        case Firm_MAC:
        {
            MacAddress* mac = (MacAddress*)data;
            MacAddress current_mac = firmware::MacAddress();
            memcpy(mac, &current_mac, sizeof(MacAddress));
        }
        default:
            return false;
    }
}
