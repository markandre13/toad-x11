#!/usr/bin/perl -w
#
# generate a report
#

use strict;

print <<"EOF";
TOAD testsuite report
---------------------
EOF

my $file;
my $result;

while($file=shift) {
  open(FD, "${file}") || die "${file}: $!";
  $result = <FD>;
  $result =~ s/\n$//;
  print "$file: $result\n";
  close(FD);
}
