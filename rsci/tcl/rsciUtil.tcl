
package provide rsciUtil 1.0

package require syslog

# Common code
proc GenerateFileName {time type} {
	
	# Generates file name based on time and type.
	
	global ROOT_DIR RX_NAME

	# Filename
	set dateString [clock format $time -format "%Y-%m-%d" -gmt 1]
	set timeString [clock format $time -format "%H-%M-%S" -gmt 1]
	set fileName [file join $ROOT_DIR datafiles $dateString $RX_NAME\_$dateString.$type]

	return $fileName
}

proc GenerateFileNameBin {time type} {
	
	# Generates file name based on time and type.
	
	global ROOT_DIR RX_NAME

	# Filename
	set dateString [clock format $time -format "%Y-%m-%d" -gmt 1]
	set timeString [clock format $time -format "%H-%M-%S" -gmt 1]
	set fileName [file join $ROOT_DIR bin $dateString $RX_NAME\_$dateString.$type]

	return $fileName
}

proc GenerateFileNameBS {time type} {
	
	# Generates file name based on time and type.
	
	global ROOT_DIR RX_NAME

	# Filename
	set dateString [clock format $time -format "%Y-%m-%d" -gmt 1]
	set timeString [clock format $time -format "%H-%M-%S" -gmt 1]
	set fileName [file join $ROOT_DIR bandscans $dateString $RX_NAME\_${dateString}_${timeString}.$type]

	return $fileName
}

proc CmdArg2FileName {argv} {

	# Converts command line argument into filename


	
	# Read date and time
	set dateString [lindex $argv 0]
	set timeString [lindex $argv 1]
	set type [lindex $argv 2]

	if {$dateString == "" || $timeString == "" || $type == ""} {
	
		syslog "warn" "Error! Three arguments required: <YYYY-MM-DD> <HH-MM-SS> <type>"
		exit
	}

	# Convert to tclTime
	regexp {(..)-(..)-(..)} $timeString match hh mm ss
	regexp {(..)-(..)-(..)} $dateString match Y M D ; # added for tcl8.0
	#set tclTime [clock scan "$dateString $hh:$mm:$ss" -gmt 1]
	set tclTime [clock scan "$M/$D/$Y $hh:$mm:$ss" -gmt 1]; # Modified for tcl8.0
	
	# Generate file name
	set fileName [GenerateFileName $tclTime $type]

	return $fileName
}

proc ReadAFS {fileName} {
	
	# Read audio frame status from file <fileName> and return as list with
	# { {frequency in kHz} {afsString} }

	# Open file
	set fid [open $fileName]

	# Set to binary
	#fconfigure $fid -encoding binary
	fconfigure $fid -translation binary

	# Read first two bytes containing the frequency
	binary scan [read $fid 2] "S" frequency


	# Read byte-by-byte
	set outAFS ""
	while {1} {

		# Read 1 byte
		set byte [read $fid 1]
	
		if [eof $fid] {
			break
		}

	
		# Convert to string
		binary scan $byte "B8" byteString
		
		# Convert to AFS
		for {set i 0} {$i<8} {incr i 2} {
	
			set twoBit [string range $byteString $i [expr $i+1]]
			
			case $twoBit {
				00 {append outAFS 0}
				01 {append outAFS 1}
				10 {append outAFS 2}
				11 {append outAFS 3}
			}
		}
	}
	
	close $fid
	
	return [list $frequency $outAFS]
}

proc WriteBandscanFile {bsData} {
	# format of list is startTime, startFreq, stopFreq, freqStep, then median rdbv for each freq
	set startTime [lindex $bsData 0]
	set startFreq [lindex $bsData 1]
      set stopFreq [lindex $bsData 2]
      set freqStep [lindex $bsData 3]

	set fileName [GenerateFileNameBS $startTime "bs"]

	# Generate directory
	file mkdir [file dirname $fileName]

	# Open file for appended writing
	set fid [open $fileName w]

	set i 4
	for {set freq $startFreq} {$freq<=$stopFreq} {set freq [expr $freq + $freqStep]} {
		puts $fid "$freq\t[lindex $bsData $i]"
		incr i
	}

	close $fid
}

