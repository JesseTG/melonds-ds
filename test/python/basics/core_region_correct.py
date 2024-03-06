from sys import argv
from libretro import Core
from libretro.defs import Region

core = Core(argv[1])

assert core.get_region() == Region.NTSC
