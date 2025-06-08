from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.noload_session() as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    melondsds_analyze_vfl_issues = session.get_proc_address(b"melondsds_analyze_vfl_issues", CFUNCTYPE(c_size_t))
    assert melondsds_analyze_vfl_issues is not None

    issues: int = melondsds_analyze_vfl_issues()

    assert issues == 0, f"Expected melondsds_analyze_vfl_issues to return 0, got {issues}"

