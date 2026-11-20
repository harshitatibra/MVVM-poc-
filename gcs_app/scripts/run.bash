#!/usr/bin/env bash
set -eo pipefail
source "$(dirname -- "${BASH_SOURCE[0]}")/environment.bash"
if [[ ! -x "$GCS_ROOT/build/gcs" ]]; then
  echo 'Build with scripts/build.bash first, or run the gcs target inside Qt Creator.' >&2
  exit 1
fi
exec "$GCS_ROOT/build/gcs" "$@"
