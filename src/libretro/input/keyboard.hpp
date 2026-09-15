/*
    Copyright 2026 Davey Hughes

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
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <libretro.h>
#include <PokeTypeKeyboard.h>

namespace melonDS {
    class NDS;
}

namespace MelonDsDs {
    class CoreConfig;

    /// The game's keyboard reports letters the other way up from a host one:
    /// a bare A carries 'A', and Shift+A carries 'a'.
    /// \p region's layout says which keys are letters,
    /// including the ones outside ASCII, like German's Ä.
    [[nodiscard]] uint16_t FlipLetterCase(melonDS::PokeTypeKeyboard::Region region, uint16_t c) noexcept;

    /// The wireless keyboard that came with Learn with Pokémon: Typing Adventure.
    /// melonDS emulates its Bluetooth link;
    /// this types the frontend's keystrokes into the game.
    class TypingKeyboardState {
    public:
        void SetConfig(const CoreConfig& config) noexcept;

        /// Whether the loaded game is Typing Adventure.
        /// Keystrokes are ignored and arrow keys aren't polled otherwise.
        void SetActive(bool active) noexcept;

        /// The frontend's keyboard callback.
        /// Some frontends call it from their video thread, so it only queues keystrokes.
        void KeyEvent(bool down, unsigned keycode, uint32_t character, uint16_t modifiers) noexcept;

        /// Polls the arrow keys, after the frontend's input has been polled.
        void Update() noexcept;

        /// Types the queued keystrokes into the game.
        void Apply(melonDS::NDS& nds) noexcept;

        /// The KEYINPUT bits that arrow keys are holding down,
        /// when arrow keys are set to press the D-pad.
        [[nodiscard]] uint32_t HeldButtons() const noexcept { return _heldButtons; }

        /// Drops queued keystrokes and lets go of held keys.
        void Reset() noexcept;
    private:
        enum class KeystrokeKind : uint8_t {
            /// Type Character; a KeyId of 0 looks the key up from it, once its case is flipped
            Typed,
            /// Character is the label on the host's key; type what the game's key with that label types
            Label,
            /// Fn, which makes the keyboard discoverable until the game has connected
            Pairing,
        };

        struct Keystroke {
            uint16_t Character;
            /// HID usage
            uint8_t KeyId;
            /// HID modifier byte
            uint8_t Modifiers;
            KeystrokeKind Kind;
            bool CapsLock;
        };

        void Queue(const Keystroke& keystroke) noexcept;
        bool Dequeue(Keystroke& keystroke) noexcept;
        void LockQueue() noexcept;
        void UnlockQueue() noexcept { _queueLocked.store(false, std::memory_order_release); }

        // A spinlock and a fixed ring rather than a mutex and a vector:
        // nothing to allocate on the callback's thread, and nothing to free
        // if the frontend calls back while the core is being torn down.
        // More keystrokes than fit between two frames would overflow the game's own queue anyway.
        static constexpr size_t QueueSize = 32;
        std::array<Keystroke, QueueSize> _queue {};
        size_t _queueHead = 0;
        size_t _queueTail = 0;
        std::atomic<bool> _queueLocked = false;

        std::atomic<bool> _active = false;
        std::atomic<bool> _enabled = true;
        std::atomic<unsigned> _fnKey = RETROK_INSERT;
        std::atomic<bool> _altGrHeld = false;
        bool _autoPair = true;
        bool _arrowsPressDpad = false;
        uint32_t _heldButtons = 0;
    };
}
