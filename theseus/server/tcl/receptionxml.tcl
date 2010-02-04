proc WriteStatsTagXML {tagName tty varReceptionInfo} {
  upvar $varReceptionInfo receptionInfo 
  if [info exists receptionInfo($tagName)] {
    puts -nonewline $tty "<$tagName numFrames=\"[lindex $receptionInfo($tagName) 0]\">"
    for {set i 1} {$i<[llength $receptionInfo($tagName)]} {set i [expr $i + 2]} {
      puts -nonewline $tty "<statistic percentile=\"[lindex $receptionInfo($tagName) $i]\">"
      puts -nonewline $tty "[lindex $receptionInfo($tagName) [expr $i + 1]]</statistic>"
    }
    puts $tty "</$tagName>"
  }
}

proc WriteReceptionXML {tty rxList} {
	global rxIDCounters
	puts $tty "<receivers>"
        puts $tty "<date>[clock format [clock seconds] -format "%c" -gmt 1]</date>"

	foreach rxID [lsort $rxList] {
		upvar #0 receptionInfo$rxID receptionInfo
		puts -nonewline $tty "  <receiver name=\"$rxID\" "
		if [info exists receptionInfo(rgps)] {
			puts -nonewline $tty "latitude=\"[format %9.6f [lindex $receptionInfo(rgps) 0]]\" longitude=\"[format %9.6f [lindex $receptionInfo(rgps) 1]]\"" 
		}
		puts $tty ">"
                puts $tty "    <last_update>$rxIDCounters($rxID)</last_update>" 
                catch { puts $tty "    <mode>$receptionInfo(rdmo)</mode>" }
		catch { puts $tty "    <frequency>[expr $receptionInfo(rfre)/1000]</frequency>" }
		catch { puts $tty "    <mer>$receptionInfo(rmer)</mer>" }
                catch { puts $tty "    <wmf>$receptionInfo(rwmf)</wmf>" }
                catch { puts $tty "    <wmm>$receptionInfo(rwmm)</wmm>" }
		catch { puts $tty "    <signal>$receptionInfo(rdbv)</signal>" }
                catch { 
                  set audioList [split $receptionInfo(rafs) {}]
                  set audioPercentage [expr 100 * [llength [lsearch -all $audioList 0]] / [llength $audioList]]
                  puts $tty "    <audio>$audioPercentage</audio>"
                }
 
                if [info exists receptionInfo(label)] {
                  set label $receptionInfo(label)
                  regsub -all (<) $label { } label
                  regsub -all (>) $label { } label
		  regsub -all (&) $label {&amp;} label
                  puts $tty "    <label>$label</label>"
                } else {
                        puts $tty "    <label>No label</label>"
                }

		WriteStatsTagXML xwmf $tty receptionInfo
		WriteStatsTagXML xwmm $tty receptionInfo
		WriteStatsTagXML xmer $tty receptionInfo
		WriteStatsTagXML xdbv $tty receptionInfo

		set tagName rast
		if [info exists receptionInfo($tagName)] {
		    set totalFrames [lindex $receptionInfo($tagName) 0]
		    set correctFrames [lindex $receptionInfo($tagName) 1]

		    puts -nonewline $tty "<audio_stats numAudioFrames=\"$totalFrames\">"
		    
		    puts $tty "[expr 100 \* $correctFrames / $totalFrames]</audio_stats>"
		}

		if [info exists receptionInfo(rpsd)] {

                	set freq -7.875
                	set fstep 0.1875
			set endFreq [expr $freq + $fstep * ([string length $receptionInfo(rpsd)] - 1)]
			puts -nonewline $tty "    <psd start=\"$freq\" end=\"$endFreq\">"

	                for {set i 0} {$i < [string length $receptionInfo(rpsd)]} {set i [expr $i + 1]} {
       	                	binary scan [string range $receptionInfo(rpsd) $i $i] "c" byte
       	                	set byte [expr ($byte+0x100)%0x100]
                        	puts -nonewline $tty "[expr - ${byte} / 2]"
                                if {$i != [expr [string length $receptionInfo(rpsd)]-1]} {
					puts -nonewline $tty ","
				}
                        	set freq [expr $freq + $fstep]
                	}
			puts $tty "</psd>"
		}

		if [info exists receptionInfo(rpir)] {
 			if {[string length $receptionInfo(rpir)] > 4} {

                		binary scan [string range $receptionInfo(rpir) 0 1] "S" startTime
				set startTime [expr $startTime / 256.0]
                		binary scan [string range $receptionInfo(rpir) 2 3] "S" endTime
				set endTime [expr $endTime / 256.0]
				puts -nonewline $tty "    <pir start=\"$startTime\" end=\"$endTime\">"

	                	for {set i 4} {$i < [string length $receptionInfo(rpir)]} {set i [expr $i + 1]} {
       	                		binary scan [string range $receptionInfo(rpir) $i $i] "c" byte
       	                		set byte [expr ($byte+0x100)%0x100]
                        		puts -nonewline $tty "[expr - ${byte} / 2]"
                                	if {$i != [expr [string length $receptionInfo(rpir)]-1]} {
						puts -nonewline $tty ","
					}
                		}
				puts $tty "</pir>"
			}
		}

		if [info exists receptionInfo(hist_rdbv)] {
			set startTime [expr [llength $receptionInfo(hist_rdbv)] * 0.4]
			puts -nonewline $tty "<rdbv_history start=\"-$startTime\" end=\"0\">"
			puts -nonewline $tty [join $receptionInfo(hist_rdbv) ","]
			puts $tty "</rdbv_history>"
		}

		if [info exists receptionInfo(hist_rwmm)] {
			set startTime [expr [llength $receptionInfo(hist_rwmm)] * 0.4]
			puts -nonewline $tty "<rwmm_history start=\"-$startTime\" end=\"0\">"
			puts -nonewline $tty [join $receptionInfo(hist_rwmm) ","]
			puts $tty "</rwmm_history>"
		}

		puts $tty "  </receiver>"
	}

	puts $tty "</receivers>"
}
