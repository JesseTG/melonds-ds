from libretro import retro_system_av_info, Session

import prelude

session: Session
with prelude.session() as session:
    av_info: retro_system_av_info = session.core.get_system_av_info()

    assert av_info is not None
    assert av_info.timing.sample_rate != 0
    assert av_info.geometry.base_width != 0
