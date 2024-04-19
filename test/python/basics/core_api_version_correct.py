from libretro import Core

import prelude

core = Core(prelude.core_path)

print(f"api_version: {core.api_version()}")

assert core.api_version() == 1
