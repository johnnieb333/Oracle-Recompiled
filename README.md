# Oracle Portable — Interpreter Preview

An enhanced, interpreter-based player for Oracle of Ages and Oracle of Seasons.
This version is not a static recompilation. The static recompilation remains
on the main branch.

## Playing the portable package

Extract the entire portable ZIP into a writable folder.
Open oracle-launcher.exe, select your own supported USA ROMs, and click Play.
Players do not need MSYS2, a compiler, or a game-build step.
Press F1 in a game to show the enhancement menu.

Each game keeps its own saves beside its executable.
Keep the folder layout and bundled DLLs together.
No ROMs, generated game code, or personal saves are supplied.

## Building from source

Developers can build the runtime and launcher using MSYS2 MINGW64 with
GCC, CMake, Ninja, and SDL2 installed:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel 2

No ROM is needed to compile this interpreter version.
The launcher is produced at build/bin/oracle-launcher.exe.

## Tested

Both games launched, reached gameplay, and retained saves after reopening.
The interpreter experiment also passed user-reported autosave and editor checks.
Both games launched from a freshly extracted portable ZIP.
The user estimated the interpreter build took under three minutes.
A full playthrough and testing on a separate clean Windows PC remain outstanding.

## Credits and licenses

Based on sp00nznet/oracle-recompiled and the gb-recompiled runtime by
arcanite24, contributors, and sp00nznet.
Uses Dear ImGui and SDL2.
The linked-secret adaptation derives from ZoraSharp by Amy Nagle.

Retain LICENSE, launcher/LICENSE, third-party notices, and LGPL/GPL texts.
The portable test package includes application source and rebuild instructions.
Public binary distribution review remains outstanding.

The older generation and setup scripts retained on this branch are from
the static recompilation workflow and are not needed for this interpreter build.
