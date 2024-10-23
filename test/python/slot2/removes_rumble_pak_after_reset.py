import typing
from ctypes import CFUNCTYPE, c_uint32

from libretro import DictRumbleDriver, IterableInputDriver

import prelude

input_driver = IterableInputDriver(rumble=DictRumbleDriver())
options = {
    b"melonds_slot2_device": b"rumble-pak"
}

with prelude.builder().with_input(input_driver).with_options(options).build() as session:
    session.run()

    get_gba_cart_type = session.get_proc_address("melondsds_get_gba_cart_type", CFUNCTYPE(c_uint32))
    assert get_gba_cart_type is not None, "melondsds_get_gba_cart_type not found"

    cart_type = get_gba_cart_type()
    assert cart_type == 0x202, f"Expected a cart type of 0x202 (RumblePak), got {hex(cart_type)}"

    session.options.variables[b"melonds_slot2_device"] = b"auto"

    session.run()

    cart_type = get_gba_cart_type()
    assert cart_type == 0x202, f"Rumble Pak should not be removed while a game is running"

    session.reset()

    cart_type = get_gba_cart_type()
    assert cart_type == 0, f"Rumble Pak should be removed after a reset"