proc WriteDataBlock {dataBlock type} {

	# Writes content of <dataBlock> to file defined by <type>.
	# The data entity list <dataBlock> contains the following pairs:
	# {<ID in ASCII> <length in bytes of each data element> <dataList in ASCII>}
	# <dataList> can consisist of one or several entries

	global RX_NAME

	# Init binary output string
	set outStringBin ""
	
	# Go trough each data entity ...
	foreach dataEntity $dataBlock {

		# Extract fields
		set ID [lindex $dataEntity 0]
		set itemSize [lindex $dataEntity 1]
		set dataList [lindex $dataEntity 2]

		# Remember time stamp
		if {$ID == 0x00} {
			set timeStamp $dataList
		}
		
		# Number of elements
		set noOfEntries [llength $dataList]

		# 8 bit Binary ID
		set IDBin [binary format "c" $ID]

		# 16 bit Number of entries
		set noOfEntriesBin [binary format "S" $noOfEntries]

		# 8 bit Binary item size
		set itemSizeBin [binary format "c" $itemSize]

		# Go through data list and create binary data
		set dataBin ""
		foreach dataEntry $dataList {
	
			if {$itemSize == 1} {			
				append dataBin [binary format "c" $dataEntry]
				
			} elseif {$itemSize == 2} {
				append dataBin [binary format "S" $dataEntry]
				
			} elseif {$itemSize == 4} {
				append dataBin [binary format "I" $dataEntry]
				
			} else {
				syslog "warn" "Error in WriteDataBlock! Field size $itemSize is not yet defined!"
				exit
			}
		}
	
		# Assemble into binary output string
		append outStringBin $IDBin $noOfEntriesBin $itemSizeBin $dataBin
	}
	
	# Write to file 
	
	# Generate file name
	if [info exists timeStamp] {
	    set fileName [GenerateFileName $timeStamp $type]
	} else {
	    syslog "warn" "Error in WriteDataBlock! Data block does not contain time stamp"
	    exit
	}


	# Generate directory
	file mkdir [file dirname $fileName]

	# Open file for appended writing
	set fid [open $fileName a]

	# Set to binary encoding
#	fconfigure $fid -encoding binary
	fconfigure $fid -translation binary
	
	# Write to file
	puts -nonewline $fid $outStringBin

	# Close file
	close $fid
}

proc DisplayBinaryString {binString} {

	# Displays binary string

	for {set i 0} {$i<[string length $binString]} {incr i} {

		set byteBin [string range $binString $i $i]
		
		# Convert
		binary scan $byteBin "H2" byte
		
		# Display
		syslog "warn" -nonewline "$byte  "
	}
	syslog "warn" ""
}

proc GetDEInfo {ID} {

	# Returns a list with two field giving information on the given ID 
	# <length in bytes of each data entry> <number of entries>
	# Returns -1 at failure

	# Data entity info: Pairs are {<ID> <length in bytes of each data entry> <number of entries>}
	# <number of entries> = -1 means that a 16-bit length field follows
	set DE_INFO [list {0 4 1}\
		{1  4 1}\
		{5  2 1}\
		{6  1 9}\
		{7  1 9}\
		{8  1 9}\
		{9  1 9}\
		{16 2 1}\
		{17 2 1}\
		{18 2 1}\
		{19 2 1}\
		{32 2 1}\
		{33 2 1}\
		{34 2 1}\
		{35 2 1}\
		{48 2 1}\
		{51 1 5}\
		{56 2 1}]

	# Find position in list ([expr] changes hex to dec)
	set pos [lsearch -regexp $DE_INFO "[expr $ID] .+ .+"]
	
	if {$pos != -1} {
		return [lrange [lindex $DE_INFO $pos] 1 2]
	} else {
		return -1
	}
}

proc ExtractBinaryData {binaryStringVar length} {

	# Extract <length> bytes from binaryString and return them as ASCII number.
	# binaryString will contain the remainder

	upvar $binaryStringVar binaryString 

	set extract [string range $binaryString 0 [expr $length-1]]
	set binaryString [string range $binaryString $length end]

	# Reaction depends on segment length
	if {$length == 1} {
		
		# Complement to 16 bit number by adding trailing 0x00
		set extract "[binary format c 0]$extract"
		
		# Convert to ASCII
		binary scan $extract "S" extractASCII
	}

	if {$length == 2} {
		binary scan $extract "S" extractASCII
	}

	if {$length == 3} {

		# Complement to 32 bit number by adding trailing 0x00
		set extract "[binary format c 0]$extract"
		
		# Convert to ASCII
		binary scan $extract "I" extractASCII
	}

	if {$length == 4} {
		binary scan $extract "I" extractASCII
	}

	if {$length > 4} {
		syslog "warn" "$error! Length > 4 bytes not yet defined!"
		exit
	}
	
	return $extractASCII
}


