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

using namespace melonDS;

/// An AAC decoder, as used by the DSi's DSP.
///
/// melonDS only calls the \c Platform::AAC_ functions
/// from its HLE implementation of the AAC ucode,
/// which melonDS DS doesn't currently enable;
/// \c melonDS::DSiArgs::DSPHLE is left at its default of \c false.
/// They still have to be defined,
/// because melonDS compiles its DSP HLE sources unconditionally.
///
/// TODO: Implement these once DSP HLE is exposed as a core option.
///  libretro-common's raac (see \c <formats/raac.h>) is a suitable AAC-LC backend;
///  add \c formats/aac/raac.c to \c cmake/libretro-common.cmake to build it.
///  Two things to watch out for:
///
///  - \c raac_open takes an MPEG-4 AudioSpecificConfig,
///    but melonDS provides a sample rate and a channel count.
///    A two-byte ASC is enough to describe an AAC-LC stream:
///    \c asc[0]=(2<<3)|(freqIndex>>1) and \c asc[1]=((freqIndex&1)<<7)|(channels<<3),
///    where \c freqIndex is the ISO/IEC 14496-3 sampling frequency index.
///    Don't copy melonDS's own five-byte ASC;
///    its trailing SBR syncExtension makes raac emit 2048 samples per channel,
///    which is twice what melonDS's output buffer can hold.
///  - melonDS reads mono output as if it were interleaved stereo,
///    keeping every other sample.
///    Match that behavior instead of "fixing" it.
struct melonDS::Platform::AACDecoder
{
};

/// \returns \c nullptr, always.
/// melonDS logs this and carries on without a decoder.
Platform::AACDecoder* Platform::AAC_Init()
{
    return nullptr;
}

void Platform::AAC_DeInit(Platform::AACDecoder* dec)
{
}

/// \note melonDS calls this even if \c AAC_Init returned \c nullptr,
/// so \c dec may be null.
/// \returns \c false, always.
bool Platform::AAC_Configure(Platform::AACDecoder* dec, int frequency, int channels)
{
    return false;
}

/// \note melonDS calls this even if \c AAC_Init returned \c nullptr,
/// so \c dec may be null.
/// \returns \c false, always.
bool Platform::AAC_DecodeFrame(Platform::AACDecoder* dec, const void* input, int inputlen, void* output, int outputlen)
{
    return false;
}
