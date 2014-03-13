#!/usr/bin/perl
use warnings;
use strict;
use Term::ANSIColor qw(:constants);
use feature qw/switch/;

my $file     = $ARGV[0];
my $name     = $ARGV[1];
my $query    = $ARGV[2];
my $op       = $ARGV[3];
my $expected = $ARGV[4];
my $result;

given($op) {
	when('=') {
		$result = qx(sqlite3 $file '$query');
		chomp $result;
	
		&print_result;
	}
	
	when(/bbox/) {
		my @values = split(';', $query);
		my $table     = $values[0];
		my $left      = $values[1]; 
		my $right     = $values[2];
		my $bottom    = $values[3];
		my $top       = $values[4];
		my $condition = $values[5];
		my $assembled_query;

		given($op) {
			# queries from http://giswiki.hsr.ch/SpatiaLite_-_Tipps_und_Tricks
			when('outofbbox') {
				$assembled_query = "SELECT COUNT(*) FROM $table WHERE ROWID IN (SELECT pkid FROM idx_${table}_GEOMETRY WHERE (xmin>$right OR xmax<$left OR ymin>$top OR ymax<$bottom) ";
			}
			when('inbbox') {
				$assembled_query = "SELECT COUNT(*) FROM $table WHERE ROWID IN (SELECT pkid FROM idx_${table}_GEOMETRY WHERE (xmin<$left AND xmax>$right AND ymin<$bottom AND ymax>$top) ";
			}
			default {
				&report_unknown_operator;
			}
		}
		if (defined $condition) {
			$assembled_query .= " AND $condition)"; 
		} else {
			$assembled_query .= ")";
		}
		$result = qx(sqlite3 $file '$assembled_query');
		chomp $result;
	
		&print_result;
	} 
	
	default {
		&report_unknown_operator;
	}
}


sub print_result {
	if ($result eq $expected) {
		print BOLD, GREEN, "PASS: ", RESET, "$name\n";
	} else {
		print BOLD, RED, "FAIL: ", RESET, "$name: Expected '$expected', but result was '$result'\n";
	}
}

sub report_unknown_operator {
	print BOLD, RED, "FAIL: ", RESET, "$name: unknown operator\n";
}
