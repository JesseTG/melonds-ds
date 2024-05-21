import os
import shutil
import sys
import tempfile

import libretro
from libretro import SubsystemContent, SessionBuilder, DefaultPathDriver, PillowVideoDriver

if not __debug__:
    raise RuntimeError("The melonDS DS test suite should not be run with -O")

SYSTEM_FILES = ("ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND")
testdir = tempfile.TemporaryDirectory(".libretro")
system_dir = os.path.join(testdir.name, "system")
assets_dir = os.path.join(testdir.name, "assets")
playlist_dir = os.path.join(testdir.name, "playlist")
save_dir = os.path.join(testdir.name, "savefiles")
save_directory = save_dir
savestate_directory = os.path.join(testdir.name, "states")
core_system_dir = os.path.join(system_dir, "melonDS DS")
core_save_dir = os.path.join(save_dir, "melonDS DS")
wfcsettings_path = os.path.join(core_system_dir, "wfcsettings.bin")
dldi_sd_card_path = os.path.join(core_save_dir, "dldi_sd_card.bin")
dldi_sd_card_sync_path = os.path.join(core_save_dir, "dldi_sd_card")

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
subsystem = os.getenv("SUBSYSTEM")
core_path = sys.argv[1]
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
        .with_paths(
            DefaultPathDriver(
                corepath=core_path,
                system=system_dir,
                assets=assets_dir,
                save=save_dir,
                playlist=playlist_dir,
            )
        )
        .with_options(options)
    )


def session(**kwargs) -> libretro.Session:
    return builder(**kwargs).build()


def noload_session(**kwargs) -> libretro.Session:
    return builder(**kwargs).with_content_driver(None).build()