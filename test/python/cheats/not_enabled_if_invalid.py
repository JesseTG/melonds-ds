from typing import cast

from libretro import Session, LoggerMessageInterface, LogLevel

import prelude

session: Session
with prelude.session() as session:
    message = cast(LoggerMessageInterface, session.message)
    session.core.cheat_set(0, True, b'fgsfds')

    assert message.message_exts[-1].level == LogLevel.WARNING
