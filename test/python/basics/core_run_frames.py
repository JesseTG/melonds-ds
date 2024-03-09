from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    for i in range(300):
        session.core.run()
