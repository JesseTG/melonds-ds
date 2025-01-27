add_python_test(
    NAME "Host microphone is open at start when configured"
    TEST_MODULE microphone.open_at_start
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Host microphone is open and active at start if button is held in Hold mode"
    TEST_MODULE microphone.active_at_start_when_button_held
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Host microphone is open and inactive at start if button isn't held in Hold mode"
    TEST_MODULE microphone.inactive_at_start_when_button_not_held
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Host microphone is closed at start if mic isn't toggled in Toggle mode"
    TEST_MODULE microphone.inactive_at_start_when_button_not_toggled
    CONTENT "${NDS_ROM}"
)

add_python_test(
    NAME "Host microphone is open and active at start when in Always mode"
    TEST_MODULE microphone.active_at_start_if_always
    CONTENT "${NDS_ROM}"
)