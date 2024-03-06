from sys import argv
from libretro import default_session

import prelude

with default_session(argv[1], argv[2]) as env:
    assert env.support_no_game is True
