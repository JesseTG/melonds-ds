import itertools
from libretro import default_session
from libretro.callback.input import JoypadState

import prelude


def generate_input():
    yield from itertools.repeat(0, 180)

    yield JoypadState(a=True)

    yield from itertools.repeat(0)


with default_session(prelude.core_path, system_dir=prelude.system_dir, input_state=generate_input) as session:

    for i in range(360):
        session.core.run()

    assert False
    # TODO: Assert that I'm in the main menu