proc ReadDataBlock {binaryContentVar} {

	# Returns content of the first data block contained in binaryContent in a list
	# {ID ASCIIcontent} {ID ASCIIcontent} ...
	# The order is maintained.
	# When the procedure returns, binaryContent contains the remaining binary content

	upvar $binaryContentVar binaryContent

	# Init flag indicating that the timestamp was found
	set foundID0 0
	
	# Init result
	set deList [list]
	
	while {[string length $binaryContent] > 0} {

		#DisplayBinaryString $binaryContent\n
		
		# Read ID
		set ID [ExtractBinaryData binaryContent 1]
		
		#syslog "warn" $ID\n\n

		# If it the time stamp...
		if {$ID == 0x00} {
		
			# ... and if it is the first time stamp
			if {$foundID0 == 0} {
				# Remember
				set foundID0 1
			} else {
				# The time stamp was already found: A new block begins here
				
				# Add ID 0 back at the beginning
				set binaryContent "[binary format c 0]$binaryContent"
				
				# Break the loop 
				break
			}
		}
			
		# Read DE length
		# set deLength [lindex [GetDEInfo $ID] 0]
		set noOfEntries [ExtractBinaryData binaryContent 2]
		set noOFEntries [expr ($noOfEntries+0x10000)%0x10000]

		# Read size of individual entry
		set itemSize [ExtractBinaryData binaryContent 1]
		set itemSize [expr ($itemSize+0x100)%0x100]

		syslog "warn" "$ID\t$noOfEntries\t$itemSize"


		# Read each data entry and store in list deContent
		set deContentList [list]

		for {set i 0} {$i<$noOfEntries} {incr i} {

			lappend deContentList [ExtractBinaryData binaryContent $itemSize]
		}
		
		# Store in list
		lappend deList [list $ID $deContentList]
	}

	# Return result
	return $deList
}

proc ReadDataEntity {dataBlock ID} {

	# Extracts all data entities with identity <ID> from datablock and returns them.

	# Find all positions in the datablock
	set db $dataBlock
	while {[set pos [lsearch -regexp $db "^$ID ."]] !=-1} {

		# Add new position to posList
		lappend posList $pos
		
		# Replace element by "rep" to keep size of data block constant
		set db [lreplace $db $pos $pos "rep"]
	}
	

	# Init result list
	set outList [list]
	
	# Extract data of data entities with positions given by posList
	foreach pos $posList {
		lappend outList [lindex [lindex $dataBlock $pos] 1]
	}

	return $outList
}


proc ReadAGC {fileName} {
	
	# Read signal strength AGC values from file <fileName> and return them in list

	# Open file
	set fid [open $fileName]

	# Set to binary
	fconfigure $fid -encoding binary
	fconfigure $fid -translation binary

	# Read byte-by-byte
	set agcList [list]
	while {1} {

		# Read 1 byte
		set byte [read $fid 1]

		if [eof $fid] {
			break
		}
	
		# Convert to dec value
		binary scan [binary format c 0]$byte "S" agcValue

		# Append to agcList
		lappend agcList $agcValue
	}
	
	close $fid
	
	return $agcList
}

proc ConvertAgc {agcList} {
	
	# Convert all agc values in agcList to dbuV. 
	# Global constants are taken from receiver-specific tcl file
	
	global configCorrectionFactorDigital 
	global configAgcCoefs

	set coefs $configAgcCoefs

	foreach agc $agcList {

		# Init vars
		set result 0.0
		set x 1.0
		set order [llength $coefs]
	
		# Calculate polynomial expression
		for {set i 1} {$i<=$order} {incr i} {
			set result [expr $result + ($x*[lindex $coefs [expr $order-$i]])]
			set x [expr $x*$agc]
		}
	
		# Apply digital correction factor (for digital modes) and convert to dBm to dBuV (+107 dB)
		set result [expr round($result + $configCorrectionFactorDigital + 107)]
	
		lappend resultList $result
	}

	return $resultList

}
	
