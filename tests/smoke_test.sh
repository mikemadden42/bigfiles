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

# Runs bigfiles and expects it to fail with the given text on stderr.
expect_error() {
	local text="$1"
	shift
	if "$bigfiles" "$@" 2>"$tmp/stderr.txt" >/dev/null; then
		fail "expected non-zero exit for: $*"
	fi
	grep -q -- "$text" "$tmp/stderr.txt" || fail "expected '$text' on stderr for: $*"
}

# A directory with a space in its name, a sparse 5 GiB file (no real disk
# usage), files of 2 KiB, exactly 1 KiB, and 6 bytes, and a symlink to the
# big file.
mkdir -p "$tmp/scan/a b"
big="$tmp/scan/a b/big file"
dd if=/dev/null of="$big" bs=1048576 seek=5120 count=0 2>/dev/null
medium="$tmp/scan/a b/2k file"
head -c 2048 /dev/zero >"$medium"
head -c 1024 /dev/zero >"$tmp/scan/1k file"
echo "small" >"$tmp/scan/small.txt"
ln -s "$big" "$tmp/scan/link"

# Default limit: the big file is listed exactly once, unquoted; the symlink
# is ignored.
output=$("$bigfiles" "$tmp/scan")
expected="Files larger than 4G:
$big"
[ "$output" = "$expected" ] || fail "unexpected output:
$output"

# --min-size 1K: files strictly larger than 1024 bytes, in any order.
output=$("$bigfiles" --min-size 1K "$tmp/scan")
[ "$(head -n 1 <<<"$output")" = "Files larger than 1K:" ] || fail "unexpected header:
$output"
files=$(tail -n +2 <<<"$output" | sort)
expected=$(printf '%s\n' "$big" "$medium" | sort)
[ "$files" = "$expected" ] || fail "unexpected files for --min-size 1K:
$output"

# A missing directory fails with a message on stderr.
expect_error "Cannot open directory" "$tmp/missing"

# With --log, the error also goes to the log file with a timestamp.
expect_error "Cannot open directory" --log "$tmp/log.txt" "$tmp/missing"
grep -qE '^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} Error: Cannot open directory' \
	"$tmp/log.txt" || fail "no timestamped error in log file"

# Bad arguments fail with a message.
expect_error "invalid size" --min-size 4X "$tmp/scan"
expect_error "invalid size" --min-size -1 "$tmp/scan"
expect_error "invalid size" --min-size 99999999999999999999 "$tmp/scan"
expect_error "needs a value" "$tmp/scan" --min-size
expect_error "unknown option" --bogus "$tmp/scan"
expect_error "unexpected argument" "$tmp/scan" "$tmp/log.txt"
expect_error "Usage:"

# --help prints usage to stdout and succeeds.
"$bigfiles" --help | grep -q "Usage:" || fail "--help didn't print usage"

echo "PASS"
