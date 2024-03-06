from sys import argv

from libretro import default_session

with default_session(argv[1]) as session:
    av_info = session.core.get_system_av_info()

    assert av_info is not None
    assert av_info.timing.sample_rate != 0
    assert av_info.geometry.base_width != 0
