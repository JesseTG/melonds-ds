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
    // HID usages of the keys that type no printable character,
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

    constexpr uint8_t HID_SHIFT = 0x02;
    constexpr uint8_t HID_ALT_GR = 0x40;

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

    uint8_t HidModifiers(uint16_t modifiers, bool altGr) noexcept {
        uint8_t hid = 0;
        if (modifiers & RETROKMOD_CTRL) hid |= 0x01;
        if (modifiers & RETROKMOD_SHIFT) hid |= HID_SHIFT;
        if (modifiers & RETROKMOD_ALT) hid |= 0x04;
        if (altGr) hid |= HID_ALT_GR;
        return hid;
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

void TypingKeyboardState::KeyEvent(bool down, unsigned keycode, uint32_t character, uint16_t modifiers) noexcept {
    if (keycode == RETROK_RALT || keycode == RETROK_MODE) {
        _altGrHeld = down;
    }

    // Arrow keys press the D-pad from their polled state (see Update), so releases don't matter here
    if (!down || !_active || !_enabled) return;

    uint8_t hidModifiers = HidModifiers(modifiers, _altGrHeld);
    uint8_t keyId = (keycode == _fnKey) ? FN : SpecialKeyId(keycode);

    if (keyId == FN) {
        Queue({PokeTypeKeyboard::SpecialCharForKeyID(FN), FN, hidModifiers, KeystrokeKind::Pairing, false});
    }
    else if (keyId != 0) {
        Queue({PokeTypeKeyboard::SpecialCharForKeyID(keyId), keyId, hidModifiers, KeystrokeKind::Typed, false});
    }
    else if (character >= 0x20 && character != 0x7F && character <= 0xFFFF) {
        // Type whatever the host's layout made of the key
        Queue({static_cast<uint16_t>(character), 0, hidModifiers, KeystrokeKind::Typed, false});
    }
    else if (character == 0 && keycode > 0x20 && keycode < 0x7F) {
        // Some frontends (RetroArch on Windows, for one) never send the character,
        // only the key, whose RETROK code is the ASCII character on it
        bool capsLock = modifiers & RETROKMOD_CAPSLOCK;
        Queue({static_cast<uint16_t>(keycode), 0, hidModifiers, KeystrokeKind::Label, capsLock});
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

    auto& bt = static_cast<melonDS::NDSCart::CartRetailBT&>(*cart);
    PokeTypeKeyboard& game = nds.PokeTypeKeyboard;

    // Whether the keyboard answers the game's search from power-on,
    // as if it had been switched on with Fn held
    bt.SetAutoPair(_autoPair);

    Keystroke keystroke {};
    while (Dequeue(keystroke)) {
        switch (keystroke.Kind) {
            case KeystrokeKind::Pairing:
                // Once the game has connected, Fn types like any other key
                if (!bt.EnterPairingMode()) {
                    game.PushKeyID(keystroke.Character, keystroke.KeyId, keystroke.Modifiers);
                }
                break;
            case KeystrokeKind::Label:
                if (const PokeTypeKeyboard::KeyDesc* key = FindKeyByLabel(game.GetRegion(), keystroke.Character)) {
                    bool shift = keystroke.Modifiers & HID_SHIFT;
                    bool altGr = keystroke.Modifiers & HID_ALT_GR;
                    if (uint16_t c = PokeTypeKeyboard::CharForKey(*key, shift, altGr, keystroke.CapsLock)) {
                        game.PushKeyID(c, key->KeyID, keystroke.Modifiers);
                    }
                }
                break;
            case KeystrokeKind::Typed:
                if (keystroke.KeyId != 0) {
                    game.PushKeyID(keystroke.Character, keystroke.KeyId, keystroke.Modifiers);
                }
                else {
                    game.PushKey(FlipLetterCase(game.GetRegion(), keystroke.Character), keystroke.Modifiers);
                }
                break;
        }
    }
}

void TypingKeyboardState::Reset() noexcept {
    LockQueue();
    _queueHead = _queueTail;
    UnlockQueue();

    _altGrHeld = false;
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
    // When full, drop the keystroke, as the game's own queue would
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
