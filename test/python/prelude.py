import os
import shutil
import sys

import libretro
from libretro import SubsystemContent, SessionBuilder, TempDirPathDriver

if not __debug__:
    raise RuntimeError("The melonDS DS test suite should not be run with -O")

SYSTEM_FILES = ("ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND")
core_path = sys.argv[1]
path_driver = TempDirPathDriver(core_path, ".libretro")
testdir = path_driver.root_dir
system_dir = path_driver.system_dir
assets_dir = path_driver.core_assets_dir
playlist_dir = path_driver.playlist_dir
save_dir = path_driver.save_dir
save_directory = save_dir
savestate_directory = os.path.join(testdir, b"states")
core_system_dir = os.path.join(system_dir, b"melonDS DS")
core_save_dir = os.path.join(save_dir, b"melonDS DS")
wfcsettings_path = os.path.join(core_system_dir, b"wfcsettings.bin")
dldi_sd_card_path = os.path.join(core_save_dir, b"dldi_sd_card.bin")
dldi_sd_card_sync_path = os.path.join(core_save_dir, b"dldi_sd_card")

print("Test dir:", testdir)
print("System dir:", system_dir)
print("Save dir:", save_dir)
print("Savestate dir:", savestate_directory)
print("Core system dir:", core_system_dir)

os.makedirs(core_system_dir, exist_ok=True)
os.makedirs(core_save_dir, exist_ok=True)

for _f in SYSTEM_FILES:
    if _f in os.environ:
        basename = os.path.basename(os.environ[_f])
        targetpath = os.path.join(core_system_dir, basename.encode())
        print(f"Copying {os.environ[_f]} to {targetpath}")
        shutil.copyfile(os.environ[_f], targetpath)

options_string = os.getenv("RETRO_CORE_OPTIONS")
subsystem = os.getenv("SUBSYSTEM")
content_path = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] else None
content_paths = tuple(s for s in sys.argv[2:] if s) if len(sys.argv) > 2 else ()

options = {
    k.lower().encode(): v.encode()
    for k, v in os.environ.items() if k.lower().startswith("melonds_")
}

default_args = {
    "system_dir": system_dir,
    "save_dir": save_dir,
    "options": options,
}


def builder(**kwargs) -> SessionBuilder:
    content = None
    match content_paths:
        case [] | None:
            pass
        case [path]:
            content = path
        case [*paths]:
            content = SubsystemContent(subsystem, paths)
        case _:
            raise TypeError(f"Unexpected content_paths {type(content_paths).__name__}")

    return (
        libretro
        .defaults(core_path)
        .with_content(content)
        .with_paths(path_driver)
        .with_options(options)
    )


def session(**kwargs) -> libretro.Session:
    return builder(**kwargs).build()


def noload_session(**kwargs) -> libretro.Session:
    return builder(**kwargs).with_content_driver(None).build()