/*
    Copyright 2026 Matheus Fraguas

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

#pragma once

#include <array>
#include <cstdint>

#include <libretro.h>

// GBACart.h refers to NDSCart without including it.
#include <NDSCart.h>
#include <GBACart.h>

#include "config/types.hpp"

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class CoreConfig;

    /// The cart type that \c PaddleCart reports.
    /// melonDS doesn't emulate the paddle itself, so this ID isn't one of its own;
    /// it's kept well clear of melonDS's IDs so that upstream additions won't collide with it.
    /// Savestates use it to make sure they're restored with the same Slot-2 device.
    constexpr melonDS::u32 PADDLE_CART_TYPE = 0x2A0;

    /// The Taito Paddle Controller, a spinner that plugs into Slot-2.
    /// Arkanoid DS and Space Invaders Extreme (among others) can use it.
    ///
    /// The paddle has no ROM or save memory to speak of.
    /// It reports its dial's position as a 12-bit counter
    /// that grows as the dial turns clockwise and wraps around at either end;
    /// games track how it changes between reads, not where it is.
    ///
    /// - Every ROM halfword reads 0xEFFF, which is how games detect the paddle.
    /// - SRAM byte 0 holds the counter's low 8 bits,
    ///   and SRAM byte 1 holds its high 4 bits.
    ///   Every other SRAM byte reads 0.
    /// - Writing anything to ROM or SRAM recalibrates the paddle,
    ///   which sets the counter to 0.
    class PaddleCart final : public melonDS::GBACart::CartCommon {
    public:
        PaddleCart() noexcept;

        void DoSavestate(melonDS::Savestate* file) override;

        [[nodiscard]] melonDS::u16 ROMRead(melonDS::u32 addr) const override;
        void ROMWrite(melonDS::u32 addr, melonDS::u16 val) override;

        melonDS::u8 SRAMRead(melonDS::u32 addr) override;
        void SRAMWrite(melonDS::u32 addr, melonDS::u8 val) override;

        /// Turns the dial by \c steps counter units;
        /// positive is clockwise.
        void Turn(int steps) noexcept;

        [[nodiscard]] uint16_t Position() const noexcept { return _position; }
    private:
        static constexpr melonDS::u16 POSITION_MASK = 0xFFF;
        melonDS::u16 _position = 0;
    };

    /// Turns the player's mouse and left analog stick into the paddle's dial movement.
    ///
    /// While the mouse is turning the paddle, it can't touch the screen,
    /// and its buttons press whichever DS buttons the player chose;
    /// any mouse button mapped to the toggle switches between the two.
    class PaddleState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;

        /// Reads how far the mouse and stick moved since the last frame.
        ///
        /// Call once per frame, after polling for input.
        void Update() noexcept;

        /// Turns the paddle in Slot-2 by however much the mouse and stick moved.
        void Apply(melonDS::NDS& nds) const noexcept;

        /// Whether the mouse is currently turning the paddle
        /// instead of driving the touch screen.
        [[nodiscard]] bool CapturesMouse() const noexcept { return _capturesMouse; }

        /// The RetroPad buttons that the mouse's buttons are pressing this frame.
        [[nodiscard]] uint32_t JoypadButtons() const noexcept { return _joypadButtons; }
    private:
        PaddleAxis _axis = PaddleAxis::MouseX;
        /// Dial movement per unit of mouse movement, in percent.
        unsigned _speed = 100;
        bool _inverted = false;

        /// What each mouse button does, indexed by \c PaddleMouseButton;
        /// see \c CoreConfig::PaddleMouseButtons.
        std::array<uint32_t, PaddleMouseButtonCount> _mouseButtonMappings {};

        bool _capturesMouse = true;
        bool _toggleButton = false;
        uint32_t _joypadButtons = 0;

        /// How far to turn the dial this frame, in counter units.
        int _steps = 0;

        /// Dial movement too small to register yet, in hundredths of a step.
        /// Carried over to the next frame so that slow speeds still add up.
        int _remainder = 0;
    };
}
