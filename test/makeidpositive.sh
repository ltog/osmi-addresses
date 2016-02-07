#!/bin/bash

if [ $# -ne 1  ]; then
	echo "Usage: $0 file.xml"
	echo "$0 creates a new file pos-file.xml with all negative attributes id= and ref= set to their absolute value."
	echo "$0 relies on 'sed' instead of a proper XML parser ('xmlstarlet' would probably be a fine choice) but should work fine as long the id= and ref= attributes have no linebreaks in them."
	exit 1
fi

cat $1 | sed -e "s/\(id\|ref\)=\('\|\"\)-\([0-9]\+\)\('\|\"\)/\1='\3'/g" > pos-$1
