#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
exec python3 test/e2e/run_e2e.py "$@"
