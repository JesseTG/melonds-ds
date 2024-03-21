import os

from libretro import Session

import prelude

nds_firmware_path = os.environ["NDS_FIRMWARE"]
nds_firmware_basename = os.path.basename(nds_firmware_path)
dsi_firmware_path = os.environ["DSI_FIRMWARE"]
dsi_firmware_basename = os.path.basename(dsi_firmware_path)

with open(nds_firmware_path, "rb") as firmware_file:
    nds_firmware = firmware_file.read()

with open(dsi_firmware_path, "rb") as firmware_file:
    dsi_firmware = firmware_file.read()

assert nds_firmware != dsi_firmware, "DS and DSi firmware are the same"

session: Session
with prelude.session() as session:
    for i in range(30):
        session.core.run()

    match prelude.options[b'melonds_console_mode']:
        case 'ds' | b'ds':
            print("Switching to DSi mode")
            session.options.variables['melonds_console_mode'] = b'dsi'
        case 'dsi' | b'dsi':
            print("Switching to DS mode")
            session.options.variables['melonds_console_mode'] = b'ds'
        case _ as mode:
            raise ValueError(f"Unknown console mode {mode}")

    session.core.reset()

    for i in range(30):
        session.core.run()

nds_firmware_path_local = os.path.join(prelude.core_system_dir, nds_firmware_basename)
dsi_firmware_path_local = os.path.join(prelude.core_system_dir, dsi_firmware_basename)

with open(nds_firmware_path_local, "rb") as nds_firmware_file_local:
    nds_firmware_after = nds_firmware_file_local.read()

with open(dsi_firmware_path_local, "rb") as dsi_firmware_file_local:
    dsi_firmware_after = dsi_firmware_file_local.read()

assert len(nds_firmware_after) == len(nds_firmware), \
    f"DS firmware size changed from {len(nds_firmware)} to {len(nds_firmware_after)}"

assert len(dsi_firmware_after) == len(dsi_firmware), \
    f"DSi firmware size changed from {len(dsi_firmware)} to {len(dsi_firmware_after)}"

assert nds_firmware_after != dsi_firmware_after, "Firmware was overwritten after switching console mode"