proc incr {valueVar {delta 1}} {

	# Replaces tcl function incr. Adds initialisation
	upvar $valueVar value

	if ![info exists value] {
		set value $delta
	} else {
		set value [expr $value + $delta]
	}
}

proc CalcDistribution {valueList} {

	# Calculate distribution of values in valueList
	# Values are returned as list:
	# {min dist90 dist50 dist10 max} 
	
	# Fill distribution array
	foreach value $valueList {
		incr agcDist($value)
	}

	# Determine Min and Max
	set indexList [lsort -integer -decreasing [array names agcDist]]
	set min [lindex $indexList end]
	set max [lindex $indexList 0]

	# Calculate Sum
	set sum 0
	foreach p $indexList {
		incr sum $agcDist($p)
	}
		
	# Init varables
	set shareSum 0
	set dist90 $min
	set dist50 $min
	set dist10 $max
	
	# Go through all possible power levels
	foreach p $indexList {

		# Calculate share
		set share [expr 100*$agcDist($p)/double($sum)]
		set shareSum [expr $shareSum + $share]
			
		# syslog "warn" $p\t$share\t$shareSum

		# dist10
		if {$shareSum <= 10.0} {
			set dist10 $p
		}
				
		# median
		if {$shareSum <= 50.0} {
			set dist50 $p
		}

		# dist90
		if {$shareSum <= 90.0} {
			set dist90 $p
		}

		
	}

	return [list $min $dist90 $dist50 $dist10 $max]
}

proc ReadBinaryFile {fileName} {

	# Open file
	set fid [open $fileName r]

	# Set to binary encoding
	#fconfigure $fid -encoding binary
	#fconfigure $fid -eofchar {}
	fconfigure $fid -translation binary

	# Read content
	set binaryContent [read $fid]

	# Close file
	close $fid

	return $binaryContent
}

proc SendSqlQuery {db query} {

	# Send SQL query and return rows as list

	# Send query
	sql query $db $query

	# Init result
	set outList [list]
	
	# Retrieve results
	while {[set row [sql fetchrow $db]] != ""} {
		lappend outList $row
	}
	
	sql endquery $db

	return $outList
}

proc ReadScheduleIBB {fileName} {

	syslog "info" "reading schedule $fileName"
	# Read schedule stored in ASCII file <fileName>
	# and returns list with its content
	# Format {entry1} {entry 2} ...
	# with entry: {type txLoc rxLox rxName dowList startTcl stopTcl frequencyHz deList}
	# dowList: List of the days of the week (0 = Sunday)

	global RX_NAME

	# Init result schedule
	set scheduleList [list]

	# Read content of file
	if [catch {set fid [open $fileName r]} result] {
		return
	}

	set content [read $fid]
	close $fid

	# Go though each line
	foreach line [split $content \n] {
		
		# Trim line
		set line [string trim $line]

		# Convert to lower case
		set line [string tolower $line]

		syslog "debug" $line
		
		# Ignore comments
		if [regexp {^\*} $line] {
			continue
		}

		# Ignore empty lines
		if [regexp "^\[ \t]*$" $line] {
			syslog "debug" "skipping empty line"
			continue
		}

		if [regexp "^VOA" $line] {
			syslog "debug" "skipping VOA line"
			continue
		}

		# Split line into fields
		set fieldList [split $line "\t"]

		# Trim fields
		for {set index 0} {$index<[llength $fieldList]} {incr index} {
			lreplace $fieldList $index $index [string trim [lindex $fieldList $index]]
		}

		# Ignore all except normal schedule entries.
		set entryType [lindex $fieldList 2]
		if {$entryType != "s" && $entryType != "b"} {
			syslog "debug" "skipping non-service entry"
			continue
		}

		set mode [lindex $fieldList 6]
		
		# Start
		set startTcl [clock scan "01/01/1970 [lindex $fieldList 1]" -gmt 1]
		syslog "debug" [clock format $startTcl -gmt 1]		


		# Days of the week
		set doW [lindex $fieldList 0]

		set recProfile "a"
		if {[regexp {^[a-z]$} $recProfile] == -1} {
			syslog "warn" "Invalid profile: $recProfile"
		}

		set mode [string toupper $mode]

		# Convert dow to list
		set dowList [list]
		for {set i 0} {$i<[string length $doW]} {incr i} {
			if {[string range $doW $i $i] != "."} {
				lappend dowList $i
			}
		}

		set dowList [lsort -integer $dowList]
		#syslog "warn" [join $dowList ""]

		if {$entryType == "s"} {
			# Stop - add on the length to the start time
			set stopTcl [expr $startTcl + [lindex $fieldList 8]]
			if {$stopTcl >= 86400} {
				# DRM schedule format expects it to wrap back to 00:00
				set stopTcl [expr $stopTcl - 86400]
			}

			#syslog "warn" [clock format $stopTcl -gmt 1]

			# Frequency in Hz
			set frequency [expr [lindex $fieldList 5] \* 1000]


		} else { # bandscan
			# Stop - add on the length to the start time TODO how long?
			set stopTcl [expr $startTcl + 30] 
			if {$stopTcl >= 86400} {
				# DRM schedule format expects it to wrap back to 00:00
				set stopTcl [expr $stopTcl - 86400]
			}

			#syslog "warn" [clock format $stopTcl -gmt 1]
			set mode "BS"
			set recProfile ""

			# Frequency in Hz
			set startFrequency [expr [lindex $fieldList 4] \* 1000]
			set stopFrequency [expr [lindex $fieldList 5] \* 1000]
			set frequency "$startFrequency,$stopFrequency"
		}

		set entry [list $startTcl $stopTcl $frequency $dowList $recProfile $mode]

		# Add entry to schedule
		lappend scheduleList $entry
		
	}

	return $scheduleList
}



