from sys import argv
from libretro import default_session

with default_session(argv[1]) as session:
    session.core.run()
