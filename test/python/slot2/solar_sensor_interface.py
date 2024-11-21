from ctypes import CFUNCTYPE, c_int32
from itertools import repeat
from typing import cast

from libretro import IterableSensorDriver

import prelude

options = {
    b"melonds_slot2_device": b"solar1",
    b"melonds_solar_sensor_host_sensor": b"enabled",
}

def generate_sensor_readings():
    yield 0
    yield from repeat(32000, 10) # Direct sunlight
    yield from repeat(400, 10) # Sunset
    yield from repeat(0)

with prelude.builder().with_options(options).with_sensor(generate_sensor_readings).build() as session:
    sensor = cast(IterableSensorDriver, session.sensor)
    assert sensor is not None, "Sensor interface not found"

    assert sensor.sensor_state[0].illuminance_enabled, "Illuminance sensor should be enabled after loading the game"

    session.run()

    get_solar_sensor_level = session.get_proc_address("melondsds_get_solar_sensor_level", CFUNCTYPE(c_int32))
    assert get_solar_sensor_level is not None, "melondsds_get_solar_sensor_level not found"

    light_level = get_solar_sensor_level()

    assert light_level == 0, f"Expected a light level of 0, got {light_level}"

    for i in range(10):
        session.run()

    light_level = get_solar_sensor_level()

    assert light_level > 0, f"Expected a positive light level, got {light_level}"

    for i in range(5):
        session.run()

    light_level = get_solar_sensor_level()

    assert 0 < light_level <= 10, f"Expected a light level between 0 (exclusive) and 10 (inclusive), got {light_level}"