proc ReadSchedule {fileName} {

	global SCHEDULE_FORMAT
	if [info exists SCHEDULE_FORMAT] {
		if {$SCHEDULE_FORMAT == "IBB"} {
			return [ReadScheduleIBB $fileName]
		}
	}

	set fileName
	# Read schedule stored in ASCII file <fileName>
	# and returns list with its content
	# Format {entry1} {entry 2} ...
	# with entry: {type txLoc rxLox rxName dowList startTcl stopTcl frequencyHz deList}
	# dowList: List of the days of the week (0 = Sunday)

	global RX_NAME

	# Init result schedule
	set scheduleList [list]

	# Read content of file
	if [catch {set fid [open $fileName r]} result] {
		return
	}

	set content [read $fid]
	close $fid

	# Go though each line
	foreach line [split $content \n] {
		
		# Trim line
		set line [string trim $line]

		# Convert to lower case
		set line [string tolower $line]

		# syslog "warn" $line
		
		# Ignore comments
		if [regexp {^#} $line] {
			continue
		}

		# Ignore empty lines
		if [regexp "^\[ \t]*$" $line] {
			continue
		}

		# Split line into fields
		set fieldList [split $line "\t "]

		# Trim fields
		for {set index 0} {$index<[llength $fieldList]} {incr index} {
			lreplace $fieldList $index $index [string trim [lindex $fieldList $index]]
		}

		set mode "drm"
		
		# Start
		set startTcl [clock scan "01/01/1970 [lindex $fieldList 0]" -gmt 1]
		#syslog "warn" [clock format $startTcl]

		# Stop
		set stopTcl [clock scan "01/01/1970 [lindex $fieldList 1]" -gmt 1]
		#syslog "warn" [clock format $stopTcl]

		# Frequency in Hz
		set frequency [expr [lindex $fieldList 2] * 1000]

		set field [lindex $fieldList 3]

		if {[regexp {[a-z]+} $field]}	{	# it's a mode not DoW 
			set mode [lindex $fieldList 3];

			if {($mode != "am") && ($mode != "drm")} {
				syslog "warn" "Invalid mode, $mode, in schedule. Default to drm"
				set mode "drm"
			}

			# Days of the week
			set doW [lindex $fieldList 4]
			if {[regexp {^[0123456]$} $doW] == -1} {
				syslog "warn" "Invalid day of the week in schedule: $doW"
			}

			set recProfile [lindex $fieldList 5]
			if {[regexp {^[a-z]$} $recProfile] == -1} {
				syslog "warn" "Invalid profile: $recProfile"
			}

		} else {
			# Days of the week
			set doW [lindex $fieldList 3]
			if {[regexp {^[0123456]$} $doW] == -1} {
				syslog "warn" "Invalid day of the week in schedule: $doW"
			}

			set recProfile [lindex $fieldList 4]
			if {[regexp {^[a-z]$} $recProfile] == -1} {
				syslog "warn" "Invalid profile: $recProfile"
			}

		}

		set mode [string toupper $mode]

		# Convert dow to list
		set dowList [list]
		for {set i 0} {$i<[string length $doW]} {incr i} {
			lappend dowList [string range $doW $i $i]
		}
		set dowList [lsort -integer $dowList]

		set entry [list $startTcl $stopTcl $frequency $dowList $recProfile $mode]

		# Add entry to schedule
		lappend scheduleList $entry
		
		# Store in array. Index is the start time
		# Removed on 2002-12-03. Sorting is not required
#		if ![info exists scheduleArray($startTcl)] {
#			set scheduleArray($startTcl) $entry
#		} else {
#			# startTcl exists several times
#			syslog "warn" "Identified multiple schedule entries with same start time"
#			exit
#		}
	}

	# Convert array into list. Sort entries by start time
#	set scheduleList [list]
#	set startTclList [lsort -integer [array names scheduleArray]]
#	foreach startTcl $startTclList {
#		lappend scheduleList $scheduleArray($startTcl)
#	}

	return $scheduleList
}


proc CheckSchedule {schedule} {

	# Checks whether start and stop times overlap overlap
	# Return 1 if ok, 0 if overlap was detected

	set lastStopTcl 0
	
	set result 1

	foreach entry $schedule {

		set startTcl [lindex $entry 2]
		
		if {$startTcl < $lastStopTcl} {
			syslog "warn" "Time overlap in schedule!"
			set result 0
		}

		set lastStopTcl [lindex $entry 3]
	}

	return $result
}

proc Wait {tclTime} {

	set waitTime [expr ($tclTime - [clock seconds])]
	
	# Wait until tclTime is reached
	while {$waitTime > 0} {
		
		# Output time
		#syslog "warn" -nonewline "CountDown:\t[format "%4d" $waitTime]\r"
		#flush stdout
		
		# Wait 1 s
		set flag 0
		after 1000 {set flag 1}
		vwait flag
		
		# Update wait time
		set waitTime [expr ($tclTime - [clock seconds])]
	}
	# syslog "warn" ""
}
	


proc Convert2Binary {value} {

    # Converts <value> into binary 16-bit string (MSB first)

    binary scan [binary format "S" $value] "B16" result

    return $result
}

proc DisplayBinary {binaryString} {

    # Display binary string as sequence of hex values

    for {set i 0} {$i<[string length $binaryString]} {incr i} {

	binary scan [string range $binaryString $i $i] "c" byteValue
	set byteValue [expr ($byteValue+0x100)%0x100]
	puts -nonewline [format "%02x " $byteValue]

	if {[expr $i%16] == 15} {
	    puts ""
	}
    }
    puts ""
}


proc IEEE2float {data byteorder} {
    if {$byteorder == 0} {
        set code [binary scan $data cccc se1 e2f1 f2 f3]
    } else {
        set code [binary scan $data cccc f3 f2 e2f1 se1]
    }
    
    set se1  [expr {($se1 + 0x100) % 0x100}]
    set e2f1 [expr {($e2f1 + 0x100) % 0x100}]
    set f2   [expr {($f2 + 0x100) % 0x100}]
    set f3   [expr {($f3 + 0x100) % 0x100}]
    
    set sign [expr {$se1 >> 7}]
    set exponent [expr {(($se1 & 0x7f) << 1 | ($e2f1 >> 7))}]
    set f1 [expr {$e2f1 & 0x7f}]
    
    set fraction [expr {double($f1)*0.0078125 + \
            double($f2)*3.0517578125e-05 + \
            double($f3)*1.19209289550781e-07}]
    
    set res [expr {($sign ? -1. : 1.) * \
            pow(2.,double($exponent-127)) * \
            (1. + $fraction)}]
    return $res
}

