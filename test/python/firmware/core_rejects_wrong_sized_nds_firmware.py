import os
from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    definition = session.options.definitions[b"melonds_firmware_nds_path"]
    assert definition is not None, "melonds_firmware_nds_path should be defined"

    assert os.environ["ARM7_BIOS"] is not None

    arm9name = os.path.basename(os.environ["ARM9_BIOS"]).encode()
    assert arm9name is not None and arm9name != b""
    assert all(v.value is None or arm9name not in v.value for v in definition.values), \
        "ARM9_BIOS should not be in melonds_firmware_nds_path values"
    # Account for the empty entries in the values array
