#!/usr/bin/perl
use warnings;
use strict;
use Term::ANSIColor qw(:constants);
$Term::ANSIColor::AUTORESET = 1; # doesn't seem to work

my $directory = $ARGV[0];
my $name      = $ARGV[1];
my $query     = $ARGV[2];
my $file      = $ARGV[3];
my $op        = $ARGV[4];
my $expected  = $ARGV[5];
my $result;

# replace ** with name of the file (which is also the name of the table)
$name  =~ s/\Q**\E/$file/g;
$query =~ s/\Q**\E/$file/g;

# call code-branches for specific operators
if ($op eq '=') {
	$result = qx(spatialite "$directory/$file.sqlite" '$query');
	chomp $result;

	exit &print_result;
} elsif ($op =~ /bbox/) {
	my @values = split(';', $query);
	my $table     = $values[0];
	my $left      = $values[1]; 
	my $right     = $values[2];
	my $bottom    = $values[3];
	my $top       = $values[4];
	my $condition = $values[5];
	my $assembled_query;

	if ($op eq 'outofbbox') {
		# queries from http://giswiki.hsr.ch/SpatiaLite_-_Tipps_und_Tricks
		$assembled_query = "SELECT COUNT(*) FROM $table WHERE ROWID IN (SELECT pkid FROM idx_${table}_GEOMETRY WHERE (xmin>$right OR xmax<$left OR ymin>$top OR ymax<$bottom) ";
	} elsif ($op eq 'inbbox') {
		$assembled_query = "SELECT COUNT(*) FROM $table WHERE ROWID IN (SELECT pkid FROM idx_${table}_GEOMETRY WHERE (xmin<$left AND xmax>$right AND ymin<$bottom AND ymax>$top) ";
	} else {
		exit &report_unknown_operator;
	}

	if (defined $condition) {
		$assembled_query .= " AND $condition)"; 
	} else {
		$assembled_query .= ")";
	}
	$result = qx(spatialite "$directory/$file.sqlite" '$assembled_query');
	chomp $result;

	exit &print_result;
} else {
	exit &report_unknown_operator;
}

# print the test result
sub print_result {
	if ($result eq $expected) {
		print BOLD, GREEN, "PASS: ", RESET, BRIGHT_BLACK, "$name\n", RESET;
		return 0; # success
	} else {
		print BOLD, RED, "FAIL: ", RESET, BOLD, "$name: Expected '$expected', but result was '$result'\n", RESET;
		return 1; # failure
	}
}

# report unknown test operators
sub report_unknown_operator {
	print BOLD, RED, "FAIL: ", RESET, "$name: unknown operator\n", RESET;
	return 2; # failure
}
