from ctypes import *

from libretro import Session

import prelude
from libretro.api.power import retro_device_power, PowerState

power = retro_device_power(PowerState.DISCHARGING, 3540, 52)
session: Session
with prelude.builder().with_power(power).build() as session:
    get_power = session.get_proc_address(b"libretropy_get_power", CFUNCTYPE(bool, POINTER(retro_device_power)))

    assert get_power is not None
    returned_power = retro_device_power()
    assert get_power(byref(returned_power))
    assert power == returned_power, f"{power} != {returned_power}"
