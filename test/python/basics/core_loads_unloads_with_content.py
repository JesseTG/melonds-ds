from sys import argv
from libretro import default_session

with default_session(argv[1], argv[2]) as session:
    for i in range(10):
        session.core.run()
