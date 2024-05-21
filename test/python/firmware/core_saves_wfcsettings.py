import os

from libretro import Session

import prelude

assert not os.access(prelude.wfcsettings_path, os.F_OK), f"{prelude.wfcsettings_path} should not exist yet"

session: Session
with prelude.session() as session:
    for i in range(300):
        session.run()

    assert os.access(prelude.wfcsettings_path, os.F_OK), f"{prelude.wfcsettings_path} should exist by now"
