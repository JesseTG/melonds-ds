import logging
import typing
from ctypes import *

import prelude
from libretro import LoggerMessageInterface

driver = LoggerMessageInterface(0, logging.getLogger('libretro'))
with prelude.builder().with_message(driver).build() as session:
    message = typing.cast(LoggerMessageInterface, session.message)
    assert message is not None
    assert message.version == 0, f"Expected version 0, got {message.version}"

    send_message = session.get_proc_address(b"libretropy_send_message", CFUNCTYPE(c_bool, c_char_p))
    assert send_message is not None

    assert send_message(b"Hello, world!")

    assert message.messages is not None
    assert len(message.messages) > 0
    assert any(m.msg == b"Hello, world!" for m in message.messages)
