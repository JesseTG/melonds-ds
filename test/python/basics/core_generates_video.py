from array import array
from sys import argv
from libretro import default_session

with default_session(argv[1]) as session:
    for i in range(10):
        session.core.run()

    assert session.video.frame is not None

    frame1 = array(session.video.frame)

    for i in range(60):
        session.core.run()

    assert session.video.frame != frame1
