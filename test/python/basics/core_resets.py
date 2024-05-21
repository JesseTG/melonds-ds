import prelude

with prelude.session() as session:
    for i in range(60):
        session.run()

    session.reset()

    for i in range(60):
        session.run()
