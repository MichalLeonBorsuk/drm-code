
set config_dir [file dirname $argv0]
source [file join $config_dir "settings.tcl"]

package require rsciUtil
package require http ;# needed for download of current schedule

proc FindNextScheduleEntry {schedule} {

	global RX_NAME

	# Finds next schedule entry and returns it.
	# If none is found, an empty list is returned.
	
	# Init output variables
	set nextEntry [list]
	
	# Current time
	set nowTcl [clock seconds]
	
	# Go through schedule
	foreach entry $schedule {
	
		set relStartTcl [lindex $entry 0]
		set relStopTcl [lindex $entry 1]
		set dowList [lindex $entry 3]
	
		# If midnight is included in time span: add 24h to relStopTcl
		if {$relStopTcl <= $relStartTcl} {
		    set relStopTcl [expr $relStopTcl + 24*60*60]
		}

		if {[info tclversion] == "8.3"} {
		    set todayDate [clock format $nowTcl -format "%Y-%m-%d" -gmt 1]
		} else {
		    set todayDate [clock format $nowTcl -format "%m/%d/%Y" -gmt 1]
		}

		set todayMidnight [clock scan "$todayDate 00:00:00" -gmt 1]  
		set startTcl [expr $todayMidnight + $relStartTcl]
		set stopTcl [expr $todayMidnight + $relStopTcl]
		
		# Increase times in steps of 1 day until we match dowList AND 
		# startTcl is in the future OR
		# startTcl is past but stopTcl in in the future
		
		while {1} {
	
			if {$startTcl >= $nowTcl || ($startTcl<$nowTcl && $stopTcl>$nowTcl)} {

				set dow [clock format $startTcl -format "%w" -gmt 1]
				if {[lsearch $dowList $dow] != -1} {
					break
				}
			}

			incr startTcl 86400
			incr stopTcl 86400
		}


		# The winner is the entry that has the next (closest in time to 'now') start time

		# Init variable chosenStartTime the first time round (+1 makes the next if clause possible)
		if ![info exists chosenStartTime] {
			set chosenStartTime [expr $startTcl + 1]
		}

		if {$startTcl < $chosenStartTime} {
			set nextEntry [lreplace $entry 0 1 $startTcl $stopTcl]
			set chosenStartTime $startTcl
		}
		
	}
	syslog "debug" "chosenStartTime $chosenStartTime nowTcl $nowTcl"
	set previousStopTcl [lindex $nextEntry 1]

	set gap "gap"

	# andrewm
	# Go through schedule again looking for an entry that has the same start time as our stop time(!)
	foreach entry $schedule {
	
		set relStartTcl [lindex $entry 0]
		set relStopTcl [lindex $entry 1]
		set dowList [lindex $entry 3]
	
		# If midnight is included in time span: add 24h to relStopTcl
		if {$relStopTcl <= $relStartTcl} {
		    set relStopTcl [expr $relStopTcl + 24*60*60]
		}

		if {[info tclversion] == "8.3"} {
		    set todayDate [clock format $nowTcl -format "%Y-%m-%d" -gmt 1]
		} else {
		    set todayDate [clock format $nowTcl -format "%m/%d/%Y" -gmt 1]
		}

		set todayMidnight [clock scan "$todayDate 00:00:00" -gmt 1]  
		set startTcl [expr $todayMidnight + $relStartTcl]
		set stopTcl [expr $todayMidnight + $relStopTcl]
		
		# Increase times in steps of 1 day until we match dowList AND 
		# startTcl is in the future OR
		# startTcl is past but stopTcl in in the future

		set a 0;
		while {[expr $a < 7]} {
	
			if {$startTcl == $previousStopTcl} {	# possible match, so check the dow
#				syslog "info" "Possible match: $startTcl  $previousStopTcl"
				set dow [clock format $startTcl -format "%w" -gmt 1]
				if {[lsearch $dowList $dow] != -1} {
					break
				}
			}

			incr startTcl 86400
			incr stopTcl 86400

			set a [expr $a+1]
		}

		if {$startTcl == $previousStopTcl} {
			set gap "gapless"
		}		
	}

	lappend nextEntry $gap

#	puts "    next: $nextEntry"

	return $nextEntry
}


