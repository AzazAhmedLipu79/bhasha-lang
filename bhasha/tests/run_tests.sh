#!/usr/bin/env bash
# Test runner for Bhasha compiler.
# - Verifies valid programs compile and execute correctly
# - Verifies error programs fail gracefully (no crash, exit code 1)
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

pass=0
fail=0
failures=()

ok() { pass=$((pass + 1)); echo "  PASS  $1"; }
ko() {
  fail=$((fail + 1))
  failures+=("$1")
  echo "  FAIL  $1"
}

# ----- Valid programs (must compile, must produce expected stdout) -----
# Format: "<source.bng>|<expected program stdout>"
valid_cases=(
  "examples/hello.bng|হ্যালো বিশ্ব!"
  "examples/variables.bng|30"
  "examples/age_check.bng|তুমি ১০ বা তার বেশি বয়সী"
  "examples/grade_check.bng|খুব ভালো"
  "examples/arithmetic.bng|14
20"
  "examples/calculator.bng|10
4
21
2"
  "examples/counting.bng|1
2
3
4
5"
  "examples/factorial.bng|120"
  "examples/sum_numbers.bng|55"
)

for case in "${valid_cases[@]}"; do
  src="${case%%|*}"
  expected="${case#*|}"
  actual=$(./bhashac "$src" --run 2>/dev/null | awk '/--- Running generated Python ---/{flag=1; next} flag')
  if [ "$actual" = "$expected" ]; then
    ok "compile+run $src"
  else
    ko "compile+run $src (expected='$expected' got='$actual')"
  fi
done

# ----- Error programs (must NOT crash; must exit non-zero) -----
err_programs=(
  examples/type_error.bng
  examples/syntax_error.bng
)

for src in "${err_programs[@]}"; do
  set +e
  ./bhashac "$src" >/dev/null 2>/dev/null
  rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    ok "rejects $src (exit=$rc)"
  else
    ko "rejects $src (expected nonzero exit)"
  fi
done

# ----- Crash safety: feed malformed inputs -----
echo "=== Crash safety ==="
crash_inputs=(
  $'%s%s%s%s%s'
  $'\x00\x01\x02'
  "দেখাও("
  "সংখ্যা x ="
  "যদি (x > 5)"
  "}"
  ""
  "দেখাও দেখাও দেখাও("
)
tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" EXIT

i=0
for input in "${crash_inputs[@]}"; do
  src="$tmpdir/crash_$i.bng"
  printf "%s" "$input" > "$src"
  set +e
  ./bhashac "$src" >/dev/null 2>/dev/null
  rc=$?
  set -e
  if [ "$rc" -le 1 ]; then
    ok "crash-safe input #$i (exit=$rc)"
  else
    ko "crash-safe input #$i (exit=$rc, >1 means crash)"
  fi
  i=$((i + 1))
done

# ----- Summary -----
echo
if [ "$fail" -eq 0 ]; then
  echo "All $pass tests passed."
  exit 0
else
  echo "FAILED: $fail of $((pass + fail)) tests failed."
  for f in "${failures[@]}"; do echo "  - $f"; done
  exit 1
fi

