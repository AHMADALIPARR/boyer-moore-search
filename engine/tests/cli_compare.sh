#!/bin/bash
# cli_compare.sh — byte-for-byte CLI comparison of the CompCert-core
# binary against a GCC reference binary.
#
# Usage: ./tests/cli_compare.sh [path-to-gcc-bm]
# Default reference: the GCC binary of SNAPKITTYWEST/boyer-moore-search.
# Compares stdout, stderr, and exit code on 16 cases (stdin, file
# input, binary data, large input, error cases). Exits 0 only if all
# cases match.
set -u
GCC="${1:-$HOME/workspace/boyer-moore-search/bin/bm}"
CCP="$(dirname "$0")/../bin/bm_compcert"
pass=0; fail=0
run_case() {
  local desc="$1"; shift
  local input="$1"; shift
  local gout gcode cout ccode
  if [ -n "$input" ]; then
    gout=$(printf '%s' "$input" | "$GCC" "$@" 2>&1); gcode=$?
    cout=$(printf '%s' "$input" | "$CCP" "$@" 2>&1); ccode=$?
  else
    gout=$("$GCC" "$@" 2>&1); gcode=$?
    cout=$("$CCP" "$@" 2>&1); ccode=$?
  fi
  local gout_n cout_n
  gout_n=$(printf '%s' "$gout" | sed "s|$GCC|BIN|g")
  cout_n=$(printf '%s' "$cout" | sed "s|$CCP|BIN|g")
  if [ "$gout_n" = "$cout_n" ] && [ "$gcode" = "$ccode" ]; then
    pass=$((pass+1)); echo "PASS: $desc (out='$gout_n' exit=$gcode)"
  else
    fail=$((fail+1)); echo "FAIL: $desc gcc(out='$gout_n' exit=$gcode) compcert(out='$cout_n' exit=$ccode)"
  fi
}
run_case "README example stdin" "XABCABCD" "ABCD"
run_case "full match stdin" "XABCABCD" "ABCABCD"
run_case "not found stdin" "XABCABCD" "XYZ"
run_case "empty pattern" "ABC" ""
run_case "pattern longer than text" "ABC" "ABCD"
run_case "no args" ""
run_case "single char" "A" "A"
run_case "hello world" "hello world" "world"
run_case "repeated" "AAAAAAAAAA" "AAA"
run_case "overlap" "ABCABCABC" "ABCABC"
printf 'The quick brown fox jumps over the lazy dog. The quick brown fox.' > /tmp/cli_text.txt
run_case "file input found" "" "fox" /tmp/cli_text.txt
run_case "file input not found" "" "cat" /tmp/cli_text.txt
run_case "missing file" "" "fox" /tmp/does-not-exist.txt
printf 'A\0BCDE' > /tmp/cli_bin.dat
run_case "binary file" "" "BC" /tmp/cli_bin.dat
python3 -c "import sys; sys.stdout.write('A'*100000 + 'NEEDLE' + 'B'*100000)" > /tmp/cli_big.txt
run_case "large file match" "" "NEEDLE" /tmp/cli_big.txt
run_case "large file no match" "" "HAYSTACK" /tmp/cli_big.txt
echo "CLI comparison: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
