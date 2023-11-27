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

//! Local multiplayer is not implemented in melonDS DS.

using namespace melonDS;
bool Platform::MP_Init() {
    return false;
}

void Platform::MP_DeInit() {
}

void Platform::MP_Begin() {
}

void Platform::MP_End() {
}

int Platform::MP_SendPacket(u8*, int, u64) {
    return 0;
}

int Platform::MP_RecvPacket(u8*, u64 *) {
    return 0;
}

int Platform::MP_SendCmd(u8*, int, u64) {
    return 0;
}

int Platform::MP_SendReply(u8*, int, u64, u16) {
    return 0;
}

int Platform::MP_SendAck(u8*, int, u64) {
    return 0;
}

int Platform::MP_RecvHostPacket(u8 *, u64 *) {
    return 0;
}

u16 Platform::MP_RecvReplies(u8 *, u64, u16) {
    return 0;
}