"""Copy required local DLLs, preserve replaced files, and remember selected ROMs."""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]

def replace_preserving(source, target):
    if target.exists():
        if hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(source.read_bytes()).digest():
            return
        backup = ROOT / 'build-setup-backups' / str(time.time_ns()) / target.relative_to(ROOT)
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target, backup)
    descriptor, stage = tempfile.mkstemp(prefix='.dll-stage-', dir=target.parent)
    os.close(descriptor)
    try:
        shutil.copyfile(source, stage)
        os.replace(stage, target)
    finally:
        Path(stage).unlink(missing_ok=True)

def main():
    mode = '-u' if sys.platform.startswith(('cygwin', 'msys')) else '-w'
    prefix = Path(subprocess.check_output(['cygpath', mode, '/mingw64/bin'], text=True).strip())
    tasks = [(prefix / 'libwinpthread-1.dll', ROOT / 'build-launcher/bin/libwinpthread-1.dll')]
    for game in ('ages', 'seasons'):
        for name in ('libwinpthread-1.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'SDL2.dll'):
            tasks.append((prefix / name, ROOT / 'build/bin' / game / name))
    for source, target in tasks:
        if not source.is_file() or not target.parent.is_dir():
            raise ValueError('Required DLL or output folder missing: ' + str(source))
    for source, target in tasks:
        replace_preserving(source, target)
    settings = ROOT / 'build-launcher/bin/oracle-launcher.ini'
    if settings.exists():
        backup = ROOT / 'build-setup-backups' / str(time.time_ns()) / 'oracle-launcher.ini'
        backup.parent.mkdir(parents=True)
        shutil.copy2(settings, backup)
    # UTF-16 BOM lets the launcher's Windows INI APIs preserve the file paths.
    values = {}
    for game in ('ages', 'seasons'):
        value = os.environ['ORACLE_SETUP_' + game.upper()]
        if sys.platform.startswith(('cygwin', 'msys')):
            value = subprocess.check_output(['cygpath', '-w', value], text=True).strip()
        else:
            value = str(Path(value).resolve())
        values[game] = value
    content = '[ROMs]\r\n' + ''.join(game + '=' + values[game] + '\r\n' for game in ('ages', 'seasons'))
    descriptor, stage = tempfile.mkstemp(prefix='.ini-stage-', dir=settings.parent)
    os.close(descriptor)
    try:
        Path(stage).write_bytes(content.encode('utf-16'))
        os.replace(stage, settings)
    finally:
        Path(stage).unlink(missing_ok=True)

if __name__ == '__main__':
    main()
