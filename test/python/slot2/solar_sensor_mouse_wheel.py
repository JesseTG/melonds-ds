from ctypes import CFUNCTYPE, c_int32
from itertools import repeat

from libretro import MouseState

import prelude

options = {
    b"melonds_slot2_device": b"solar1"
}

def generate_input():
    yield 0
    yield from repeat(MouseState(wheel_up=True), 10)
    yield from repeat(MouseState(wheel_down=True), 5)
    yield from repeat(0)

with prelude.builder().with_options(options).with_input(generate_input).build() as session:
    session.run()

    get_solar_sensor_level = session.get_proc_address("melondsds_get_solar_sensor_level", CFUNCTYPE(c_int32))
    assert get_solar_sensor_level is not None, "melondsds_get_solar_sensor_level not found"

    light_level = get_solar_sensor_level()

    assert light_level == 0, f"Expected a light level of 0, got {light_level}"

    for i in range(10):
        session.run()

    assert light_level == 10, f"Expected a light level of 10, got {light_level}"

    for i in range(5):
        session.run()

    assert light_level == 5, f"Expected a light level of 5, got {light_level}"
