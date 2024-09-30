from pprint import pprint

import prelude

with prelude.session() as session:
    info = session.environment.controller_info

    assert info is not None
    assert len(info) > 0
    assert all(i.desc for i in info)


# Testing this _after_ unloading the core to ensure the data is still valid
pprint(info)
pprint(info[0])
