#!/bin/bash

#enable -f sleep sleep

CASSINI=./cassini
PIPESDIR=./run/pipes
TESTSDIR=./tests
PATTERN=cassini-test-

TMP1=$(mktemp)
TMP2=$(mktemp)

REQUEST_PIPE=saturnd-request-pipe
REPLY_PIPE=saturnd-reply-pipe

TIMEOUT=3

if ! command -v timeout >/dev/null 2>&1
then
  echo "The command timeout (from package coreutils) is missing"
  exit 1
fi

if ! find "$TESTSDIR" -maxdepth 1 -name "cassini-test-*" | grep . >/dev/null 2>&1
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
#if [ ! -e "$PIPESDIR/cassini-lock" ]; then touch "$PIPESDIR/cassini-lock"; fi

normalize_output() {
  sed 's/://g;s/[[:blank:]]+/ /g;s/^ //g;s/ $//g;/^$/d' | sort -n -k1,1
  # TODO : maybe find a way of normalizing each timing field ?
}

run_test() {
  CURDIR="$1"
  
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

  timeout $TIMEOUT cat "$CURDIR/reply" > "$PIPESDIR/$REPLY_PIPE" &
  PID1=$!
  timeout $TIMEOUT cat "$PIPESDIR/$REQUEST_PIPE" > "$TMP1" &
  PID2=$!
  timeout $TIMEOUT $CASSINI -p "$PIPESDIR" "${ARGS[@]}" > "$TMP2" 2>/dev/null
  RES=$?
  CMD="$CASSINI -p '$PIPESDIR' ${ARGS_ESC[@]}"


  # TODO: use printf instead of echo
  
  if [ $RES -eq 124 ]; then
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nTimed out (after $TIMEOUT seconds)"
    return 1;
  fi

  if ! [ -s "$TMP1" ]; then
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nSent no request on $PIPESDIR/$REQUEST_PIPE"
    return 1
  fi
  
  if ! cmp "$TMP1" "$CURDIR/request" --silent; then
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nSent an incorrect request:"
    hexdump -C "$TMP1"
    echo -e "\nThe following request was expected:"
    hexdump -C "$CURDIR/request"
    return 1
  fi
  
  EXPECTED_RES=$(cat "$CURDIR/exitcode")
  if [ "$RES" -ne "$EXPECTED_RES" ]; then
    echo -e "Command:\n"; echo "$CMD"
    echo -e "\nWith reply:"
    hexdump -C "$CURDIR/reply"
    echo -e "\nExited with code $RES instead of $EXPECTED_RES"
    return 1
  fi

  if [ "$RES" -eq "0" ]; then
    if ! cmp <( cat "$TMP2" | normalize_output ) <( cat "$CURDIR/stdout" | normalize_output ) --silent; then
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
    
  if ps -p $PID1 > /dev/null; then kill $PID1; fi
  if ps -p $PID2 > /dev/null; then kill $PID2; fi
  
  return 0
}

echo_red() {
  if [ -t 1 ] && command -v tput >/dev/null 2>&1; then
    echo -n "$(tput setaf 1)$(tput bold)" 2>/dev/null
    echo "$@"
    echo -n "$(tput sgr0)" 2>/dev/null
  else
    echo "$@"
  fi
}

PASSED=true

for f in $(find "$TESTSDIR" -maxdepth 1 -name "cassini-test-*" | sort -n -t- -k3,3)
do
  if ! run_test "$f"; then
    echo -ne "\n"
    PASSED=false
    WHICH_TEST_FAILED="$f"
    break
  fi
done

rm -f "$TMP1"
rm -f "$TMP2"

if $PASSED; then
  echo "All tests passed"
else
  echo_red -ne "Test failed: "
  echo "$WHICH_TEST_FAILED"
fi

exec $PASSED
