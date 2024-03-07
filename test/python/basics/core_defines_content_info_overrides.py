from libretro import default_session

import prelude

with default_session(prelude.core_path) as session:
    overrides = session.content_info_overrides

    assert overrides is not None
    assert len(overrides) > 0
    assert all(o.extensions for o in overrides)
