#!/usr/bin/perl -w
#
# This code takes as input N data files 
# and generates output of the form X Y Ymin Ymax for use with Gnuplot's
# errorbars.
while (<>)
  { 
      @tmp = split " ", $_; 
      #      print $tmp[0], " ", $tmp[1], "\n"; 
      if ((!defined($mins{$tmp[0]})) || ($mins{$tmp[0]} > $tmp[1]))
      { 
	  $mins{$tmp[0]} = $tmp[1];
      }
      if ((!defined($maxs{$tmp[0]})) || ($maxs{$tmp[0]} < $tmp[1]))
      { 
	  $maxs{$tmp[0]} = $tmp[1];
      }
      if (!defined($sum{$tmp[0]}))
      { 
	  $sum{$tmp[0]} = 0.0;
      }
      $sum{$tmp[0]} += $tmp[1];
      if (!defined($num{$tmp[0]}))
      { 
	  $num{$tmp[0]} = 0;
      }
      $num{$tmp[0]} += 1;
  }

foreach $key (sort {$a <=> $b} keys %mins)
{
    printf "%d %f %f %f\n",$key,$sum{$key}/($num{$key}),$mins{$key},$maxs{$key};
}
