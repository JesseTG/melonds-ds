from typing import cast
from libretro import Session, UnformattedLogDriver

import prelude

session: Session
with prelude.session() as session:
    log: UnformattedLogDriver = cast(UnformattedLogDriver, session.log)
    assert log is not None
    assert log.records is not None
    assert len(log.records) > 0
