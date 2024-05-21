from libretro import Session
from libretro import StandardFileSystemInterface, HistoryFileSystemInterface, VfsOperation, VfsOperationType

import prelude

vfs = HistoryFileSystemInterface(StandardFileSystemInterface())
session: Session
with prelude.builder().with_vfs(vfs).build() as session:
    for i in range(300):
        session.run()

    op: VfsOperation
    for op in filter(lambda f: f.operation == VfsOperationType.OPEN, vfs.history):
        path = op.args[0]
        assert isinstance(path, bytes), f"Expected path to be bytes, found {type(path).__name__} instead"
        assert not path.endswith(b'bios7.bin'), f"{path} should not have been opened"
        assert not path.endswith(b'bios9.bin'), f"{path} should not have been opened"
