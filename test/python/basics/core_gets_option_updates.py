from libretro import Session
import prelude

session: Session
with prelude.session() as session:
    assert not session.options.variable_updated

    assert session.options.variables['melonds_audio_interpolation'] == b'disabled'
    assert not session.options.variable_updated
    # Assert that checking an option in the frontend doesn't mark variables as updated

    session.options.variables['melonds_audio_interpolation'] = b'linear'
    assert session.options.variable_updated
    # Assert that setting an option in the frontend marks variables as updated

    session.core.run()

    assert not session.options.variable_updated
    # Assert that running the core clears the updated flag
