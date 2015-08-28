#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage: $0 OSM_INFILE"
	exit 1
fi

if [ ! -f $1 ]; then
	echo "$1 is not a file."
	exit 1
fi

export tmpoutfile="/tmp/pmp-deleteme.sqlite"
export stfile="./pmp-stacktraces"
export processed_stfile="/tmp/pmp-stacktraces"
export infile="$1"

cleanup() {
	rm -f $tmpoutfile
	rm -f ${tmpoutfile}-journal
	rm -f $processed_stfile
}

trap cleanup SIGINT SIGTERM EXIT

cleanup

sudo true # ask for sudo password so the next two commands may start immediately after each other

../osmi/osmi $infile $tmpoutfile & sudo ./pmp.sh -i 0 -k $stfile -s 0.1 > /dev/null

tac $stfile | awk -f cleanup-osmi-trace.awk - | tac > $processed_stfile

./pmp.sh $processed_stfile

# cleanup is called by trap function at the end of the program
