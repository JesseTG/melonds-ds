from collections.abc import Mapping

from libretro import Session
from libretro.api.options import retro_core_option_v2_definition

import prelude

definitions: Mapping[bytes, retro_core_option_v2_definition]
session: Session
with prelude.session() as session:
    assert session.options is not None
    assert session.options.version == 2

    assert session.options.definitions is not None
    assert len(session.options.definitions) > 0
    assert all(v.key for v in session.options.definitions.values())
    definitions = session.options.definitions

    assert session.options.categories is not None
    assert len(session.options.categories) > 0
    assert all(c.key for c in session.options.categories.values())

# Ensure that the frontend's copy of this data is still alive
del session
assert definitions is not None
assert len(definitions) > 0
assert all(v.key for v in definitions.values())