import prelude
from libretro import StandardFileSystemInterface, HistoryFileSystemInterface

vfs = HistoryFileSystemInterface(StandardFileSystemInterface())

with prelude.builder().with_vfs(vfs).build() as session:
    assert session.vfs is vfs, f"Expected the session to use the vfs interface we provided, found {session.vfs} instead"
    assert vfs.history is not None, "Expected the vfs interface to have a history, found None instead"
    assert len(vfs.history) > 0, "Expected the vfs interface to have a non-empty history"
