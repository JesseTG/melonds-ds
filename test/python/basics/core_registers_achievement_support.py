from libretro import Session
import prelude

session: Session
with prelude.session() as session:
    assert session.environment.support_achievements
