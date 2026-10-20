#!/usr/bin/env bash
set -euo pipefail
export PATH=/mingw64/bin:/usr/bin:$PATH
cd -- "$(cygpath -u "$ORACLE_SETUP_ROOT")"
for program in cmake ninja g++ python; do
    if ! command -v "$program" >/dev/null 2>&1; then
        printf 'Missing %s. See EASY-SETUP.txt for MSYS2 prerequisites.\n' "$program" >&2
        exit 1
    fi
done
if [[ ! -f /mingw64/bin/SDL2.dll ]]; then
    printf 'SDL2 is missing. See EASY-SETUP.txt.\n' >&2
    exit 1
fi
printf 'Step 1/5: Build compiler\n'
cmake -S vendor -B build-tools -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-tools --parallel 2
printf 'Step 2/5: Prepare games from your ROMs\n'
python tools/setup-prepare.py
printf 'Step 3/5: Build games\n'
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
printf 'Step 4/5: Build launcher\n'
cmake -S launcher -B build-launcher -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-launcher --parallel 2
printf 'Step 5/5: Set up DLLs and remembered ROM locations\n'
python tools/setup-finish.py
printf 'Setup complete.\n'
