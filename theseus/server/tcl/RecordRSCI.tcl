# Read common code and global variables
source SettingsRemoteMonitoring.tcl

package require http ;
# Include support for UDP
package require udp

set serverURL "http://localhost/theseus/inform.php"

set HTTP_TIMEOUT 10000
set ROOT_DIR2 "/tmp"

set tempFileName [file join $ROOT_DIR2 filelist.txt]

proc declareFile {rsciFileName} {
  global tempFileName
  global serverURL
  global HTTP_TIMEOUT
  puts "declaring file $rsciFileName"
  # write the filename to a file (I know, but seems to be the only way)
  set tempFile [open $tempFileName w]
  puts $tempFile "$rsciFileName A"
  close $tempFile

  #transfer the list (of one file)
  set tempFile [open $tempFileName r]
  set token [http::geturl $serverURL -querychannel $tempFile -type "text/plain" -timeout $HTTP_TIMEOUT]
  #check what happened
  upvar #0 $token state
  puts [http::status $token]
puts "body of get:"
puts $state(body)
puts "end of body"
  close $tempFile
}

proc MakeFileName {rxID} {
  global RSI_FILES_ROOT
  set currentTime [clock seconds]
  set currentDate [clock format $currentTime -format "%Y-%m-%d" -gmt 1]
  set currenthhmmss [clock format $currentTime -format "%H-%M-%S" -gmt 1]
  set dir [file join $RSI_FILES_ROOT "$rxID" "${currentDate}"]
  file mkdir $dir
  return [file join "$dir" "xxxxxxxxxx${rxID}_${currentDate}_${currenthhmmss}_00000000.rsA"]
}

proc OpenRSCIFile {fileName} {
  puts "Opening file: $fileName"
  set ttyFile [open $fileName w]
  fconfigure $ttyFile -translation binary
  return $ttyFile
}

proc ProcessStatusData {ttyMediator fileName} {
  global ttyFile

  if [eof $ttyMediator] {
    puts "EOF on input socket"
    return
  }

  set data [read $ttyMediator]

  # open output file if we have data and it's not already open
  if {[string length $data] > 0 && ![info exists ttyFile]} {
    set ttyFile [OpenRSCIFile $fileName]
puts "Opened $fileName as $ttyFile"
  }
  puts -nonewline $ttyFile $data
}

##### Main #####

# cmd line args
set rxID [lindex $argv 0]
set recordTime [lindex $argv 1]

set fileName [MakeFileName $rxID]

# connect to mediator
set ttyMediator [socket localhost $SUBSCRIBERS_SERVER_PORT]
fconfigure $ttyMediator -translation binary -blocking 0
fileevent $ttyMediator readable [list ProcessStatusData $ttyMediator $fileName]

# request the required ID
puts $ttyMediator "$rxID"
flush $ttyMediator

set waitVar 0
after [expr $recordTime * 1000] {set waitVar 1}
vwait waitVar

close $ttyMediator
if [info exists ttyFile] {
  close $ttyFile
  declareFile $fileName
}


puts "exiting"

