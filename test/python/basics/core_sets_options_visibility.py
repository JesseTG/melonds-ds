from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    assert session.options.update_display_callback
    assert session.options.update_display_callback.callback

    assert 'melonds_console_mode' in session.options.variables
    assert 'melonds_dsi_nand_path' in session.options.variables

    assert session.options.variables['melonds_console_mode'] == b'ds'
    assert session.options.visibility['melonds_ds_battery_ok_threshold']

    session.options.variables['melonds_console_mode'] = b'dsi'

    assert session.options.variables['melonds_console_mode'] == b'dsi'
    assert not session.options.visibility['melonds_ds_battery_ok_threshold']
