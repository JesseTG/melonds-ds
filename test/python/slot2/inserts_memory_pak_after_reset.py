import typing
from ctypes import CFUNCTYPE, c_uint32

from libretro import DictRumbleDriver, IterableInputDriver

import prelude

input_driver = IterableInputDriver(rumble=DictRumbleDriver())
options = {
    b"melonds_slot2_device": b"auto"
}

with prelude.builder().with_input(input_driver).with_options(options).build() as session:
    session.run()

    get_gba_cart_type = session.get_proc_address("melondsds_get_gba_cart_type", CFUNCTYPE(c_uint32))
    assert get_gba_cart_type is not None, "melondsds_get_gba_cart_type not found"

    cart_type = get_gba_cart_type()
    assert cart_type == 0, f"Expected no GBA cart to be inserted, got {hex(cart_type)}"

    session.options.variables[b"melonds_slot2_device"] = b"expansion-pak"

    session.run()

    cart_type = get_gba_cart_type()
    assert cart_type == 0, f"Expansion Pak should not be inserted while a game is running"

    session.reset()

    cart_type = get_gba_cart_type()
    assert cart_type == 0x201, f"Expansion Pak should be inserted after a reset if enabled in the options"
