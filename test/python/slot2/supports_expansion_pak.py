from ctypes import c_uint32, CFUNCTYPE

import prelude

with prelude.session() as session:
    session.run()
    get_gba_cart_type = session.get_proc_address("melondsds_get_gba_cart_type", CFUNCTYPE(c_uint32))
    assert get_gba_cart_type is not None, "melondsds_get_gba_cart_type not found"

    cart_type = get_gba_cart_type()
    assert cart_type == 0x201, f"Expected a cart type of 0x201 (RAMExpansion), got {hex(cart_type)}"
