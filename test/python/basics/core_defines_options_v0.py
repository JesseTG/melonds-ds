from collections.abc import Mapping

from libretro import Session
from libretro.api.options import retro_core_option_v2_definition, StandardOptionState

import prelude

definitions: Mapping[bytes, retro_core_option_v2_definition]
session: Session
with prelude.session(options=StandardOptionState(version=0)) as session:
    assert session.options is not None
    assert session.options.version == 0

    assert session.options.categories is None

    assert session.options.definitions is not None
    assert len(session.options.definitions) > 0
    assert all(d.key for d in session.options.definitions.values())
    assert all(not d.category_key for d in session.options.definitions.values())
    definitions = session.options.definitions

# Ensure that the frontend's copy of this data is still alive
del session
assert definitions is not None
assert len(definitions) > 0
assert all(v.key for v in definitions.values())