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
#include <net/net_socket.h>

bool Platform::MP_Init() {
    // TODO: Implement
    return false;
}

void Platform::MP_DeInit() {
    // TODO: Implement
}

void Platform::MP_Begin() {
    // TODO: Implement
}

void Platform::MP_End() {
    // TODO: Implement
}

int Platform::MP_SendPacket(u8 *data, int len, u64 timestamp) {
    // TODO: Implement
    return 0;
}

int Platform::MP_RecvPacket(u8 *data, u64 *timestamp) {
    // TODO: Implement
    return 0;
}

int Platform::MP_SendCmd(u8 *data, int len, u64 timestamp) {
    // TODO: Implement
    return 0;
}

int Platform::MP_SendReply(u8 *data, int len, u64 timestamp, u16 aid) {
    // TODO: Implement
    return 0;
}

int Platform::MP_SendAck(u8 *data, int len, u64 timestamp) {
    // TODO: Implement
    return 0;
}

int Platform::MP_RecvHostPacket(u8 *data, u64 *timestamp) {
    // TODO: Implement
    return 0;

}

u16 Platform::MP_RecvReplies(u8 *data, u64 timestamp, u16 aidmask) {
    // TODO: Implement
    return 0;
}