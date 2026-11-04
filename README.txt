Oracle Portable v0.2.0-beta — source

Interpreter-based player for Oracle of Ages and Oracle of Seasons.
Supply your own supported USA ROMs at runtime. No ROM, generated game code,
or game assets are included. Building does not require a ROM.

Build in MSYS2 MINGW64 with GCC, CMake, Ninja and SDL2:
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel 2

Outputs: build/bin/oracle-launcher.exe and build/bin/{ages,seasons}/oracle-{ages,seasons}.exe.
The launcher supports this build layout and the packaged portable layout.
For double-click launch, keep the packaged dependency DLLs beside each executable.

F1 opens the enhancement menu. Controller contains saved Enable extra item
buttons and Show button indicators options. Both default off. When enabled,
X/Y/L/R use owned-item assignments instead of their ordinary game mappings.
Game speed (%) accepts 10-500; press Enter to apply. 100 is normal speed.

Keep LICENSE, launcher/LICENSE and all notices. The linked-secret adaptation
is covered by the included LGPL terms; complete corresponding application
source is supplied for rebuilding and relinking. See notices/6-THIRD_PARTY.txt.
Credits include sp00nznet/oracle-recompiled; arcanite24 and gb-recompiled
contributors; Dear ImGui; SDL2; and ZoraSharp by Amy Nagle.
The Zelda games are Nintendo / Capcom / Flagship titles.

This beta has targeted tests, not complete playthrough verification.
See VALIDATION.txt and the package TEST-RESULTS.txt for tested scope.
