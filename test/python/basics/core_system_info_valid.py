from libretro import Core

import prelude

core = Core(prelude.core_path)

system_info = core.get_system_info()

assert system_info is not None

print(f"library_name: {system_info.library_name}")
print(f"library_version: {system_info.library_version}")
print(f"valid_extensions: {system_info.valid_extensions}")
print(f"need_fullpath: {system_info.need_fullpath}")
print(f"block_extract: {system_info.block_extract}")

assert b"melonDS DS" == system_info.library_name

