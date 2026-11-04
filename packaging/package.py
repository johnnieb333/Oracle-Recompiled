#!/usr/bin/env python3
"""Create a clean Oracle Portable v0.2.0-beta release candidate."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

SYSTEM_DLLS = {
    'advapi32.dll', 'bcrypt.dll', 'comdlg32.dll', 'gdi32.dll', 'imm32.dll',
    'kernel32.dll', 'msvcrt.dll', 'ole32.dll', 'oleaut32.dll', 'setupapi.dll',
    'shell32.dll', 'user32.dll', 'version.dll', 'winmm.dll'
}
GAME_DLLS = ('libwinpthread-1.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'SDL2.dll')
NOTICE_FILES = ('gcc/COPYING.RUNTIME', 'gcc/COPYING3', 'libgcc/COPYING.RUNTIME',
                'libgcc/COPYING3', 'libwinpthread/COPYING', 'winpthreads/COPYING', 'SDL2/LICENSE.txt')

def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def check_pe(path, objdump, allowed):
    text = subprocess.check_output([str(objdump), '-p', str(path)], text=True, errors='replace')
    if 'pei-x86-64' not in text:
        raise ValueError('Expected an x86-64 Windows binary: ' + str(path))
    imports = set(x.lower() for x in re.findall(r'DLL Name:\s*(\S+)', text))
    if not imports:
        raise ValueError('No DLL import information found: ' + str(path))
    unexpected = imports - SYSTEM_DLLS - {x.lower() for x in allowed}
    if unexpected:
        raise ValueError('Unpackaged DLL dependencies in ' + path.name + ': ' + ', '.join(sorted(unexpected)))
    return sorted(imports)

def package(project, msys, output):
    project, msys, output = map(lambda p: Path(p).resolve(), (project, msys, output))
    build = project / 'build'
    if (project / 'source/CMakeLists.txt').is_file():
        project = project / 'source'
    archive = output.parent / (output.name + '.zip')
    if output.exists() or archive.exists():
        raise ValueError('Output folder or ZIP already exists. Nothing replaced: ' + str(output))
    if output == project or project in output.parents:
        raise ValueError('Choose an output folder outside the source project.')
    kit = Path(__file__).resolve().parent
    expected = json.loads((kit / 'expected-source.json').read_text())
    for name, digest in expected.items():
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Invalid source inventory path.')
        path = project / relative
        if path.is_symlink() or not path.is_file() or project not in path.resolve().parents:
            raise ValueError('Expected regular source file is missing: ' + name)
        if hashlib.sha256(path.read_bytes().replace(b'\r\n', b'\n')).hexdigest() != digest:
            raise ValueError('Source differs from the inspected interpreter build: ' + name)
    binaries = {
        'oracle-launcher.exe': build / 'bin/oracle-launcher.exe',
        'bin/ages/oracle-ages.exe': build / 'bin/ages/oracle-ages.exe',
        'bin/seasons/oracle-seasons.exe': build / 'bin/seasons/oracle-seasons.exe'
    }
    prefix = msys / 'mingw64'
    objdump = prefix / 'bin/objdump.exe'
    if not objdump.is_file():
        raise ValueError('MinGW objdump was not found: ' + str(objdump))
    for source in binaries.values():
        if not source.is_file(): raise ValueError('Missing built executable: ' + str(source))
    for name in GAME_DLLS:
        if not (prefix / 'bin' / name).is_file(): raise ValueError('Missing DLL: ' + name)
    for name in NOTICE_FILES:
        if not (prefix / 'share/licenses' / name).is_file(): raise ValueError('Missing license notice: ' + name)
    imports = {}
    for name, path in binaries.items():
        imports[name] = check_pe(path, objdump, GAME_DLLS if name.startswith('bin/') else ('libwinpthread-1.dll',))
    for name in GAME_DLLS:
        imports[name] = check_pe(prefix / 'bin' / name, objdump, GAME_DLLS)
    # Validate everything before writing the package. No working source or build is changed.
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.oracle-package-', dir=output.parent) as temporary:
        stage = Path(temporary) / output.name
        stage.mkdir()
        for destination, source in binaries.items():
            target = stage / destination; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        shutil.copyfile(prefix / 'bin/libwinpthread-1.dll', stage / 'libwinpthread-1.dll')
        for game in ('ages', 'seasons'):
            for name in GAME_DLLS:
                shutil.copyfile(prefix / 'bin' / name, stage / 'bin' / game / name)
        # Exact source allowlist: never recurse over a user's build/save directories.
        for name in expected:
            target = stage / 'source' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(project / name, target)
        (stage / 'source/packaging').mkdir()
        for name in ('package.py', 'expected-source.json'):
            shutil.copyfile(kit / name, stage / 'source/packaging' / name)
        for name in NOTICE_FILES:
            target = stage / 'notices/dependencies' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(prefix / 'share/licenses' / name, target)
        for path in (project / 'notices').iterdir():
            if path.is_file(): shutil.copyfile(path, stage / 'notices' / path.name)
        shutil.copyfile(project / 'LICENSE', stage / 'LICENSE')
        shutil.copyfile(project / 'runtime/third_party/imgui/LICENSE.txt', stage / 'notices/ImGui-LICENSE.txt')
        shutil.copyfile(project / 'launcher/LICENSE', stage / 'notices/Launcher-LICENSE.txt')
        (stage / 'START-HERE.txt').write_text(START_HERE, encoding='utf-8')
        (stage / 'REBUILD.txt').write_text(REBUILD, encoding='utf-8')
        (stage / 'TEST-RESULTS.txt').write_text(TEST_RESULTS, encoding='utf-8')
        shutil.copyfile(project / 'launcher/Oracle-Portable.ico', stage / 'Oracle-Portable.ico')
        # Versions and hashes are recorded without personal absolute source/ROM paths.
        versions = {}
        pacman = msys / 'usr/bin/pacman.exe'
        if pacman.is_file():
            for name in ('mingw-w64-x86_64-gcc-libs', 'mingw-w64-x86_64-SDL2', 'mingw-w64-x86_64-libwinpthread'):
                result = subprocess.run([str(pacman), '-Q', name], capture_output=True, text=True)
                versions[name] = result.stdout.strip() if result.returncode == 0 else 'version lookup unavailable'
        (stage / 'BUILD-INFO.json').write_text(json.dumps({'version': '0.2.0-beta', 'status': 'release-candidate', 'execution': 'interpreter-only', 'imports': imports,
            'dependency_packages': versions, 'binary_sha256': {name: sha256(stage / name) for name in binaries}}, indent=2)+'\n')
        hashes = {p.relative_to(stage).as_posix(): sha256(p) for p in stage.rglob('*') if p.is_file()}
        forbidden = {'.gb', '.gbc', '.rom', '.sav', '.rtc', '.bin', '.ini', '.cfg', '.bak'}
        if any(Path(name).suffix.lower() in forbidden for name in hashes):
            raise ValueError('Unexpected personal or game data in package.')
        (stage / 'SHA256SUMS.txt').write_text(''.join(value+'  '+name+'\n' for name,value in sorted(hashes.items())))
        staged_zip = Path(temporary) / 'package.zip'
        with zipfile.ZipFile(staged_zip, 'w', zipfile.ZIP_DEFLATED) as zipped:
            for path in sorted(stage.rglob('*')):
                if path.is_file(): zipped.write(path, output.name+'/'+path.relative_to(stage).as_posix())
        with zipfile.ZipFile(staged_zip) as zipped:
            if zipped.testzip() is not None: raise ValueError('ZIP verification failed.')
        stage.rename(output)
        try:
            # Exclusive destination creation avoids replacing an existing release ZIP.
            with archive.open('xb') as target, staged_zip.open('rb') as source:
                shutil.copyfileobj(source, target)
        except Exception:
            raise ValueError('Package folder created, but ZIP could not be saved. Existing files were not replaced.')
    print('Release candidate folder: ' + str(output))
    print('Portable ZIP: ' + str(archive))
    print('No ROMs, saves, states, settings, or remembered paths were included.')
    print('Local release candidate only. Nothing was uploaded or published.')

START_HERE = "Oracle Portable v0.2.0-beta — release candidate\n\n1. Extract the entire ZIP into a writable folder.\n2. Double-click oracle-launcher.exe.\n3. Select your own supported USA ROM for each game and click Play.\n\nF1 > Controller:\n- Enable extra item buttons: assign owned items to gamepad X/Y/L/R.\n- Show button indicators: optionally display the four input boxes.\nBoth options default OFF and save separately for each game. Assignments save\nas well. Extra controls OFF restores those buttons' ordinary game mappings.\nThe feature does not change the original A/B item assignments.\n\nF1 > Config > Game speed (%): type 10-500 and press Enter.\n25 is quarter speed, 50 half speed, 100 normal, 200 double, 500 five times.\nReset to Normal returns to 100%.\n\nNo ROMs, personal saves, save states, settings or remembered ROM paths are\nincluded. Each game creates its own data in bin/ages or bin/seasons.\nFor upgrades, BACK UP your existing portable folder first. Preserve your own\nsaves and states; do not overwrite or delete them. Test imported states on\ncopies, since universal compatibility is not promised.\n\nThe complete folder layout and DLLs are required. Do not run inside the ZIP.\nFor a custom shortcut icon, right-click the shortcut, choose Properties >\nChange Icon, then select Oracle-Portable.ico from this folder.\nNo compiler or MSYS2 installation is needed to play the prebuilt package.\nThis player uses an interpreter; it is not a static recompilation.\nSupply USA ROMs with the original supported revisions. ROM paths must be\nrepresentable in your Windows system code page for this build.\nClose an instance of a game before launching that same game again.\n\nThis is beta software with targeted testing; full playthroughs are not verified.\nSee TEST-RESULTS.txt. Source, rebuild instructions and notices are included.\nCredits: Nintendo/Capcom/Flagship (games); sp00nznet/oracle-recompiled;\narcanite24 and gb-recompiled contributors; Dear ImGui; SDL2; ZoraSharp / Amy Nagle.\n"

REBUILD = '''Rebuilding / replacing the linked-secret adaptation

Complete inspected application and runtime source is in source/, including the
LGPL adaptation oracle_linked_core.h, its modifications, and menu integration.
You can modify it and rebuild the full application without any ROM or generated
game code as build inputs. See source/notices/6-THIRD_PARTY.txt and the LGPL/GPL
texts. No signatures or activation keys are required for the rebuilt programs.

Using an installed MSYS2 MINGW64 toolchain with GCC, CMake, Ninja and SDL2:
  cd source
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel 2

Built programs are source/build/bin/oracle-launcher.exe and
source/build/bin/{ages,seasons}/oracle-{ages,seasons}.exe.
Run them from MINGW64 or copy the packaged DLLs beside the new executables.
To replace the portable games, close them and preserve their saves, then replace
only the executables in bin/ages and bin/seasons with your rebuilt executables.
The launcher can be replaced with your rebuilt launcher executable at the root.
Packaging source and its source inventory are included in source/packaging/.

Dependency notices and recorded installed package versions are included.
Upstream dependency source and MSYS2 packaging references:
  https://github.com/msys2/MINGW-packages
  https://gcc.gnu.org/
  https://github.com/libsdl-org/SDL
  https://github.com/mingw-w64/mingw-w64

This is a local release candidate. Functional tests do not themselves complete
public distribution review or dependency source obligations.
'''
TEST_RESULTS = 'Oracle Portable v0.2.0-beta — validation scope\n\nReported Windows gameplay tests: both games launch through the launcher;\nowned-item dropdowns, assignments and feature/indicator settings persist.\nSeasons: 15 selectable extra items exercised, upgrades and zero-ammo checks.\nAges: 15 selectable extra items exercised, sword/shield holds through scrolls,\nsimultaneous items, A/B aliases, carry/throw, ammo and zero-ammo checks,\nSwitch/Long Hook swapping, Cane block creation/removal and all Harp tunes.\nEditors reopen after all four Seasons changes and all three Ages travel methods.\nExact speed entry and keyboard Enter capture were tested in gameplay.\n\nLocal automated checks: actual interpreter input/initialization fixtures,\nSeasons regression checks, owned/sorted menu checks, option persistence,\nwarp-guard regression checks and full changed C++ translation-unit syntax.\nAges fixture slot-priority selection is simulated; it is not a full game run.\n\nNot fully verified: complete playthroughs, every dungeon/boss or item combination,\ncompanion summoning, Seasons magnetic attraction/repulsion puzzle behavior,\nand Ages heavy-statue lifting. These features are not claimed fully tested.\nNo achievements system or widescreen feature is added by this release.\nA fresh-folder release-candidate smoke test is still required.\n'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True)
    parser.add_argument('--msys', default='C:/msys64')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    try:
        package(args.project, args.msys, args.output)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        parser.exit(1, 'Packaging stopped: ' + str(error) + '\n')

if __name__ == '__main__':
    main()
