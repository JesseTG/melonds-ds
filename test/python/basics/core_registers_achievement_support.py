from libretro import default_session

import prelude

with default_session(prelude.core_path, prelude.content_path) as env:
    assert env.support_achievements is True
