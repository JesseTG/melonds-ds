from sys import argv
from typing import cast
from libretro import default_session
from libretro.callback.log import StandardLogger

import prelude

with default_session(argv[1], system_dir=prelude.core_system_dir) as session:
    log: StandardLogger = cast(StandardLogger, session.log)
    assert log is not None
    assert log.records is not None
    assert len(log.records) > 0
