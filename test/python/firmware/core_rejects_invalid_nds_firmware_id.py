import os
from libretro import Session

import prelude

with open(os.environ["NDS_FIRMWARE"], "rb") as f:
    firmware = bytearray(f.read())

assert len(firmware) == 262144
firmware[0x8:0xC] = b"NNNN"

badfirmwarepath = os.path.join(prelude.core_system_dir, "badfirmware.bin")
with open(badfirmwarepath, "wb") as f:
    f.write(firmware)

session: Session
with prelude.session() as session:
    definition = session.options.definitions[b"melonds_firmware_nds_path"]
    assert definition is not None, "melonds_firmware_nds_path should be defined"

    assert all(v.value is None or b"badfirmware.bin" not in v.value for v in definition.values), \
        "ARM9_BIOS should not be in melonds_firmware_nds_path values"
    # Account for the empty entries in the values array
