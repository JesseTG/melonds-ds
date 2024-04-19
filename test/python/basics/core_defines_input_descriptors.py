from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    descriptors = session.input_descriptors

    assert descriptors is not None
    assert len(descriptors) > 0
    assert all(d.description for d in descriptors)