proc WaitForNextEvent {} {

	global DATA_PROCESSING_TIME TIME_SERVER
	global SCHEDULE_FILENAME

	if ![info exists SCHEDULE_FILENAME]	{
		set SCHEDULE_FILENAME schedule.txt
	}
	# Finds next schedule entry and returns it.
	# If there is no entry, the scripts waits until there is an entry
	# This they wait for a schedule update
	# If there is enough time,
	#	- the data is processed
	#	- data is sent off
	#	- time is synchronised (TBD)

	# Init output variables
	set remainingTime [expr 2*$DATA_PROCESSING_TIME]

	# Check whether there is enough time generate and send a data file and synchronised
	while {$remainingTime >= [expr 2*$DATA_PROCESSING_TIME]} {

	 # puts "Point 1:\t$remainingTime sec remaining."

		# Read schedule
		set schedule [ReadSchedule $SCHEDULE_FILENAME]

	# Retry after timeout, if no schedule entry is available
		while {$schedule == ""} {
			syslog "info" "No schedule entry available! Waiting for retry...."

			set RETRY_TIMEOUT 20

			# Wait <RETRY_TIMEOUT> seconds
			Wait [expr [clock seconds] + $RETRY_TIMEOUT]

			# Download new schedule
			GetLatestSchedule
			set schedule [ReadSchedule schedule.txt]
		}
		

		# Find next entry in schedule
		set nextEntry [FindNextScheduleEntry $schedule]

		# Start time
		set startTcl [lindex $nextEntry 0]

		# If there was an entry, calculate remaining seconds = startTcl - now
		# Otherwise we stay in the loop (the cdx is still true)
		if {$startTcl != ""} {
			set remainingTime [expr $startTcl - [clock seconds]]
		} 

		 #puts "Point 2:\t$remainingTime sec remaining."

		# If there is still enough time, do housekeeping
		if {$remainingTime >= [expr 2*$DATA_PROCESSING_TIME]} {

			# Determine whether the next event is on the next day.
			set currentDay [clock format [clock seconds] -format "%j" -gmt 1]
			set nextEventDay [clock format $startTcl -format "%j" -gmt 1]
			
			if {$currentDay != $nextEventDay} {
				set processImmediately 1
			} else {
				set processImmediately 0
			}

			# Wait <DATA_PROCESSING_TIME> seconds
			Wait [expr [clock seconds] + $DATA_PROCESSING_TIME]

			# Process data and try to send it off if possible
			ProcessData $processImmediately

			# Synchronise time with nt server
			if [info exists TIME_SERVER] {
			    SynchoniseWithTimeServer $TIME_SERVER
			}

			# Download new schedule
			GetLatestSchedule

			# If there was an entry, calculate remaining seconds = startTcl - now
			if {$startTcl != ""} {
				set remainingTime [expr $startTcl - [clock seconds]]
			}
		}

		 #puts "Point 3:\t$remainingTime sec remaining."
	}

	return $nextEntry
}

proc DisplayTime {tclTime} {

	return [clock format $tclTime -format "%a, %b %d %H:%M:%S" -gmt 1]
}

proc sendToCom {string} {
	global comId
	puts $comId $string
	if [catch {flush $comId} result] {
		syslog "warn" $result
	}

}
proc DoBandScan {entry} {

	global comId BANDSCAN_STEP_FREQ BANDSCAN_STEP_TIME
      set timeStamp [clock seconds]

	set freqStep $BANDSCAN_STEP_FREQ
	set collectionTime $BANDSCAN_STEP_TIME

	set frequency [lindex $entry 2]
      set recProfile [lindex $entry 4]
      set mode [lindex $entry 5]

	set freqlist [split $frequency ","]
	set startFreq [expr [lindex $freqlist 0] / 1000]
	set stopFreq [expr [lindex $freqlist 1] / 1000]
	
	syslog "info" "Band scan from $startFreq to $stopFreq in $freqStep"

	# Memorise start time
	set targetTime $timeStamp

	# initialise bandscan collection
	sendToCom "BandScanInit [lindex $entry 0] $startFreq $stopFreq $freqStep" 

	for {set freq $startFreq} {$freq <= $stopFreq} {set freq [expr $freq + $freqStep]} {
		set targetTime [expr $targetTime + $collectionTime]
		
		sendToCom "start $mode $freq $recProfile"

		while {[clock seconds] <= $targetTime} {

			set flag 0
			after 1000 {set flag 1}
			vwait flag
		}    
	}

	sendToCom "BandScanEnd"

}

proc StartDeRecording {entry} {

	# Starts the recording of data entities in schedule entry
	
	global comId THOMCAST_CTRL
	set frequency [lindex $entry 2]
	set deList [lindex $entry 3]
      set recProfile [lindex $entry 4]
      set mode [lindex $entry 5]

	if {$mode=="BS"} { # comma-separated list of start and stop
		DoBandScan $entry
	} else {
		set freqkHz [expr $frequency/1000.0]

		# Set frequency of Thomcast receiver if this option was selected
		if [info exists THOMCAST_CTRL] {
			SetFreqThomcast $frequency
		}

		# Start AGC recording
		sendToCom "start $mode $freqkHz $recProfile"
	}
}

