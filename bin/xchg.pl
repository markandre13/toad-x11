#!/usr/bin/perl

# XCHG 0.1 -- exchanges strings in files
# written 1997 by Mark-André Hopf

if ($#ARGV < 2)
{
  print "usage: xchg oldstring newstring files ...\n\n";
  exit(0);
}

$olds = $ARGV[0];
$news = $ARGV[1];

print "converting from '$olds' to '$news'\n";

$i = 2;
foreach $i ( 2 .. $#ARGV)
{
	$oldf = $ARGV[$i];
	$newf = "$oldf.bak";
	rename($oldf,$newf);
	
	# open files and create new filehandles 'FD_IN' and 'FD_OUT'
	#-----------------------------------------------------------
	open(FD_IN, "<$newf") || warn "xchg: couldn't open $newf: $!\n";
	open(FD_OUT,">$oldf") || warn "xchg: couldn't open $oldf: $!\n";	
	# loop for all lines in 'FD_IN'
	#------------------------------
	while(<FD_IN>)
	{
		# do a substitution on the current line:
		# s/         : invokes substitute command
		# \b$olds$\b : matching pattern: \b=word boundary, $olds=old string 
		# /          : just to separate the pattern from the replacement
		# $news	     : replacement
		# /g	     : replace all occurences of the pattern
		#-------------------------------------------------------------------
		s/\b$olds\b/$news/g;
		print FD_OUT $_;
	}
	close(FD_IN);
	close(FD_OUT);
}
