/*
    Copyright 2026 Jesse Talavera

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

#include "../core/core.hpp"
#include "../microphone.hpp"
#include "tracy.hpp"

void melonDS::Platform::Mic_Start(void* userdata)
{
    // no-op for now
}

void melonDS::Platform::Mic_Stop(void* userdata)
{
    // no-op for now
}

/// melonDS asks for samples as it needs them,
/// rather than being given a frame's worth at a time.
/// It's okay to provide fewer samples than requested (including none at all);
/// melonDS will reuse the most recent sample for the remainder.
int melonDS::Platform::Mic_ReadInput(s16* data, int maxlength, void* userdata)
{
    ZoneScopedN(TracyFunction);
    if (!data || maxlength <= 0)
        return 0;

    MelonDsDs::CoreState& core = *reinterpret_cast<MelonDsDs::CoreState*>(userdata);
    return core.GetMicrophoneState().Read(std::span(data, static_cast<size_t>(maxlength)));
}
