from pprint import pprint
import libretro

import prelude

session: libretro.Session
with prelude.session() as session:
    overrides = session.content_info_overrides

    pprint(overrides)
    assert overrides is not None
    assert len(overrides) > 0
    assert all(o.extensions for o in overrides)
