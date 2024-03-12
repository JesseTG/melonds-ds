from pprint import pprint
from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    info = session.controller_info

    pprint(info)

    assert info is not None
    assert len(info) > 0
    assert all(len(i) for i in info)
