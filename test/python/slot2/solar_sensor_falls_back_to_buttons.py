from ctypes import CFUNCTYPE, c_int32
from itertools import repeat

from libretro import JoypadState
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

    get_solar_sensor_level = session.get_proc_address("melondsds_get_solar_sensor_level", CFUNCTYPE(c_int32))
    assert get_solar_sensor_level is not None, "melondsds_get_solar_sensor_level not found"

    light_level = get_solar_sensor_level()
    assert light_level == 0, f"Expected a light level of 0, got {light_level}"

    for i in range(20):
        session.run()

    light_level = get_solar_sensor_level()
    assert light_level > 0, f"Expected a positive light level, got {light_level}"