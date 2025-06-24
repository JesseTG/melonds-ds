from ctypes import CFUNCTYPE, c_bool, c_char_p

import prelude

# Add a comment that says "language=toml" before each TOML-containing string literal
# so that CLion recognizes and highlights the syntax inline.

with prelude.noload_session() as session:
    layout_toml_parses = session.get_proc_address("melondsds_layout_toml_parses", CFUNCTYPE(c_bool, c_char_p))
    assert layout_toml_parses is not None, "melondsds_layout_toml_parses not found"

    # language=toml
    BASIC = b'''
    [top]
    name = "Top Only"
    screens = [{position = {x = 0, y = 0}, type = "top", scale = 1}]
    '''
    ok = layout_toml_parses(BASIC)
    assert ok, f"Failed to parse: {BASIC}"

    # language=toml
    BASIC_ALT_SYNTAX = b'''
    top.name = "Top Only"
    top.screens = [{position = {x = 0, y = 0}, type = "top", scale = 1}]
    '''
    ok = layout_toml_parses(BASIC_ALT_SYNTAX)
    assert ok, f"Failed to parse: {BASIC_ALT_SYNTAX}"
