from libretro import DictOptionDriver

import prelude

with prelude.builder().with_options(DictOptionDriver(version=1)).build() as session:
    assert session.options is not None
    assert session.options.version == 1

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
