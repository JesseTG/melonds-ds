from ctypes import CFUNCTYPE, c_int32
from itertools import repeat
from typing import cast

from libretro import JoypadState, LoggerMessageInterface
from libretro.api.input.device import InputDevice

import prelude

options = {
    b"melonds_slot2_device": b"solar1",
    b"melonds_solar_sensor_host_sensor": b"enabled",
}

def generate_input():
    yield 0

    for i in range(10):
        yield JoypadState(select=True, up=True)
        yield None

    yield from repeat(None)

with prelude.builder().with_options(options).with_input(generate_input).with_sensor(None).build() as session:
    session.set_controller_port_device(0, InputDevice.JOYPAD)
    session.run()

    message = cast(LoggerMessageInterface, session.message)
    assert isinstance(message, LoggerMessageInterface), "LoggerMessageInterface not found"

    assert any(b"luminance" in m.msg for m in message.message_exts), "Expected the fallback message to be logged"
