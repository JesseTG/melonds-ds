from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    assert session.support_no_game is True
