#!/usr/bin/env python3
"""
Wrapper script to generate all extension result code headers from extension_configs.json
"""

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(
        description="Generate all extension result code headers from config"
    )
    parser.add_argument(
        "-c",
        "--config",
        required=True,
        help="Path to extension_configs.json file",
    )
    parser.add_argument(
        "-s",
        "--source-dir",
        required=True,
        help="CMake source directory (CMAKE_CURRENT_SOURCE_DIR)",
    )
    args = parser.parse_args()

    # Read extension configs
    config_path = Path(args.config)
    if not config_path.exists():
        print(f"ERROR: Config file not found: {config_path}", file=sys.stderr)
        return 1

    with open(config_path, "r") as f:
        config = json.load(f)

    extensions = config.get("extensions", [])
    if not extensions:
        print("WARNING: No extensions found in config file", file=sys.stderr)
        return 0

    # Get the directory where this script lives (to find extension_result_code_generator.py)
    script_dir = Path(__file__).parent.resolve()
    generator_script = script_dir / "extension_result_code_generator.py"

    if not generator_script.exists():
        print(
            f"ERROR: Extension generator script not found: {generator_script}",
            file=sys.stderr,
        )
        return 1

    # Generate headers for each extension
    source_dir = Path(args.source_dir).resolve()
    failed_extensions = []

    for ext_config in extensions:
        ext_name = ext_config.get("name", "unknown")
        json_path_rel = ext_config.get("json_path")
        header_path_rel = ext_config.get("header_path")

        if not json_path_rel or not header_path_rel:
            print(
                f"WARNING: Extension '{ext_name}' missing json_path or header_path",
                file=sys.stderr,
            )
            continue

        # Resolve paths relative to config file directory
        config_dir = config_path.parent.resolve()
        json_path = (config_dir / json_path_rel).resolve()
        # The header path is just the filename like "swupdate_handler_result_codes.h"
        # We want to place it in src/inc/aduc/ alongside result.h so all targets can find it
        # This way we don't need to modify CMake include paths
        header_path = source_dir / "src" / "inc" / "aduc" / header_path_rel

        if not json_path.exists():
            print(f"WARNING: JSON file not found for '{ext_name}': {json_path}", file=sys.stderr)
            continue

        # Ensure output directory exists
        header_path.parent.mkdir(parents=True, exist_ok=True)

        print(f"Generating header for extension '{ext_name}'...")
        print(f"  JSON: {json_path}")
        print(f"  Output: {header_path}")

        # Run the generator
        result = subprocess.run(
            [
                sys.executable,
                str(generator_script),
                "-j",
                str(json_path),
                "-o",
                str(header_path),
            ],
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            print(f"  ERROR: Failed to generate header for '{ext_name}'", file=sys.stderr)
            print(f"  {result.stderr}", file=sys.stderr)
            failed_extensions.append(ext_name)
        else:
            print(f"  SUCCESS: Generated {header_path.name}")

    if failed_extensions:
        print(
            f"\nERROR: Failed to generate headers for: {', '.join(failed_extensions)}",
            file=sys.stderr,
        )
        return 1

    print(f"\nSuccessfully generated headers for {len(extensions)} extension(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
