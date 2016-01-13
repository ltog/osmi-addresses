#!/bin/bash

# Author:  Lukas Toggenburger
# Website: https://github.com/ltog/osmi-addresses

description="This tool compares the contents of two sqlite/spatialite files or directories containing such files and prints their difference if any."

parallel_options="--noswap"

# read script input arguments
path1="$1"
path2="$2"

# delete the temporary directory and its files
cleanup() {
	rm -rf "$tmpdir"
}
export -f cleanup

# create temporary directory
export tmpdir=$(mktemp --tmpdir --directory "$(basename ${0})-XXXXXXXXXX") || {
	echo "ERROR: Could not create temporary directory.";
	cleanup
	exit 1;
}

# make sure exactly two arguments are given
if [ $# -ne 2 ]; then
	echo "Usage: $0 (file1.sqlite file2.sqlite | dir_with_sqlite_files1 dir_with_sqlite_files2)"
	echo "Description: $description"
	exit 1
fi

# define a function to call a difftool (implemented this way so 'diff' can be used when 'colordiff' ist not installed)
difftool() {
	if hash colordiff 2>/dev/null; then # from http://stackoverflow.com/a/677212
		colordiff -y --suppress-common-lines "$@"
	else
		diff -y --suppress-common-lines "$@"
	fi
}
export -f difftool

handle_signals() {
	echo "Signal handling function was called. Going to clean up..."
	cleanup
	exit 1
}
export -f handle_signals
trap handle_signals SIGINT SIGTERM

# compare the contents of the tables
print_diff_of_all_tables() {
	local file1="$1"
	local file2="$2"
	local tmpfile1=$(mktemp --tmpdir="$tmpdir" "tmpfile1-XXXXXXXXXXXXXXXXXX") || {
		echo "ERROR: Could not create temporary file.";
		cleanup
		exit 1;
	}
	local tmpfile2=$(mktemp --tmpdir="$tmpdir" "tmpfile2-XXXXXXXXXXXXXXXXXX") || {
		echo "ERROR: Could not create temporary file.";
		cleanup
		exit 1;
	}

	# make note of the tables names of the two files (can't use ".tables" since its output has two columns)
	local tables1=$(sqlite3 "$file1" '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)
	local tables2=$(sqlite3 "$file2" '.schema' | grep "CREATE TABLE '" | sed -e "s/^CREATE TABLE '\([^']*\)'.*$/\1/" | sort)

	# make sure the table names are the same
	if [ "$tables1" != "$tables2" ]
	then
		difference=$(echo -e "${tables1}\n${tables2}" | sort | uniq -u) # store the table names that occurred only once
		echo "ERROR: Tables in $file1 are not the same. Look out for table(s):"
		echo "$difference"
		exit 1
	fi

	while read -r table; do
		echo "Checking table $table"

		# read column info
		sqlite3 "$file1" "PRAGMA table_info($table)" > "$tmpfile1"
		sqlite3 "$file2" "PRAGMA table_info($table)" > "$tmpfile2"

		# read rows
		sqlite3 "$file1" "SELECT * FROM $table" >> "$tmpfile1"
		sqlite3 "$file2" "SELECT * FROM $table" >> "$tmpfile2"

		# print diff
		difftool "$tmpfile1" "$tmpfile2"

	done <<< "$tables1"

	rm -rf "$tmpfile1"
	rm -rf "$tmpfile2"
}
export -f print_diff_of_all_tables

if [[ -f "$path1" && -f "$path2" ]]; then # both args are files

	print_diff_of_all_tables "$path1" "$path2"

elif [[ -d "$path1" && -d "$path2" ]]; then  # both args are directories

	# check if filenames in both directories are identical
	filenames1=$(find "$path1" -maxdepth 1 -type f -printf '%f\n')
	filenames2=$(find "$path2" -maxdepth 1 -type f -printf '%f\n')
	filenamesdiff=$(echo -e "${filenames1}\n${filenames2}" | sort | uniq -u) # combine file lists, remove double entries
	if [[ ! -z "$filenamesdiff" ]]; then
		echo "ERROR: The files in the given directories differ. Look out for file(s):"
		echo "$filenamesdiff"
		exit 1
	fi

	# aggregate list of files
	# the previous lists cannot be recycled since they contain the filenames without the directory's name
	files1=$(find "$path1" -maxdepth 1 -type f | sort)
	files2=$(find "$path2" -maxdepth 1 -type f | sort)

	mkdir -p "$tmpdir" # will be deleted by cleanup function
	parallel $parallel_options --xapply print_diff_of_all_tables ::: "$files1" ::: "$files2"

else
	echo "ERROR: $path1 and $path2 are not of the same type (file/directory)"
	exit 1
fi

cleanup

