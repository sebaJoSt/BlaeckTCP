# Contribution to BlaeckTCP

First, thank you for taking the time to contribute to this project.

You can submit changes via GitHub Pull Requests.

Please:

1. Make changes to `src/BlaeckCore.h` and `src/BlaeckCore.cpp` in
   [BlaeckSerial](https://github.com/sebaJoSt/BlaeckSerial), not here. They are copied
   from there, and CI fails if they differ
2. Document every public name you add, and give it an example — the rules are in
   [extras/API-STYLE.md](extras/API-STYLE.md), and CI checks them
3. Call the object `Blaeck` in examples and doc comments, never `BlaeckTCP` — a
   variable sharing its type's name switches off autocomplete in VS Code
4. Say so in the pull request if a change touches the public API or the frames on
   the wire, since BlaeckSerial usually needs the same change
