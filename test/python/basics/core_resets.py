from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    for i in range(60):
        session.core.run()

    session.core.reset()

    for i in range(60):
        session.core.run()
