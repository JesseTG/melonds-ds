import os

from libretro import Session, HistoryFileSystemInterface, StandardFileSystemInterface, VfsOperation, VfsOperationType, \
    VfsFileAccess

import prelude

assert not os.access(prelude.wfcsettings_path, os.F_OK), f"{prelude.wfcsettings_path} should not exist yet"

vfs = HistoryFileSystemInterface(StandardFileSystemInterface())
session: Session
with prelude.builder().with_vfs(vfs).build() as session:
    for i in range(300):
        session.run()

    firmwareFlushed = False
    op: VfsOperation
    for op in filter(lambda f: f.operation == VfsOperationType.OPEN, vfs.history):
        path = op.args[0]
        assert isinstance(path, bytes), f"Expected path to be bytes, found {type(path).__name__} instead"
        mode = op.args[1]
        assert isinstance(mode, VfsFileAccess), f"Expected mode to be VfsFileAccess, found {type(mode).__name__} instead"
        if path.endswith(b'wfcsettings.bin') and VfsFileAccess.WRITE in mode:
            assert op.result is not None
            firmwareFlushed = True

    assert firmwareFlushed, f"Expected to find a write operation to {prelude.wfcsettings_path}"
