from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    assert session.options.update_display_callback
    assert session.options.update_display_callback.callback
