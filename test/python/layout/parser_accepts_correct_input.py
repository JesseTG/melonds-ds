from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.noload_session() as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    is_valid_vfl = session.get_proc_address(b"melondsds_is_valid_vfl", CFUNCTYPE(c_bool, c_char_p))
    assert is_valid_vfl is not None

    assert is_valid_vfl(b"[topScreen]-[bottomScreen]")
    assert is_valid_vfl(b"[topScreen]-[bottomScreen];[topScreen]-[bottomScreen(>=width)]")
    assert is_valid_vfl(b"[topScreen]-wtf-[bottomScreen]")
    assert is_valid_vfl(b"[button(>=50)]")
    assert is_valid_vfl(b"|-50-[purpleBox]-50-|")
    assert is_valid_vfl(b"V:[topField]-10-[bottomField]")
    assert is_valid_vfl(b"[maroonView][blueView]")
    assert is_valid_vfl(b"[button(100@20)]")
    assert is_valid_vfl(b"[super(400)]")
    assert is_valid_vfl(b"[dogs(100@spain)]")
    assert is_valid_vfl(b"[button1(==button2)]")
    assert is_valid_vfl(b"[flexibleButton(>=70,<=100)]")
    assert is_valid_vfl(b"|-[find]-[findNext]-[findField(>=20)]-|")

    assert False

    # TODO: Write test cases for valid VFL strings
