#!/usr/bin/env bash
set -euo pipefail

# Apply the behavior-preserving diagnostic detail formatter migration.
# Run from the repository root after PR #5 has been merged.

patch -p1 < patches/access_migrate_diagnostic_detail_gates.patch

printf 'Applied diagnostic detail formatter migration patch.\n'
printf 'Recommended validation:\n'
printf '  make test\n'
printf '  make CXXSTD=c++17 test\n'
printf '  make strict\n'
printf '  make smoke\n'
