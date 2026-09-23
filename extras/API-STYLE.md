# Writing comments

BlaeckTCP follows BlaeckSerial's guide:
[extras/API-STYLE.md in BlaeckSerial](https://github.com/sebaJoSt/BlaeckSerial/blob/master/extras/API-STYLE.md).

There is one guide because most of the documentation is shared: `BlaeckCore.h` is a copy
of BlaeckSerial's and is only ever changed there.

Two things apply to BlaeckTCP only:

- **The begin() chain.** `BlaeckTCPBeginRef` repeats the six table-size calls of the
  core's `BlaeckBeginRef` so that `withClients()` can come anywhere in the chain. Their doc
  text has to match the core's word for word; only the `@code` example differs, because
  BlaeckTCP's `begin()` takes a port. The checker enforces it with `--same-prose`.
- **Examples use `SERVER_PORT`** for the port, and `Blaeck.Terminal` where BlaeckSerial's
  would print to `Serial`.

```
python extras/scripts/checkdocs.py src/BlaeckTCP.h src/BlaeckCore.h --skip-class BlaeckBeginRef --same-prose BlaeckTCPBeginRef=BlaeckBeginRef
```
