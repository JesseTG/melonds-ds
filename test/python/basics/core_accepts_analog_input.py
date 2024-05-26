import itertools
from collections.abc import Callable
from ctypes import c_int, CFUNCTYPE

from libretro import AnalogState

import prelude

options = {
    b"melonds_show_cursor": b"enabled"
}

def generate_input():
    yield from itertools.repeat(0, 2)
    yield from itertools.repeat(AnalogState(rstick=(2000, 150)), 25)
    yield from itertools.repeat(0)


with prelude.builder().with_input(generate_input).with_options(options).build() as session:
    cursor_x: Callable[[], int] = session.get_proc_address(b"melondsds_analog_cursor_x", CFUNCTYPE(c_int))
    assert cursor_x is not None
    cursor_y: Callable[[], int] = session.get_proc_address(b"melondsds_analog_cursor_y", CFUNCTYPE(c_int))
    assert cursor_y is not None

    initial_cursor_pos = cursor_x(), cursor_y()

    for i in range(360):
        session.run()

    current_cursor_pos = cursor_x(), cursor_y()

    assert initial_cursor_pos != current_cursor_pos, f"Cursor didn't move from initial position of {initial_cursor_pos}"

