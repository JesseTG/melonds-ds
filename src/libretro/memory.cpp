//
// Created by Jesse on 3/7/2023.
//

#include <NDS.h>
#include <ARCodeFile.h>
#include <AREngine.h>
#include <cstring>
#include "libretro.hpp"
#include "environment.hpp"
#include "memory.hpp"
#include "config.hpp"


#define MAX_SERIALIZE_TEST_SIZE 16 * 1024 * 1024 // The current savestate is around 7MiB so 16MiB should be enough for now

namespace AREngine
{
    extern void RunCheat(ARCode& arcode);
}

PUBLIC_SYMBOL size_t retro_serialize_size(void) {

        // Create the dummy savestate
        void *data = malloc(MAX_SERIALIZE_TEST_SIZE);
        Savestate *savestate = new Savestate(data, MAX_SERIALIZE_TEST_SIZE, true);
        NDS::DoSavestate(savestate);
        // Find the offset to find the current static filesize
        size_t size = savestate->GetOffset();
        // Free
        delete savestate;
        free(data);

        return size;


}

PUBLIC_SYMBOL bool retro_serialize(void *data, size_t size) {
        Savestate *savestate = new Savestate(data, size, true);
        NDS::DoSavestate(savestate);
        delete savestate;

        return true;

}

PUBLIC_SYMBOL bool retro_unserialize(const void *data, size_t size) {
        Savestate *savestate = new Savestate((void *) data, size, false);
        NDS::DoSavestate(savestate);
        delete savestate;

        return true;

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
