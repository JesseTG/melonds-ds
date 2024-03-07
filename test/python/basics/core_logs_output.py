from typing import cast
from libretro import default_session
from libretro.callback.log import StandardLogger

import prelude

with default_session(prelude.core_path, system_dir=prelude.core_system_dir) as session:
    log: StandardLogger = cast(StandardLogger, session.log)
    assert log is not None
    assert log.records is not None
    assert len(log.records) > 0
