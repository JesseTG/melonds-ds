import os
import stat

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    session.core.run()

    dldi_sdcard_stat = os.stat(prelude.dldi_sd_card_path)

    assert stat.S_ISREG(dldi_sdcard_stat.st_mode), "dldi_sd_card.bin should be a regular file"
    assert dldi_sdcard_stat.st_size > 0, "dldi_sd_card.bin should exist and be non-empty"
