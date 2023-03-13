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

#include <NDS.h>
#include <ARCodeFile.h>
#include <AREngine.h>
#include <cstring>
#include <ExternalBufferSavestate.h>
#include "libretro.hpp"
#include "environment.hpp"
#include "memory.hpp"
#include "config.hpp"


namespace AREngine {
    extern void RunCheat(ARCode &arcode);
}

namespace melonds {
    static u8* _savestate_buffer = nullptr;
    static size_t _savestate_buffer_length = 0;
}

PUBLIC_SYMBOL size_t retro_serialize_size(void) {

    // Create the dummy savestate
    void *data = malloc(melonds::DEFAULT_SERIALIZE_TEST_SIZE);

    ExternalBufferSavestate savestate((u8*)data, melonds::DEFAULT_SERIALIZE_TEST_SIZE, true);
    NDS::DoSavestate(&savestate);
    // Find the offset to find the current static filesize
    size_t size = savestate.BufferOffset();
    // Free
    // TODO: If the save state didn't fit, double the buffer size and try again
    free(data);

    return size;


}

PUBLIC_SYMBOL bool retro_serialize(void *data, size_t size) {
    ExternalBufferSavestate savestate((u8*)data, size, true);
    NDS::DoSavestate(&savestate);

    return !savestate.Error;
}

PUBLIC_SYMBOL bool retro_unserialize(const void *data, size_t size) {
    ExternalBufferSavestate savestate((u8*)data, size, false);
    NDS::DoSavestate(&savestate);

    return !savestate.Error;
}

PUBLIC_SYMBOL void *retro_get_memory_data(unsigned type) {
    if (type == RETRO_MEMORY_SYSTEM_RAM)
        return NDS::MainRAM;
    else
        return nullptr;
}

PUBLIC_SYMBOL size_t retro_get_memory_size(unsigned type) {
    if (type == RETRO_MEMORY_SYSTEM_RAM)
        return 0x400000;
    else
        return 0;
}

PUBLIC_SYMBOL void retro_cheat_reset(void) {}

PUBLIC_SYMBOL void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    if (!enabled)
        return;
    ARCode curcode;
    std::string str(code);
    char *pch = &*str.begin();
    curcode.Name = code;
    curcode.Enabled = enabled;
    curcode.CodeLen = 0;
    pch = strtok(pch, " +");
    while (pch != nullptr) {
        curcode.Code[curcode.CodeLen] = (u32) strtol(pch, nullptr, 16);
        retro::log(RETRO_LOG_INFO, "Adding Code %s (%d) \n", pch, curcode.Code[curcode.CodeLen]);
        curcode.CodeLen++;
        pch = strtok(nullptr, " +");
    }
    AREngine::RunCheat(curcode);
}

void melonds::init_savestate_buffer(size_t length) {
    u8* realloced_buffer = (u8*)realloc(_savestate_buffer, length);
    // TODO: If this failed, log an error
    _savestate_buffer = realloced_buffer;
    _savestate_buffer_length = length;

}

void melonds::free_savestate_buffer() {
    if (_savestate_buffer) {
        free(_savestate_buffer);
        _savestate_buffer = nullptr;
        _savestate_buffer_length = 0;
    }
}