#!/usr/bin/tclsh

# Writes the first n AF frames from the input file into the output file.

# Read global variables
source settings.tcl

package require rsciUtil
package require rsci

set STATUS_PROTOCOL FILE

# main program

puts "argc=$argc"
set infilename [lindex $argv 0]
set infile [open $infilename r]
fconfigure $infile -translation binary

set outfilename [lindex $argv 1]
set outfile [open $outfilename w]
fconfigure $outfile -translation binary

set numFrames [lindex $argv 2]

puts "in: $infilename out: $outfilename frames: $numFrames"

for {set i 0} {$i < $numFrames} {incr i} {
  puts "Frame $i"
  set frameList [ReadDCPPacket $infile]
  if [eof $infile] {
    puts "End of file"
    break
  }
  puts -nonewline $outfile [join $frameList ""]  
}

close $infile
close $outfile
