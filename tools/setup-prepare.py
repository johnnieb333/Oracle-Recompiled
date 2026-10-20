"""Verify/reuse locally generated code, or preserve it before regeneration."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]

def main():
    spec = importlib.util.spec_from_file_location('oracle_prepare', ROOT / 'tools/prepare_game.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    compiler = ROOT / 'build-tools/bin/gbrecomp.exe'
    compiler_hash = hashlib.sha256(compiler.read_bytes()).hexdigest()
    # Check both inputs before moving or generating anything.
    inputs = {}
    for game, expected in module.HASHES.items():
        value = os.environ['ORACLE_SETUP_' + game.upper()]
        if sys.platform.startswith(('cygwin', 'msys')):
            value = subprocess.check_output(['cygpath', '-u', value], text=True).strip()
        rom = Path(value)
        if rom.stat().st_size != 1048576 or hashlib.sha1(rom.read_bytes()).hexdigest() != expected:
            raise ValueError('Wrong ROM revision for ' + game)
        inputs[game] = rom
    for game, rom in inputs.items():
        folder = ROOT / 'generated' / game
        valid = False
        if folder.is_dir():
            try:
                meta = json.loads((folder / 'generation.json').read_text())
                valid = (meta['game'] == game and meta['rom_sha1'] == module.HASHES[game]
                    and meta['compiler_sha256'] == compiler_hash
                    and meta['game_sha256'] == hashlib.sha256((folder / 'game.c').read_bytes()).hexdigest()
                    and (folder / ('oracle_' + game + '.h')).is_file())
            except (OSError, ValueError, KeyError):
                pass
        if valid:
            print('Reusing verified generated code for ' + game, flush=True)
            continue
        if folder.exists():
            backup = ROOT / 'build-setup-backups' / str(time.time_ns()) / game
            backup.parent.mkdir(parents=True)
            folder.rename(backup)
            print('Preserved previous generated code in ' + str(backup), flush=True)
        module.prepare(game, rom, compiler)

if __name__ == '__main__':
    main()
