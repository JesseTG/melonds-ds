import os

from libretro import Session

import prelude

nds_firmware_path = os.environ["NDS_FIRMWARE"]
nds_firmware_basename = os.path.basename(nds_firmware_path)
test_nds_firmware_path = os.path.join(prelude.core_system_dir, nds_firmware_basename.encode())

original_nds_firmware_size = os.stat(nds_firmware_path).st_size
assert original_nds_firmware_size > 0, f"{nds_firmware_path} is empty"

session: Session
with prelude.session() as session:
    for i in range(300):
        session.run()

    test_nds_firmware_size = os.stat(test_nds_firmware_path).st_size

    assert original_nds_firmware_size == test_nds_firmware_size, \
        f"Expected firmware to be {original_nds_firmware_size} bytes after saving, found {test_nds_firmware_size} bytes"
