from libretro import Session

import prelude

session: Session
with prelude.noload_session() as session:
    pass
