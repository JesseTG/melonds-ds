from libretro import Core
from libretro.api.system import Region

import prelude

core = Core(prelude.core_path)

print(f"region: {core.get_region()!r}")

assert core.get_region() == Region.NTSC
