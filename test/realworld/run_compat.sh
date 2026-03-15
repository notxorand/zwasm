#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
exec python3 test/realworld/run_compat.py "$@"
