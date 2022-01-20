#!/bin/bash

#enable -f sleep sleep

# trap 'trap "exit $?" SIGTERM && kill -- -$$' EXIT
# trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM

CASSINI=./cassini
PIPESDIR=./run/pipes
PIPESDIR_MAIN="$PIPESDIR"
PIPESDIR_ALT=./run/pipes-alt
PATTERN=cassini-grading-

TMP1=$(mktemp)
TMP2=$(mktemp)

REQUEST_PIPE=saturnd-request-pipe
REPLY_PIPE=saturnd-reply-pipe

TIMEOUT=3

usage() { echo "Usage: $0 <tests-directory>" 1>&2; exit 1; }

echo
echo '============================= grade-cassini.sh =============================='
echo

TESTSDIR="$1"
if [ -z "$TESTSDIR" ]; then
  usage
fi

if ! command -v timeout >/dev/null 2>&1
then
  echo "The command timeout (from package coreutils) is missing"
  exit 1
fi

if ! find "$TESTSDIR" -maxdepth 1 -name "${PATTERN}*" | grep . >/dev/null 2>&1
then
  echo "The tests have not been generated"
  exit 1
fi

if [ ! -x "$CASSINI" ]
then
  echo "The compiled executable $CASSINI is not present"
  exit 1
fi

if [ ! -d "$PIPESDIR" ]; then mkdir -p "$PIPESDIR"; fi
if [ ! -p "$PIPESDIR/$REQUEST_PIPE" ]; then mkfifo "$PIPESDIR/$REQUEST_PIPE"; chmod 600 "$PIPESDIR/$REQUEST_PIPE"; fi
if [ ! -p "$PIPESDIR/$REPLY_PIPE" ]; then mkfifo "$PIPESDIR/$REPLY_PIPE"; chmod 600 "$PIPESDIR/$REPLY_PIPE"; fi

if [ ! -d "$PIPESDIR_ALT" ]; then mkdir -p "$PIPESDIR_ALT"; fi
if [ ! -p "$PIPESDIR_ALT/$REQUEST_PIPE" ]; then mkfifo "$PIPESDIR_ALT/$REQUEST_PIPE"; chmod 600 "$PIPESDIR_ALT/$REQUEST_PIPE"; fi
if [ ! -p "$PIPESDIR_ALT/$REPLY_PIPE" ]; then mkfifo "$PIPESDIR_ALT/$REPLY_PIPE"; chmod 600 "$PIPESDIR_ALT/$REPLY_PIPE"; fi

normalize_output() {
  sed 's/://g;s/[[:blank:]]+/ /g;s/^ //g;s/ $//g;/^$/d' | sort -n -k1,1
  # TODO : maybe find a way of normalizing each timing field ?
}

print_failed() {
  printf "=============================== Test n°%d failed =============================\n" "$NB_TESTS"
}

