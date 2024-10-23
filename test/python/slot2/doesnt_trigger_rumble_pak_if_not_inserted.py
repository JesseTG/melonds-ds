import typing

from libretro import DictRumbleDriver, IterableInputDriver

import prelude

input_driver = IterableInputDriver(rumble=DictRumbleDriver())

with prelude.builder().with_input(input_driver).build() as session:
    session.run()

    driver = typing.cast(DictRumbleDriver, session.input.rumble)
    assert isinstance(driver, DictRumbleDriver), f"Expected a DictRumbleDriver, got {type(driver).__name__}"

    state = driver[0]

    assert state is not None, "Expected a rumble state, got None"
    assert state.strong == 0, f"Expected a rumble state with no strong motor active, got {state.strong}"
    assert state.weak == 0, f"Expected a rumble state with no weak motor active, got {state.weak}"
