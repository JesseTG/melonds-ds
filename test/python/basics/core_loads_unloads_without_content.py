from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    for i in range(10):
        session.core.run()
