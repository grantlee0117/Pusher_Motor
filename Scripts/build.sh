#!/usr/bin/env bash
set -euo pipefail

preset="${1:-Debug}"

cmake --fresh --preset "$preset"
cmake --build --preset "$preset"
