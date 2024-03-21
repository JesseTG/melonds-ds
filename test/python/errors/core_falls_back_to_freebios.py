from ctypes import c_bool, CFUNCTYPE

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    console_exists = session.get_proc_address("melondsds_console_exists", CFUNCTYPE(c_bool))
    assert console_exists is not None
    assert console_exists()

    arm7_native = session.get_proc_address("melondsds_arm7_bios_native", CFUNCTYPE(c_bool))
    assert arm7_native is not None
    assert not arm7_native()

    arm9_native = session.get_proc_address("melondsds_arm9_bios_native", CFUNCTYPE(c_bool))
    assert arm9_native is not None
    assert not arm9_native()

    firmware_native = session.get_proc_address("melondsds_firmware_native", CFUNCTYPE(c_bool))
    assert firmware_native is not None
    assert not firmware_native()
