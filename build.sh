#!/bin/sh
# Build the real device image on Linux, or with the base's Docker backend.
set -eu
cd "$(dirname "$0")"
export X0X_FONTSET=terminus
export JIELI_TOOLCHAIN="${JIELI_TOOLCHAIN:-$HOME/.jieli/toolchain}"
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
exec "${PYTHON:-python3}" upstream/base/tools/build.py firmware/app "$@"
