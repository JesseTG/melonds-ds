from sys import argv
from libretro import default_session


with default_session(argv[1]) as env:
    assert env.log is not None
    assert env.log.entries is not None
    assert len(env.log.entries) > 0
