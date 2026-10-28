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

#include "keyboard.hpp"

#include <thread>

#include <NDS.h>
#include <NDSCart/CartRetailBT.h>
#include <PokeTypeKeyboard.h>

#include "config/config.hpp"
#include "environment.hpp"
#include "tracy.hpp"

using MelonDsDs::TypingKeyboardState;
using melonDS::PokeTypeKeyboard;

namespace {
    // The game's IDs for the keys that type no printable character,
    // the same set as PokeTypeKeyboard's special keys
    constexpr uint8_t ENTER = 0x28;
    constexpr uint8_t BACKSPACE = 0x2A;
    constexpr uint8_t TAB = 0x2B;
    constexpr uint8_t SPACE = 0x2C;
    constexpr uint8_t CAPS_LOCK = 0x39;
    constexpr uint8_t RIGHT = 0x4F;
    constexpr uint8_t LEFT = 0x50;
    constexpr uint8_t DOWN = 0x51;
    constexpr uint8_t UP = 0x52;
    constexpr uint8_t CTRL = 0x70;
    constexpr uint8_t RIGHT_SHIFT = 0x71;
    constexpr uint8_t LEFT_SHIFT = 0x72;
    constexpr uint8_t ALT = 0x73;
    constexpr uint8_t HOME = 0x74;
    constexpr uint8_t FN = 0x75;
    constexpr uint8_t ALT_GR = 0x76;

    uint8_t SpecialKeyId(unsigned keycode) noexcept {
        switch (keycode) {
            case RETROK_RETURN:
            case RETROK_KP_ENTER: return ENTER;
            case RETROK_BACKSPACE: return BACKSPACE;
            case RETROK_TAB: return TAB;
            case RETROK_SPACE: return SPACE;
            case RETROK_CAPSLOCK: return CAPS_LOCK;
            case RETROK_RIGHT: return RIGHT;
            case RETROK_LEFT: return LEFT;
            case RETROK_DOWN: return DOWN;
            case RETROK_UP: return UP;
            case RETROK_LCTRL:
            case RETROK_RCTRL: return CTRL;
            case RETROK_RSHIFT: return RIGHT_SHIFT;
            case RETROK_LSHIFT: return LEFT_SHIFT;
            case RETROK_LALT: return ALT;
            case RETROK_RALT:
            case RETROK_MODE: return ALT_GR;
            case RETROK_HOME: return HOME;
            default: return 0;
        }
    }

    uint16_t ToUpper(uint16_t c) noexcept {
        return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
    }

    /// The game's key with this label on it: the symbol on its unshifted level,
    /// or else a letter or digit on its shifted level (AZERTY puts the digits there).
    const PokeTypeKeyboard::KeyDesc* FindKeyByLabel(PokeTypeKeyboard::Region region, uint16_t label) noexcept {
        uint32_t count = 0;
        const PokeTypeKeyboard::KeyDesc* keys = PokeTypeKeyboard::GetKeyTable(region, count);
        if (!keys) return nullptr;

        label = ToUpper(label);
        for (uint32_t i = 0; i < count; i++) {
            if (ToUpper(keys[i].Base) == label) return &keys[i];
        }

        bool alphanumeric = (label >= 'A' && label <= 'Z') || (label >= '0' && label <= '9');
        for (uint32_t i = 0; alphanumeric && i < count; i++) {
            if (ToUpper(keys[i].Shift) == label) return &keys[i];
        }

        return nullptr;
    }
}

uint16_t MelonDsDs::FlipLetterCase(PokeTypeKeyboard::Region region, uint16_t c) noexcept {
    if (c >= 'a' && c <= 'z') return c - 0x20;
    if (c >= 'A' && c <= 'Z') return c + 0x20;

    // Outside ASCII, the letters are the keys the layout lets Caps Lock change.
    // That leaves out keys like French é, whose shifted level isn't its capital.
    uint32_t count = 0;
    const PokeTypeKeyboard::KeyDesc* keys = PokeTypeKeyboard::GetKeyTable(region, count);
    for (uint32_t i = 0; keys && i < count; i++) {
        if (!keys[i].CapsSensitive) continue;
        if (keys[i].Base == c) return keys[i].Shift;
        if (keys[i].Shift == c) return keys[i].Base;
    }

    return c;
}

void TypingKeyboardState::SetConfig(const CoreConfig& config) noexcept {
    _enabled = config.TypingKeyboard();
    _autoPair = config.TypingAutoPair();
    _arrowsPressDpad = config.TypingArrowsPressDpad();
    _fnKey = config.TypingFnKey();
}

void TypingKeyboardState::SetActive(bool active) noexcept {
    _active = active;
    if (!active) {
        Reset();
    }
}

void TypingKeyboardState::KeyEvent(bool down, unsigned keycode, uint32_t character, uint16_t) noexcept {
    // A key let go of after typing was turned off still goes up
    if (!_active || (down && !_enabled)) return;

    uint16_t retroKey = static_cast<uint16_t>(keycode);
    uint8_t keyId = (keycode == _fnKey) ? FN : SpecialKeyId(keycode);

    if (keyId != 0) {
        Queue({KeystrokeKind::Key, down, retroKey, 0, keyId});
    }
    else if (down && character >= 0x20 && character != 0x7F && character <= 0xFFFF) {
        // Type whatever the host's layout made of the key,
        // which may not be on the same key of the game's keyboard
        Queue({KeystrokeKind::Typed, true, retroKey, static_cast<uint16_t>(character), 0});
    }
    else if (character == 0 && keycode > 0x20 && keycode < 0x7F) {
        // Some frontends (RetroArch on Windows, for one) never send the character,
        // only the key, whose RETROK code is the ASCII character on it.
        // A frontend that does send it sends none with the key's release.
        Queue({KeystrokeKind::Label, down, retroKey, static_cast<uint16_t>(keycode), 0});
    }
}