proc StopDeRecording {entry} {

	# Stops the recording of data entities in schedule entry
	
	global comId

	set frequency [lindex $entry 2]
	set deList [lindex $entry 3]

	# Stop AGC recording
	puts $comId "stop"
	flush $comId
}

proc ProcessData {processImmediately} {

	global ROOT_DIR TCLSH_CMD ENABLE_EMAIL
	global INTERNET_CONNECTION_START INTERNET_CONNECTION_END	

	# Checks existing data files and creates remaining data files. Sends them off.


	# If we don't have an internet connection, return without further processing
	if {![GMTIsInTimeInterval $INTERNET_CONNECTION_START $INTERNET_CONNECTION_END]} { 
		return;
	}


	if {[info tclversion] == "8.3"} {
	    set dateFormat "%Y-%m-%d"
	} else {
	    set dateFormat "%m/%d/%Y"
	}

	# Time of last midnight. Everything recorded before that should be processed.
	set timeLimitTcl [clock scan "[clock format [clock seconds] -format $dateFormat -gmt 1] 00:00:00"]
	# puts [clock format $timeLimitTcl]

	# Get all existing dates
	set datDirList [glob -nocomplain [file join $ROOT_DIR datafiles ????-??-??]]
	
	# Go through them
	foreach dateDir $datDirList {

		# Extract dateString
		set dateString [lindex [split $dateDir /] end]


		# Adapt to tcl version
		if {[info tclversion] != "8.3"} {

		    # Extract y,m,d
		    regexp {(.+)-(.+)-(.+)} $dateString match year month day
		    set dateString "$month/$day/$year"
		}

		# Midnight time on that date
		set midnightTcl [clock scan "$dateString 00:00:00" -gmt 1]

		# Skip if it is beyond our limit and we don't want to process immediately
		if {$midnightTcl >= $timeLimitTcl && !$processImmediately} {
			continue
		}
		
		# Here, we know that a data file exists

		### Send away if it was not sent yet
		if {$ENABLE_EMAIL} {
			SendData $dateString
		}
	}
	
}

proc SendData {dateString} {
    
    global RX_NAME
    global ROOT_DIR
    
    # Create filename flagging that an email as been sent
    set tclTime [clock scan "$dateString 00:00:00" -gmt 1]
    set fileName [GenerateFileName $tclTime "dt2"]
    set sndFileName "[file root $fileName].snd"
    set gzFileName "$fileName.gz"

    syslog "debug" "schedule: $sndFileName"

    # Has an email alread been sent?
    if [file exists $sndFileName] {
	# syslog "info" "Datafile for $dateString has already been sent!"
    } else {
	
      ### Establish connection with internet
      # At the moment, we assume that we are permanently connected

      set cmdName [file join $ROOT_DIR gzip]
    
      if {[info exists GZIP_DT2_FILES]} {
	if {$GZIP_DT2_FILES} {
	        set errorFlag [catch {exec $cmdName --force $fileName} response ]
	}
      }

      # Return response
      if [file exists $gzFileName] {
  	  # Send email using email client
 	  # try gzipped
        set attachmentName $gzFileName;
      } else {
	  # Send email using email client
        # try not gzipped
	  set attachmentName $fileName 
      }

      set response [EmailDataFile $attachmentName]


	# Create .snd file to show that the email was successfully sent. Contains the current time.
	if {$response == ""} {
	    set fid [open $sndFileName w]
	    puts $fid [clock format [clock seconds] -gmt 1]
	    close $fid
	    syslog "info" "Data file $attachmentName (for $dateString) sent"
	} else {
	    syslog "info" "Failed to send data file $attachmentName: $response"
	}

	### Terminate connection to the internet
	# At the moment, we assume that we are permanently connected
	
    }
}

