from ctypes import CFUNCTYPE, c_uint32

import prelude

with prelude.session() as session:
    session.run()
    get_gba_cart_type = session.get_proc_address("melondsds_get_gba_cart_type", CFUNCTYPE(c_uint32))
    assert get_gba_cart_type is not None, "melondsds_get_gba_cart_type not found"

    cart_type = get_gba_cart_type()
    assert cart_type == 0x102, f"Expected a GameSolarSensor cart (0x102) to be inserted, got {hex(cart_type)}"
