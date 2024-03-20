from typing import cast
from libretro import Session
from libretro.api.log import UnformattedLogger

import prelude

session: Session
with prelude.session() as session:
    log: UnformattedLogger = cast(UnformattedLogger, session.log)
    assert log is not None
    assert log.records is not None
    assert len(log.records) > 0