#!/usr/bin/env python

import configparser
import os
import os.path
import subprocess
import shutil
import sys
import tempfile
from typing import List

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

os.makedirs(os.path.join(system_dir, "melonDS DS"), exist_ok=True)
os.makedirs(save_dir, exist_ok=True)
os.makedirs(log_dir, exist_ok=True)
os.makedirs(screenshot_dir, exist_ok=True)
os.makedirs(savestate_directory, exist_ok=True)
os.makedirs(config_dir, exist_ok=True)
os.makedirs(core_option_dir, exist_ok=True)

# Create a bare-bones RetroArch config.
# RetroArch itself will add defaults for any missing options.
config_path = os.path.join(tempdir, "retroarch.cfg")
config = {
    "audio_driver": "null",
    "audio_enable": "false",
    "frontend_log_level": "0",
    "libretro_log_level": "0",
    "input_pause_toggle": "nul",
    "log_to_file": "true",
    "log_dir": log_dir,
    "log_to_file_timestamp": "false",
    "log_verbosity": "true",
    "network_cmd_enable": "true",
    "pause_nonactive": "false",
    "rgui_config_directory": config_dir,
    "system_directory": system_dir,
    "savefile_directory": save_dir,
    "savestate_directory": savestate_directory,
}

with open(config_path, "x") as f:
    # Need to create a config file here so RetroArch will use it as the root directory.
    for k, v in config.items():
        f.write(f"{k} = \"{v}\"\n")

with open(core_option_path, "x") as f:
    for e in sorted(os.environ):
        option_key = e.lower()
        if option_key.startswith("melonds_"):
            f.write(f"{option_key} = \"{os.environ[e]}\"\n")

logpath = os.path.join(tempdir, "logs", "retroarch.log")
with subprocess.Popen((retroarch,  f"--config={config_path}", "--verbose", f"--log-file={logpath}", *sys.argv[1:]), stdout=subprocess.PIPE, cwd=tempdir, text=True) as process:
    print(process.args)
    result = process.wait(30)

sys.exit(result)
