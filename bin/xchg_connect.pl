#!/usr/bin/perl
#
# this script tries to convert CONNECT macros from TOAD versions before
# 0.42.20 into the new syntax using this pattern
#
# CONNECT(a,b,c,d)
# to
# OLD_CONNECT(source_signal, dest_object, dest_method)
#
# this script silently fails when your program contains CONNECT statments
# with arguments containing `,' or `)'.
#

print "$#ARGV\n";

if ($#ARGV < 0) {
  print "usage: xchg_connect.pl files...\n\n";
  exit(0);
}

foreach $i ( 0 .. $#ARGV) {
  $newf = $ARGV[$i];
  $bakf = "$newf.bak";
  rename($newf,$bakf);

  open(FD_IN, "<$bakf") || warn "xchg: couldn't open $bakf: $!\n";
  open(FD_OUT,">$newf") || warn "xchg: couldn't open $newf: $!\n";
  
  while(<FD_IN>) {
    s/CONNECT\s*\(([^,]+),([^,]+),([^,]+),([^)]+)\)/OLD_CONNECT\($1, $2, $3, $4\)/;
    print FD_OUT $_;
  }

  close(FD_IN);
  close(FD_OUT);
}
