from sys import argv
from libretro import Core

core = Core(argv[1])

assert core.api_version() == 1
