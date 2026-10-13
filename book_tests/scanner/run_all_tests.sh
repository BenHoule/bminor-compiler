#!/bin/sh

DIR=$(dirname "$0")
BMINOR="$DIR"/../../../bminor

for testfile in "$DIR"/good*.bminor
do
	if $BMINOR -scan "$testfile" > "$testfile".out
	then
		echo "$testfile success (as expected)"
	else
		echo "$testfile failure (INCORRECT)"
	fi
done

for testfile in "$DIR"/bad*.bminor
do
	if $BMINOR -scan "$testfile" > "$testfile".out
	then
		echo "$testfile success (INCORRECT)"
	else
		echo "$testfile failure (as expected)"
	fi
done
