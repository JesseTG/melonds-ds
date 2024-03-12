import os
import shutil
import sys
import tempfile

import libretro

if not __debug__:
    raise RuntimeError("The melonDS DS test suite should not be run with -O")

SYSTEM_FILES = ("ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND")
testdir = tempfile.TemporaryDirectory(".libretro")
system_dir = os.path.join(testdir.name, "system")
save_dir = os.path.join(testdir.name, "savefiles")
save_directory = save_dir
savestate_directory = os.path.join(testdir.name, "states")
core_system_dir = os.path.join(system_dir, "melonDS DS")
core_save_dir = os.path.join(save_dir, "melonDS DS")

print("Test dir:", testdir.name)
print("System dir:", system_dir)
print("Save dir:", save_dir)
print("Savestate dir:", savestate_directory)
print("Core system dir:", core_system_dir)

os.makedirs(core_system_dir, exist_ok=True)
os.makedirs(core_save_dir, exist_ok=True)

for _f in SYSTEM_FILES:
    if _f in os.environ:
        basename = os.path.basename(os.environ[_f])
        targetpath = os.path.join(core_system_dir, basename)
        print(f"Copying {os.environ[_f]} to {targetpath}")
        shutil.copyfile(os.environ[_f], targetpath)

options_string = os.getenv("RETRO_CORE_OPTIONS")
core_path = sys.argv[1]
content_path = sys.argv[2] if len(sys.argv) > 2 and len(sys.argv[2]) > 0 else None

default_args = {
    "system_dir": system_dir,
    "save_dir": save_dir,
}

def session(**kwargs) -> libretro.Session:
    return libretro.default_session(core_path, content_path, **(default_args | kwargs))

def noload_session(**kwargs) -> libretro.Session:
    return libretro.default_session(core_path, libretro.session.DoNotLoad, **(default_args | kwargs))