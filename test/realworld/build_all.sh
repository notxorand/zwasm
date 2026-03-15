#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
exec python3 test/realworld/build_all.py "$@"
