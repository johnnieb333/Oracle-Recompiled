#!/usr/bin/env python3
"""Generate one game locally from an exact supported ROM; never overwrite output."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HASHES = {"ages": "880374fb978b18af4aa529e2e32f7ffb4d7dd2f4",
          "seasons": "ba1268290fb2b1b70505d2d7b5825fc8a4816a4b"}

def externalize(source, prefix):
    declaration = "extern const uint8_t rom_data[];"
    initializer = ("void " + prefix + "_init(GBContext* ctx) {\n"
        "    /* Load ROM data into context */\n"
        "    gb_context_load_rom(ctx, rom_data, 1048576);\n"
        "    /* Set MBC type from header */\n"
        "    ctx->mbc_type = rom_data[0x147];\n}\n")
    if source.count(declaration) != 1 or source.count(initializer) != 1:
        raise ValueError("Unexpected compiler output; refusing to alter it.")
    result = source.replace(declaration, "/* ROM is supplied through the external loader. */", 1)
    result = result.replace(initializer, "/* Initialization is provided by oracle_main.c. */\n", 1)
    if re.search(r"\brom_data\b", result):
        raise ValueError("Unexpected remaining embedded-ROM reference.")
    return result

def prepare(game, rom, compiler):
    rom = Path(rom).resolve()
    compiler = Path(compiler).resolve()
    destination = ROOT / "generated" / game
    if destination.exists():
        raise ValueError(str(destination) + " already exists; nothing was changed.")
    if rom.stat().st_size != 1048576:
        raise ValueError("ROM must be exactly 1,048,576 bytes.")
    data = rom.read_bytes()
    if len(data) != 1048576 or hashlib.sha1(data).hexdigest() != HASHES[game]:
        raise ValueError("Wrong ROM revision for " + game + ".")
    if not compiler.is_file():
        raise ValueError("Build the supplied compiler first: " + str(compiler))
    destination.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="oracle-generate-") as work:
        work = Path(work)
        prefix = "oracle_" + game
        local_rom = work / (prefix + ".gbc")
        local_rom.write_bytes(data)
        subprocess.run([str(compiler), str(local_rom), "-o", str(work / "output")], check=True)
        output = work / "output"
        source = (output / (prefix + ".c")).read_text(encoding="utf-8")
        header = (output / (prefix + ".h")).read_bytes()
        modified = externalize(source, prefix)
        # Stage beside the destination, then rename only after all checks pass.
        with tempfile.TemporaryDirectory(prefix=".stage-", dir=destination.parent) as stage:
            ready = Path(stage) / game
            ready.mkdir()
            (ready / "game.c").write_text(modified, encoding="utf-8", newline="\n")
            (ready / (prefix + ".h")).write_bytes(header)
            (ready / "generation.json").write_text(json.dumps({
                "game": game, "rom_sha1": HASHES[game],
                "compiler_sha256": hashlib.sha256(compiler.read_bytes()).hexdigest(),
                "game_sha256": hashlib.sha256((ready / "game.c").read_bytes()).hexdigest()
            }, indent=2) + "\n")
            if destination.exists():
                raise ValueError("Output appeared during generation; refusing replacement.")
            ready.rename(destination)
    print("Generated " + game + " locally in " + str(destination))
    print("ROM arrays and generated entry points were not retained.")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", choices=HASHES)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--compiler", default=str(ROOT / "build-tools" / "bin" /
                        ("gbrecomp.exe" if os.name == "nt" else "gbrecomp")))
    args = parser.parse_args()
    try:
        prepare(args.game, args.rom, args.compiler)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Preparation failed: " + str(error) + "\n")

if __name__ == "__main__":
    main()
