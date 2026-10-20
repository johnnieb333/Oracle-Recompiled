# Launcher for the separate interpreter experiment

Build this through the root CMake project. The executable is build/bin/oracle-launcher.exe.
Select the supported USA ROM for each game. This launcher remembers file locations,
checks ROM size and SHA-1, and starts the matching interpreter-test executable.
Run from MINGW64 until the DLLs are packaged for double-click testing.
The experiment's root README.txt describes its test scope and limitations.

The game entry point uses narrow paths: use ROM locations supported by the Windows
system code page for now. This source preview needs Windows gameplay testing.
See LICENSE for this launcher's MIT license.
