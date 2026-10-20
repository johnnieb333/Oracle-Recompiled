# Oracle Launcher — Windows preview

One native Windows window with Select ROM and Play buttons for Ages and Seasons.
ROMs are validated by exact size and SHA-1 before selection and again before launch.
Only the file locations are remembered in oracle-launcher.ini beside the launcher.
No ROM is copied or embedded. Games keep their existing save locations.

Build from the project root using MSYS2 MINGW64:

```bash
cmake -S launcher -B build-launcher -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-launcher --parallel 2
./build-launcher/bin/oracle-launcher.exe
```

Keep the executable in build-launcher/bin for source-build testing. It finds games
in the same project's build/bin/ages and build/bin/seasons folders. A future
portable layout may instead put the launcher beside bin/ages and bin/seasons.
The Windows preview still needs game DLLs available through MINGW64's PATH;
standalone double-click packaging is a separate step.

Select the ROM that matches each game. Cancel leaves the previous selection intact;
wrong ROMs are rejected. Close a running game normally before launching another
instance of that same game, to avoid two instances writing the same saves.

The game loader currently uses a narrow-character entry point. For this preview,
use ROM locations representable in the Windows system code page (the existing
C:/ZeldaOracleRecomp paths work). Full Unicode end-to-end launch testing is pending.

Local checks: Windows argument quoting with spaces, quotes, and trailing backslashes;
installer refusal on an existing launcher folder; installer preservation of existing files.
Windows build and ROM selection passed. Both games launched and showed their existing test saves. Both ROM locations were remembered after restarting the launcher.

This is a launcher source preview, not a ready-to-distribute binary package.
See LICENSE for the launcher code license.
