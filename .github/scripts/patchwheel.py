"""
This is a modified script for patching rpath,
originally from https://github.com/arbor-sim/arbor
"""

import shutil
import subprocess
import argparse
from pathlib import Path


def parse_arguments():
    parser = argparse.ArgumentParser(
        description=("Patch AFDKO wheels built with scikit-build and"
                     " corrected by auditwheel. Linux only.")
    )
    parser.add_argument(
        "path",
        type=dir_path,
        help=("The path where your wheels are located."
              " They will be patched in place."),
    )
    parser.add_argument(
        "-ko",
        "--keepold",
        action="store_true",
        help="If you want to keep the old wheels in /old",
    )

    return parser.parse_args()


def dir_path(path):
    path = Path(path)
    if Path.is_dir(path):
        return path
    else:
        raise argparse.ArgumentTypeError(f"{path} is not a valid path")


parsed_args = parse_arguments()
Path.mkdir(parsed_args.path / "old", exist_ok=True)

for inwheel in parsed_args.path.glob("*.whl"):
    zipdir = Path(f"{inwheel}.unzip")

    subprocess.check_call(f"unzip {inwheel} -d {zipdir}", shell=True)

    libxml2n = list(zipdir.glob("**/libxml2*.so*"))[0]
    subprocess.check_call(f"patchelf --set-rpath '$ORIGIN' {libxml2n}",
                          shell=True)

    # TODO? correct checksum/bytecounts in *.dist-info/RECORD.
    # So far, Python does not report mismatches

    outwheel = Path(shutil.make_archive(inwheel, "zip", zipdir))
    Path.rename(inwheel, parsed_args.path / "old" / inwheel.name)
    Path.rename(outwheel, parsed_args.path / inwheel.name)
    shutil.rmtree(zipdir)

if not parsed_args.keepold:
    shutil.rmtree(parsed_args.path / "old")
