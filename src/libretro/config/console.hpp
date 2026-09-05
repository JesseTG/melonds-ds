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

#ifndef MELONDSDS_CONFIG_CONSOLE_HPP
#define MELONDSDS_CONFIG_CONSOLE_HPP

#include <memory>
#include <string_view>
#include "std/span.hpp"
#include "types.hpp"

namespace melonDS {
    class NDS;
    struct NDSHeader;
}

namespace retro {
    class GameInfo;
}

namespace MelonDsDs {
    class CoreConfig;
    class CoreState;

    /// The characters that GBATEK permits in an NDS game code.
    /// Nintendo assigned these codes as four uppercase ASCII characters;
    /// tools that build homebrew leave a placeholder there instead.
    /// \see https://problemkaputt.de/gbatek-ds-cartridge-header.htm
    inline constexpr std::string_view GAME_CODE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    /// \return \c true if \c gameCode has the shape
    /// that GBATEK documents for the cartridge header's game code field.
    /// \param gameCode The game code to check.
    /// \c NDSHeader::GameCode is not NUL-terminated,
    /// so callers must pass its length explicitly.
    [[nodiscard]] constexpr bool IsValidGameCode(std::string_view gameCode) noexcept {
        return gameCode.length() == 4
            && gameCode.find_first_not_of(GAME_CODE_CHARS) == std::string_view::npos;
    }

    /// \return \c true if this ROM probably wasn't released by Nintendo.
    /// \c NDSHeader::IsHomebrew only looks for a missing secure area
    /// or for the placeholder game code that \c ndstool writes,
    /// so this also rejects a game code that isn't shaped like a real one.
    [[nodiscard]] bool LooksLikeHomebrew(const melonDS::NDSHeader& header) noexcept;

    /// \return \c true if this ROM must be installed onto the DSi's NAND to run,
    /// i.e. if it's genuine DSiWare rather than homebrew wearing a DSiWare title ID.
    /// Installation needs title metadata that only Nintendo can supply,
    /// so a ROM that doesn't look retail is run from the cart slot instead.
    /// See https://github.com/JesseTG/melonds-ds/issues/319.
    [[nodiscard]] bool RequiresNandInstall(const melonDS::NDSHeader& header) noexcept;

    /// \return \c true if this ROM can only run on a DSi,
    /// i.e. a DSi-exclusive cartridge or a title that lives on the NAND.
    /// Per GBATEK, header unit code 00h is a DS game,
    /// 02h is a DSi-enhanced game that still runs on a DS,
    /// and 03h is DSi-only.
    /// Homebrew that merely carries a DSiWare title ID doesn't count;
    /// see \c RequiresNandInstall.
    [[nodiscard]] bool RequiresDSi(const melonDS::NDSHeader& header) noexcept;

    /// \return \c true if this ROM looks like a retail release rather than homebrew.
    /// Retail DSi titles have modcrypted regions, whereas homebrew and devkit builds don't;
    /// melonDS checks the same flag before decrypting them.
    /// This is a better signal than \c NDSHeader::IsHomebrew,
    /// which misses homebrew that declares a real game code.
    [[nodiscard]] bool IsRetailDSiTitle(const melonDS::NDSHeader& header) noexcept;

    /// Resolves the console mode the player asked for
    /// into the console that will actually be emulated.
    /// \param mode The player's "Console Mode" setting.
    /// \param header The loaded ROM's header,
    /// or \c nullptr if booting to the console's own menu.
    [[nodiscard]] ConsoleType ResolveConsoleType(ConsoleMode mode, const melonDS::NDSHeader* header) noexcept;

    /// Creates a new console instance, for when the player is starting a session.
    std::unique_ptr<melonDS::NDS> CreateConsole(
        CoreState& state,
        const CoreConfig& config,
        const retro::GameInfo* ndsInfo,
        const retro::GameInfo* gbaInfo,
        const retro::GameInfo* gbaSaveInfo
    );

    /// Modify a console instance with core options that are safe to adjust at runtime.
    void UpdateConsole(const CoreConfig& config, melonDS::NDS& nds) noexcept;

    /// Modify a console instance with core options that require a reset to adjust.
    void ResetConsole(const CoreConfig& config, melonDS::NDS& nds);

    bool GetDsiwareSaveDataHostPath(std::span<char> buffer, const retro::GameInfo& nds_info, int type) noexcept;
}

#endif // MELONDSDS_CONFIG_CONSOLE_HPP
