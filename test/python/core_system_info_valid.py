from sys import argv
from libretro import Core

core = Core(argv[1])

system_info = core.get_system_info()

assert system_info is not None
assert b"melonDS DS" == system_info.library_name
