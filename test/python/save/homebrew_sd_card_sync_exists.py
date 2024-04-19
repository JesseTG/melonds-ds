import os
import stat

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    session.core.run()

    dldi_sdcard_stat = os.stat(prelude.dldi_sd_card_sync_path)

    assert stat.S_ISDIR(dldi_sdcard_stat.st_mode), "dldi_sd_card should be a directory"
