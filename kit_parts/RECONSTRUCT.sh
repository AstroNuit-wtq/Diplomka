#!/bin/bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-Diplomka_Native_CPP_Source.zip}"
TMP=$(mktemp)
for f in $(ls "$DIR"/p_*.b64 | sort); do
  base64 -d "$f" >> "$TMP"
done
mv "$TMP" "$OUT"
echo "OK: $OUT ($(wc -c < "$OUT") bytes)"
