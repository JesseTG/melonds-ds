#!/usr/bin/env python

import os.path
import subprocess
import shutil
import sys

print("doge")

SYSTEM_FILES = ("ARM7_BIOS", "ARM9_BIOS", "ARM7_DSI_BIOS", "ARM9_DSI_BIOS", "NDS_FIRMWARE", "DSI_FIRMWARE", "DSI_NAND")
emutest_path = os.environ["EMUTEST"]

assert emutest_path is not None
assert os.path.isabs(emutest_path)
assert os.path.isfile(emutest_path)

emutest_dir = os.path.join(os.path.expanduser("~"), ".emutest")
print("Test dir:", emutest_dir)

system_dir = os.path.join(emutest_dir, "system")
save_dir = os.path.join(emutest_dir, "savefiles")
savestate_directory = os.path.join(emutest_dir, "states")
core_system_dir = os.path.join(system_dir, "melonDS DS")

shutil.rmtree(system_dir, True)
shutil.rmtree(save_dir, True)
shutil.rmtree(savestate_directory, True)

os.makedirs(core_system_dir, exist_ok=True)
os.makedirs(save_dir, exist_ok=True)
os.makedirs(savestate_directory, exist_ok=True)

for s in SYSTEM_FILES:
    if s in os.environ:
        sysfile = os.environ[s]
        assert os.path.exists(sysfile)
        shutil.copy2(sysfile, os.path.join(core_system_dir, os.path.basename(sysfile)))


def emutest():
    with subprocess.Popen((emutest_path, *sys.argv[1:]), cwd=emutest_dir, text=True) as process:
        print(process.args)
        result = process.wait(15)

    return result


if __name__ == "__main__":
    returnCode = emutest()

    sys.exit(returnCode)
