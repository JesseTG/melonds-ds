from typing import cast

from libretro import IterableSensorDriver
from libretro.api.input.device import InputDevice

import prelude

options = {
    b"melonds_slot2_device": b"solar1",
    b"melonds_solar_sensor_host_sensor": b"disabled",
}

with prelude.builder().with_options(options).build() as session:
    session.set_controller_port_device(0, InputDevice.JOYPAD)
    session.run()

    sensor = cast(IterableSensorDriver, session.sensor)
    assert isinstance(sensor, IterableSensorDriver), f"Expected an IterableSensorDriver, got {sensor}"

    assert not sensor.sensor_state[0].illuminance_enabled, "Expected the host sensor to be disabled"
