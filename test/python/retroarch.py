#!/usr/bin/env python

import configparser
import os
import os.path
import subprocess
import shutil
import sys
import tempfile
from typing import List

SYSTEM_FILES = ("ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND")
retroarch = os.environ["RETROARCH"]

assert retroarch is not None
assert os.path.isabs(retroarch)
assert os.path.isfile(retroarch)

# Create a temporary directory, so that we don't mess with the user's config.
# (Also, this helps us parallelize tests.)
tempdir = tempfile.mkdtemp()
print("Test dir:", tempdir)


system_dir = os.path.join(tempdir, "system")
save_dir = os.path.join(tempdir, "saves")
log_dir = os.path.join(tempdir, "logs")
screenshot_dir = os.path.join(tempdir, "screenshots")
savestate_directory = os.path.join(tempdir, "states")
config_dir = os.path.join(tempdir, "config")
core_option_dir = os.path.join(config_dir, "melonDS DS")
core_option_path = os.path.join(core_option_dir, "melonDS DS.opt")
core_system_dir = os.path.join(system_dir, "melonDS DS")

os.makedirs(core_system_dir, exist_ok=True)
os.makedirs(save_dir, exist_ok=True)
os.makedirs(log_dir, exist_ok=True)
os.makedirs(screenshot_dir, exist_ok=True)
os.makedirs(savestate_directory, exist_ok=True)
os.makedirs(config_dir, exist_ok=True)
os.makedirs(core_option_dir, exist_ok=True)

for s in SYSTEM_FILES:
    if s in os.environ:
        sysfile = os.environ[s]
        assert os.path.exists(sysfile)
        shutil.copy2(sysfile, os.path.join(system_dir, os.path.basename(sysfile)))

# Create a bare-bones RetroArch config.
# RetroArch itself will add defaults for any missing options.
config_path = os.path.join(tempdir, "retroarch.cfg")
config = {
    "audio_driver": "null",
    "audio_enable": "false",
    "camera_driver": "null",
    "frontend_log_level": "0",
    "libretro_log_level": "0",
    "input_pause_toggle": "nul",
    "log_to_file": "true",
    "log_dir": log_dir,
    "log_to_file_timestamp": "false",
    "log_verbosity": "true",
    "microphone_driver": "null",
    "midi_driver": "null",
    "network_cmd_enable": "true",
    "pause_nonactive": "false",
    "rgui_config_directory": config_dir,
    "system_directory": system_dir,
    "savefile_directory": save_dir,
    "savestate_directory": savestate_directory,
}

with open(core_option_path, "x") as core_config, open(config_path, "x") as retroarch_config:
    # Need to create a config file here so RetroArch will use it as the root directory.
    for k, v in config.items():
        retroarch_config.write(f"{k} = \"{v}\"\n")

    for e in sorted(os.environ):
        key = e.lower()
        if key.startswith("melonds_"):
            core_config.write(f"{key} = \"{os.environ[e]}\"\n")
        elif key.startswith("retroarch_"):
            retroarch_config.write(f"{key.removeprefix('retroarch_')} = \"{os.environ[e]}\"\n")

logpath = os.path.join(tempdir, "logs", "retroarch.log")
with subprocess.Popen((retroarch,  f"--config={config_path}", "--verbose", f"--log-file={logpath}", *sys.argv[1:]), stdout=subprocess.PIPE, cwd=tempdir, text=True) as process:
    print(process.args)
    result = process.wait(30)

sys.exit(result)