proc GetLatestSchedule {} {

	# Read lastest schedule from web

	global SCHEDULE_URL
	global HTTP_PROXY_HOST
	global HTTP_PROXY_PORT
	global RX_NAME
	global AUTOMATIC_SCHEDULE_DOWNLOAD
	global INTERNET_CONNECTION_START INTERNET_CONNECTION_END
	set    HTTP_TIMEOUT 10000

	# Return if the schedule is not to be downloaded automatically
	if {!$AUTOMATIC_SCHEDULE_DOWNLOAD} {
		return
	}

	# If we don't have an internet connection, return without further processing
	if {![GMTIsInTimeInterval $INTERNET_CONNECTION_START $INTERNET_CONNECTION_END]} { 
		return;
	}

	# Current time
	set timeString [clock format [clock seconds] -format "%H:%M:%S"]

	# Configure proxy
	http::config -proxyhost $HTTP_PROXY_HOST -proxyport $HTTP_PROXY_PORT

	# First, try a receiver-specific version of the schedule file
	set url "$SCHEDULE_URL/$RX_NAME\_schedule.txt"

	# Open file for new schedule
	set outId [open "new_schedule.txt" w]

	# Send request.
	catch {set token [http::geturl $url -channel $outId -timeout $HTTP_TIMEOUT -headers {Pragma no-cache}]}

	# Close file
	close $outId

	# If the geturl failed due to a bad network connection, return
	if ![info exists token] {
		syslog "warn" "Attempt to download receiver-spedific schedule aborted at $timeString due to network problems."
		return
	}

        if { [::http::status $token] == "ok"} {
		# puts "File retrieved succesfully\r"
		} else {
		syslog "warn" "Timeout or network error occurred"
		return
	      }


	# If the receiver-specifc schedule is not found, download the general one
	if {![regexp {OK} [http::code $token]]} {

		# New request
		set url "$SCHEDULE_URL/schedule.txt"

		# Open file for new schedule
		set outId [open "new_schedule.txt" w]

		# Send request.
		catch {set token [http::geturl $url -channel $outId -timeout $HTTP_TIMEOUT -headers {Pragma no-cache}]}

		close $outId

		if ![info exists token] {
			syslog "warn" "Attempt to download general-purpose schedule aborted at $timeString due to network problems."
			return
		}

	        if { [::http::status $token] == "ok"} {
       	        # puts "File retrieved succesfully\r"
       		  } else {
	              syslog "warn" "schedule" Timeout or network error occurred!\r"
        	        return
		        }

		 syslog "info" "Latest general-purpose schedule downloaded"
	} else {
		syslog "info" "Latest receiver-specific schedule downloaded for receiver $RX_NAME"
	}	

	# Rename new file to schedule.txt if successful
	if {[regexp {OK} [http::code $token]] == 1} {
		file rename -force "new_schedule.txt" "schedule.txt"
	} else {
		syslog "warn" "Error! Could not retrieve schedule"                      "
	}

	# Cleanup won't work with versions of tcl http client package <= 2.0  
	#http::cleanup $token
}


proc GMTIsInTimeInterval {startTimeString endTimeString} {

	# This function checks whether the current GMT time is between "startTimeString"
	# and "stopTimeString".
	# It returns '1', if this is the case and '0' otherwise

	set nowTcl [clock seconds]
	
	if {[info tclversion] == "8.3"} {
	    set todayDate [clock format $nowTcl -format "%Y-%m-%d" -gmt 1]
	} else {
	    set todayDate [clock format $nowTcl -format "%m/%d/%Y" -gmt 1]
	}

	set startTime [clock scan "$todayDate $startTimeString" -gmt 1]  
	set endTime [clock scan "$todayDate $endTimeString" -gmt 1]  

	if {$startTime <= $nowTcl && $nowTcl <= $endTime} {
		return 1
	} else {
		return 0
	}
}


	
# Shall all the remaining files be processed immediately?
if [info exists PROCESS_ON_STARTUP] { ProcessData 1 }


# Prepare recording
set comId  [socket localhost $TCP_PORT_RECORDER]

puts $comId "reset"
flush $comId

while {1} {

	# Find next entry in schedule
	set entry [WaitForNextEvent]
	
	# If not entry is given, quit
	if {$entry == ""} {
		syslog "info" "No more entries in schedule"
		break
	}

	set startTcl [lindex $entry 0]
	set stopTcl [lindex $entry 1]
	set frequency [lindex $entry 2]
	set deList [lindex $entry 3]
	set recProfile [lindex $entry 4]
	set mode [lindex $entry 5]
	set gap [lindex $entry 6]

	syslog "info" "Next event:\t[DisplayTime $startTcl] to [DisplayTime $stopTcl]\t$frequency Hz $mode\t$deList\t$recProfile\t$gap"
	
	# Wait until start
	Wait $startTcl
	syslog "info" "Start:\t\t[DisplayTime [clock seconds]]"
	StartDeRecording $entry
	
	Wait $stopTcl
	if { [lsearch $entry "gap"] != -1 } {		# if gap found, then send stop
		syslog "info" "Stop:\t\t[DisplayTime [clock seconds]]"
		StopDeRecording $entry
	} else {
		syslog "info" "Gapless:\t[DisplayTime [clock seconds]]"
	}
}

close $afsCom
close $agcCom