void TypingKeyboardState::Update() noexcept {
    // Polled rather than tracked from key events,
    // since frontends don't always send a release (RetroArch sends none when its window loses focus)
    _heldButtons = 0;
    if (!_active || !_enabled || !_arrowsPressDpad) return;

    // The game's menus only read the D-pad, never the keyboard's arrows
    if (retro::input_state(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT)) _heldButtons |= 1u << 4;
    if (retro::input_state(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT)) _heldButtons |= 1u << 5;
    if (retro::input_state(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP)) _heldButtons |= 1u << 6;
    if (retro::input_state(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN)) _heldButtons |= 1u << 7;
}

void TypingKeyboardState::Apply(melonDS::NDS& nds) noexcept {
    ZoneScopedN(TracyFunction);

    melonDS::NDSCart::CartCommon* cart = nds.GetNDSCart();
    if (!cart || cart->Type() != melonDS::NDSCart::CartType::RetailBT) return;

    // Whether the keyboard answers the game's search from power-on,
    // as if it had been switched on with Fn held
    static_cast<melonDS::NDSCart::CartRetailBT&>(*cart).SetAutoPair(_autoPair);

    PokeTypeKeyboard& keyboard = nds.PokeTypeKeyboard;
    ReleaseLostKeys(keyboard);

    Keystroke keystroke {};
    while (Dequeue(keystroke)) {
        switch (keystroke.Kind) {
            case KeystrokeKind::Key:
                if (keystroke.Down) {
                    Press(keyboard, keystroke.RetroKey, keystroke.KeyId);
                }
                else {
                    Release(keyboard, keystroke.RetroKey);
                }
                break;
            case KeystrokeKind::Label:
                if (!keystroke.Down) {
                    Release(keyboard, keystroke.RetroKey);
                }
                else if (const PokeTypeKeyboard::KeyDesc* key = FindKeyByLabel(keyboard.GetRegion(), keystroke.Character)) {
                    // The game's own layout and modifiers decide what it types
                    Press(keyboard, keystroke.RetroKey, key->KeyID);
                }
                break;
            case KeystrokeKind::Typed: {
                // There's no key to hold, so it's pressed and let go at once
                PokeTypeKeyboard::Keystroke typed {};
                uint16_t c = FlipLetterCase(keyboard.GetRegion(), keystroke.Character);
                if (PokeTypeKeyboard::FindKeystroke(keyboard.GetRegion(), c, typed)) {
                    keyboard.TypeKey(typed);
                }
                break;
            }
        }
    }
}

void TypingKeyboardState::Press(PokeTypeKeyboard& keyboard, uint16_t retroKey, uint8_t keyId) noexcept {
    for (size_t i = 0; i < _heldKeyCount; i++) {
        // A frontend repeating a held key's press
        if (_heldKeys[i].RetroKey == retroKey) return;
    }

    if (_heldKeyCount == _heldKeys.size()) return;

    _heldKeys[_heldKeyCount++] = {retroKey, keyId, false};
    keyboard.SetKey(keyId, true);
}

void TypingKeyboardState::Release(PokeTypeKeyboard& keyboard, uint16_t retroKey) noexcept {
    for (size_t i = 0; i < _heldKeyCount; i++) {
        if (_heldKeys[i].RetroKey != retroKey) continue;

        uint8_t keyId = _heldKeys[i].KeyId;
        _heldKeys[i] = _heldKeys[--_heldKeyCount];

        // Both Ctrl keys press the game's one Ctrl, for instance
        for (size_t j = 0; j < _heldKeyCount; j++) {
            if (_heldKeys[j].KeyId == keyId) return;
        }

        keyboard.SetKey(keyId, false);
        return;
    }
}

void TypingKeyboardState::ReleaseLostKeys(PokeTypeKeyboard& keyboard) noexcept {
    // RetroArch sends no release when its window loses focus, but stops reporting the key as held.
    // A key never reported held, by a frontend that doesn't or a tap between two polls,
    // waits for its release instead.
    for (size_t i = 0; i < _heldKeyCount;) {
        HeldKey& key = _heldKeys[i];
        if (retro::input_state(0, RETRO_DEVICE_KEYBOARD, 0, key.RetroKey)) {
            key.Polled = true;
        }
        else if (key.Polled) {
            Release(keyboard, key.RetroKey);
            continue;
        }
        i++;
    }
}

void TypingKeyboardState::Reset() noexcept {
    LockQueue();
    _queueHead = _queueTail;
    UnlockQueue();

    // The console's reset already let go of its keyboard's keys
    _heldKeyCount = 0;
    _heldButtons = 0;
}

void TypingKeyboardState::LockQueue() noexcept {
    bool unlocked = false;
    while (!_queueLocked.compare_exchange_weak(unlocked, true, std::memory_order_acquire)) {
        unlocked = false;
        std::this_thread::yield();
    }
}

void TypingKeyboardState::Queue(const Keystroke& keystroke) noexcept {
    LockQueue();
    size_t next = (_queueTail + 1) % QueueSize;
    // When full, drop the keystroke
    if (next != _queueHead) {
        _queue[_queueTail] = keystroke;
        _queueTail = next;
    }
    UnlockQueue();
}

bool TypingKeyboardState::Dequeue(Keystroke& keystroke) noexcept {
    LockQueue();
    bool any = _queueHead != _queueTail;
    if (any) {
        keystroke = _queue[_queueHead];
        _queueHead = (_queueHead + 1) % QueueSize;
    }
    UnlockQueue();
    return any;
}
