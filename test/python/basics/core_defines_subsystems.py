from pprint import pprint

import prelude

with prelude.session() as session:
    subsystems = session.environment.subsystems

    assert subsystems is not None
    assert len(subsystems) > 0
    assert all(s.desc for s in subsystems)

# Testing this _after_ unloading the core to ensure the data is still valid
pprint(subsystems)
