# Oracle Recompiled — source preview

Enhanced fork of sp00nznet/oracle-recompiled and its gb-recompiled runtime.
This is a source preparation preview for the planned v0.1.0-beta, not a finished public release.
No ROM, generated game code, executable, save, or game asset is included.
Generate and build locally with your own supported ROMs. Generated C still represents game instructions and is excluded from version control.

## Windows build (MSYS2 MINGW64)

Requirements: Python 3, CMake 3.20+, Ninja, MinGW-w64 GCC with C++20, SDL2 development package.
The compiler and game builds use separate build directories.

```bash
cmake -S vendor -B build-tools -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-tools --parallel 2
python tools/prepare_game.py ages --rom "C:/path/to/your/ages.gbc"
python tools/prepare_game.py seasons --rom "C:/path/to/your/seasons.gbc"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/bin/ages/oracle-ages.exe --rom "C:/path/to/your/ages.gbc"
./build/bin/seasons/oracle-seasons.exe --rom "C:/path/to/your/seasons.gbc"
```

Both games must be generated before configuring the game build. Generation refuses to overwrite an existing game directory. The generation script validates the ROM before invoking the compiler, uses a private temporary copy, and keeps only gameplay C, its header, and generation metadata. The original ROM is never modified.

Supported ROMs are exactly 1,048,576 bytes:

| Game | SHA-1 |
| --- | --- |
| Ages USA | 880374fb978b18af4aa529e2e32f7ffb4d7dd2f4 |
| Seasons USA | ba1268290fb2b1b70505d2d7b5825fc8a4816a4b |

Run from MINGW64 while testing so installed SDL2/compiler DLLs can be found. Each game keeps new saves and settings beside its executable; existing builds and saves are not imported. F1 shows the menu, hidden at startup.

## Validation and limits

The earlier isolated external-ROM builds launched and retained battery saves after closing and reopening for both games on Windows. On October 20, 2026, both games were regenerated from user-supplied ROMs, built on Windows, and passed movement, audio, and battery-save reload checks. The user also confirmed the enhancements work in Seasons. No complete playthrough has been verified.

The runtime includes save-state previews, rotating autosaves, LCD filters, fullscreen, controller remapping, fast-forward, game-specific editors, and linked-password tools. These features were exercised in earlier working builds, with enhancements also checked in regenerated Seasons. Regenerated Ages also passed LCD filter, manual save-state loading, autosave loading, and editor rupee-application checks. Ages ring-password filling remains unfinished; five-character NPC secrets, achievements, and a password library are not implemented.

## Credits and notices

- Games: Nintendo / Capcom Flagship. This project is unaffiliated.
- Original Oracle project: https://github.com/sp00nznet/oracle-recompiled
- Recompiler/runtime: arcanite24 and gb-recompiled contributors; sp00nznet fork.
- Compiler baseline: 9b6cb759adaf6b0e5f72a3592ab724188113d8a1, with the supplied local c_emitter.cpp fixes.
- Dear ImGui: MIT, see runtime/third_party/imgui/LICENSE.txt.
- SDL2: external build dependency; retain its notices if packaging its DLLs later.
- Linked-secret codec: ZoraSharp by Amy Nagle, adapted under LGPL-3.0-or-later; see notices/6-THIRD_PARTY.txt and LGPL/GPL texts.

Retain LICENSE, vendor/LICENSE, all notices, and third-party license files. The MIT notices exclude generated game code. Binary distribution and LGPL relinking materials have not been finalized in this preview. This archive distributes source only.

## Windows launcher

The launcher selects and remembers both ROM locations.
Both games launched with their existing test saves.
Double-click launching passed after copying the required DLLs.

From the project root, build the launcher:

    cmake -S launcher -B build-launcher -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-launcher --parallel 2

Open build-launcher/bin/oracle-launcher.exe.

For double-click launching, copy libwinpthread-1.dll from
C:/msys64/mingw64/bin into build-launcher/bin.
Copy libwinpthread-1.dll, libgcc_s_seh-1.dll, libstdc++-6.dll,
and SDL2.dll from that directory into both build/bin/ages
and build/bin/seasons.

Keep the launcher in build-launcher/bin.
See launcher/README.md for details.
No prebuilt game package is supplied.
