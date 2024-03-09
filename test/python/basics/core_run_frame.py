from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    session.core.run()