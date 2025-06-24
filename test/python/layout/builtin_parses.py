import os.path
from ctypes import CFUNCTYPE, c_bool, c_char_p

import prelude

with prelude.noload_session() as session:
    layout_toml_parses = session.get_proc_address("melondsds_layout_toml_parses", CFUNCTYPE(c_bool, c_char_p))

    assert layout_toml_parses is not None, "melondsds_layout_toml_parses not found"

    module_dir = os.path.dirname(__file__)
    config_path = os.path.join(module_dir, "../../../src/libretro/assets/screens.toml")
    config_path = os.path.normpath(config_path)
    with open(config_path) as config:
        config_data = config.read().encode("utf-8")
        ok = layout_toml_parses(config_data)
        assert ok, f"Failed to parse {config_path}"