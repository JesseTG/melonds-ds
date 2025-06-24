from ctypes import CFUNCTYPE, c_bool, c_char_p

import prelude

# Add a comment that says "language=toml" before each TOML-containing string literal
# so that CLion recognizes and highlights the syntax inline.
# Unless the syntax is invalid, in which case we don't want to highlight it.

with prelude.noload_session() as session:
    layout_toml_parses = session.get_proc_address("melondsds_layout_toml_parses", CFUNCTYPE(c_bool, c_char_p))
    assert layout_toml_parses is not None, "melondsds_layout_toml_parses not found"

    def assert_bad_toml(toml_string: bytes):
        ok = layout_toml_parses(toml_string)
        assert not ok, f"Successfully parsed a string that should've failed: {toml_string}"

    EMPTY = b''
    assert_bad_toml(EMPTY)

    BAD_SYNTAX = b'''
    fgsfds
    '''
    assert_bad_toml(BAD_SYNTAX)

    # language=toml
    WRONG_NAME_TYPE = b'''
    [layout]
    name = 123
    layout.screens = [{position = {x = 0, y = 0}, type = "top", scale = 1}]
    '''
    assert_bad_toml(WRONG_NAME_TYPE)

    # language=toml
    MISSING_SCREEN_POSITION = b'''
    [layout]
    [[layout.screens]]
    type = "top"
    '''
    assert_bad_toml(MISSING_SCREEN_POSITION)

    # language=toml
    MISSING_SCREEN_TYPE = b'''
    [layout]
    [[layout.screens]]
    position = {x = 0, y = 0}
    '''
    assert_bad_toml(MISSING_SCREEN_TYPE)

    # language=toml
    WRONG_POSITION_TYPE = b'''
    [layout]
    [[layout.screens]]
    position = "string"
    type = "top"
    '''
    assert_bad_toml(WRONG_POSITION_TYPE)

    # language=toml
    WRONG_SCALE_TYPE = b'''
    [layout]
    [[layout.screens]]
    position = {x = 0, y = 0}
    type = "top"
    scale = {}
    '''
    assert_bad_toml(WRONG_SCALE_TYPE)

    # language=toml
    WRONG_ROTATION_VALUE = b'''
    [layout]
    rotation = "invalid"
    [[layout.screens]]
    position = {x = 0, y = 0}
    type = "top"
    '''
    assert_bad_toml(WRONG_ROTATION_VALUE)

    # language=toml
    WRONG_FILTER_VALUE = b'''
    [layout]
    [[layout.screens]]
    position = {x = 0, y = 0}
    type = "top"
    filter = "invalid"
    scale = 2
    '''
    assert_bad_toml(WRONG_FILTER_VALUE)

    # language=toml
    MISSING_SCREENS = b'''
    [layout]
    rotation = "left"
    name = "Missing Screens"
    '''
    assert_bad_toml(MISSING_SCREENS)

    # language=toml
    EMPTY_SCREENS = b'''
    [layout]
    rotation = "left"
    name = "Missing Screens"
    screens = []
    '''
    assert_bad_toml(EMPTY_SCREENS)