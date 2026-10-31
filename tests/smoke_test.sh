#!/bin/bash
# Smoke test for bigfiles. Usage: smoke_test.sh <path-to-bigfiles>

set -euo pipefail

bigfiles="$1"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fail() {
	echo "FAIL: $1" >&2
	exit 1
}

# A directory with a space in its name, a sparse 5 GiB file (no real disk
# usage), a small file, and a symlink to the big file.
mkdir -p "$tmp/scan/a b"
big="$tmp/scan/a b/big file"
dd if=/dev/null of="$big" bs=1048576 seek=5120 count=0 2>/dev/null
echo "small" >"$tmp/scan/small.txt"
ln -s "$big" "$tmp/scan/link"

# The big file is listed exactly once, unquoted; the symlink is ignored.
output=$("$bigfiles" "$tmp/scan" "$tmp/log.txt")
expected="Files larger than 4 GB:
$big"
[ "$output" = "$expected" ] || fail "unexpected output:
$output"

# A missing directory fails with a message on stderr.
if "$bigfiles" "$tmp/missing" "$tmp/log.txt" 2>"$tmp/stderr.txt" >/dev/null; then
	fail "expected non-zero exit for a missing directory"
fi
grep -q "Cannot open directory" "$tmp/stderr.txt" || fail "no error on stderr"

echo "PASS"
