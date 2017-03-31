#!/usr/bin/perl

use strict;
use warnings;
use feature 'say';

say "/* THIS IS GENERATED FILE! DO NOT EDIT! DO NOT COMMIT! */";
say '#include <testing/unity_fixture.h>';
say '';

my @run_test_group;
foreach my $file (@ARGV)
{
	my @output = qx(grep -vh '#include' $file | $ENV{CPP} -);
	my $test_group_found = 0;
	foreach (@output)
	{
		s/\r//g;
		chomp;
		if (m/^TEST_GROUP\(/)
		{
			s/^TEST_GROUP/TEST_GROUP_RUNNER/;
			s/;$//;
			say "}\n" if ($test_group_found);
			say $_;
			say '{';
			$test_group_found = 1;
			s/TEST_GROUP_RUNNER/\tRUN_TEST_GROUP/;
			push @run_test_group, $_;
		}
		elsif (m/^TEST\(/)
		{
			s/^TEST/\tRUN_TEST_CASE/;
			say "$_;";
		}
		elsif (m/^TEST_WITH_TIMEOUT\(/)
		{
			s/^TEST_WITH_TIMEOUT/\tRUN_TEST_CASE/;
			s/,\s+[^,]+\)$/)/;
			say "$_;";
		}
	}
	say "}\n" if ($test_group_found);
}