run_test() {
  CURDIR="$1"
  CHECK_REQUEST_BEFORE_REPLY="$2"
  
  ARGS=()
  ARGS_ESC=()
  while read ARG; do
    ARGS+=("$ARG")
    if [[ ! "$ARG" =~ ^[-0-9A-Za-z.,_/]+$ ]]; then
      PATTERN="'"
      REPLACEMENT="'\\''"
      ARG_ESC="'${ARG//$PATTERN/$REPLACEMENT}'"
      ARGS_ESC+=("$ARG_ESC")
    else
      ARGS_ESC+=("$ARG")
    fi
  done < "$CURDIR/arguments"

  if [ "$CHECK_REQUEST_BEFORE_REPLY" = "--strict" ]; then
    {
      PIDS=()
      trap 'trap - SIGTERM; kill ${PIDS[@]}; exit 1' SIGTERM
      exec 4> "$TMP1"
      exec 3< "$PIPESDIR/$REQUEST_PIPE"
      timeout $TIMEOUT dd bs=1 count=2 <& 3 >& 4
      timeout $TIMEOUT dd if="$CURDIR/reply" > "$PIPESDIR/$REPLY_PIPE" &
      PIDS+=("$!")
      timeout $TIMEOUT dd <& 3 >& 4
    } 2>/dev/null &
    PID1=$!
  else
    {
      PIDS=()
      trap 'trap - SIGTERM; kill ${PIDS[@]}; exit 1' SIGTERM
      timeout $TIMEOUT dd < "$PIPESDIR/$REQUEST_PIPE" > "$TMP1" &
      PIDS+=("$!")
      timeout $TIMEOUT dd if="$CURDIR/reply" > "$PIPESDIR/$REPLY_PIPE"
      wait
    } 2>/dev/null &
    PID1=$!
  fi

  timeout $TIMEOUT $CASSINI -p "$PIPESDIR" "${ARGS[@]}" > "$TMP2" 2>/dev/null
  RES=$?
  timeout $TIMEOUT wait "$PID1" 2>/dev/null
  kill "$PID1" 2>/dev/null
  
  CMD="$CASSINI -p '$PIPESDIR' ${ARGS_ESC[@]}"

  # TODO: use printf instead of echo
  
  if [ $RES -eq 124 ]; then
    print_failed
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nTimed out (after $TIMEOUT seconds)"
    if [ "$CHECK_REQUEST_BEFORE_REPLY" = "--strict" ]; then
      printf "Note: the reason may be that $CASSINI tried to open the reply pipe before sending the request\n"
    fi
    return 1;
  fi

  if ! [ -s "$TMP1" ]; then
    print_failed
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nSent no request on $PIPESDIR/$REQUEST_PIPE"
    return 1
  fi
  
  if ! cmp "$TMP1" "$CURDIR/request" --silent; then
    print_failed
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nSent an incorrect request:"
    hexdump -C "$TMP1"
    echo -e "\nThe following request was expected:"
    hexdump -C "$CURDIR/request"
    return 1
  fi
  
  EXPECTED_RES=$(cat "$CURDIR/exitcode")
  if [ "$RES" -ne "$EXPECTED_RES" ]; then
    print_failed
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nWith reply:"
    hexdump -C "$CURDIR/reply"
    echo -e "\nExited with code $RES instead of $EXPECTED_RES"
    return 1
  fi

  if [ "$RES" -eq "0" ]; then
    if ! cmp <( cat "$TMP2" | normalize_output ) <( cat "$CURDIR/stdout" | normalize_output ) --silent; then
      print_failed
      echo -e "Command:\n"; echo "$CMD"
      echo -e "\nWith reply:"
      hexdump -C "$CURDIR/reply"
      echo -e "\nWrote an incorrect output on stdout:"
      cat "$TMP2"
      echo -e "\nThe following output was expected:"
      cat "$CURDIR/stdout"
      return 1
    fi
  fi
      
  return 0
}

cd "$TESTSDIR"
TESTS_LIST=()
for f in $(find . -maxdepth 1 -name "${PATTERN}*" | sort -n -t- -k3,3)
do
  TESTS_LIST+=("$TESTSDIR/$f")
done
cd - >/dev/null

NB_TESTS=0
NB_TESTS_PASSED=0

for f in "${TESTS_LIST[@]}"
do
  (( NB_TESTS++ ))

  if [ "$NB_TESTS" -eq 12 ]; then
    STRICT="--strict"
  else
    STRICT=""
  fi

  if [ "$NB_TESTS" -eq 11 ]; then
    PIPESDIR="$PIPESDIR_ALT"
  else
    PIPESDIR="$PIPESDIR_MAIN"
  fi
  
  if run_test "$f" $STRICT; then
    (( NB_TESTS_PASSED++ ))
  else
    printf "\n"
  fi
done

rm -f "$TMP1"
rm -f "$TMP2"

if [ "$NB_TESTS_PASSED" -lt "$NB_TESTS" ]; then
  printf "==============================================================================\n\n"
fi

printf "Tests passed: %d out of %d\n" "$NB_TESTS_PASSED" "$NB_TESTS"
export LANG="C"
printf "Grade: %.2f / 4\n" $(( $NB_TESTS_PASSED * 400 / $NB_TESTS))e-2

