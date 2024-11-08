from ctypes import CFUNCTYPE, c_uint32

import prelude

options = {
    b"melonds_slot2_device": b"auto"
}

with prelude.builder().with_options(options).build() as session:
    session.run()

    get_gba_cart_type = session.get_proc_address("melondsds_get_gba_cart_type", CFUNCTYPE(c_uint32))
    assert get_gba_cart_type is not None, "melondsds_get_gba_cart_type not found"

    cart_type = get_gba_cart_type()
    assert cart_type == 0, f"Expected no GBA cart to be inserted, got {hex(cart_type)}"

    session.options.variables[b"melonds_slot2_device"] = b"solar1"

    session.run()

    cart_type = get_gba_cart_type()
    assert cart_type == 0, f"Solar Sensor should not be inserted while a game is running"

    session.reset()

    cart_type = get_gba_cart_type()
    assert cart_type == 0x102, f"Solar Sensor should be inserted after a reset if enabled in the options"
