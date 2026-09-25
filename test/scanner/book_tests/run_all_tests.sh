#!/bin/sh

for testfile in /home/benh/CompilerConstruction/b-minor/test/scanner/book_tests/good*.bminor
do
	if /home/benh/CompilerConstruction/b-minor/bminor -scan $testfile > $testfile.out
	then
		echo "$testfile success (as expected)"
	else
		echo "$testfile failure (INCORRECT)"
	fi
done

for testfile in /home/benh/CompilerConstruction/b-minor/test/scanner/book_tests/bad*.bminor
do
	if /home/benh/CompilerConstruction/b-minor/bminor -scan $testfile > $testfile.out
	then
		echo "$testfile success (INCORRECT)"
	else
		echo "$testfile failure (as expected)"
	fi
done
