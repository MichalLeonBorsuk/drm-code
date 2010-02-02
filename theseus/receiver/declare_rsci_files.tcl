package require http ;

source settings.tcl

set ext ".rs*"
set typedir "RSCI"
#set ext ".IQ*"
#set typedir "IQ"

set sourcedir [file join $ROOT_DIR2 $RX_NAME]
set destdir [file join $ROOT_DIR2 $RX_NAME]
set destURL "/$RX_NAME"
set gzCmdName [file join $ROOT_DIR gzip]  
set pscpCmdName [file join $PUTTY_DIR "pscp" $EXE_SUFFIX]
set puttyCmdName [file join $PUTTY_DIR "putty" $EXE_SUFFIX]
set plinkCmdName [file join $PUTTY_DIR "plink" $EXE_SUFFIX]

set serverURL "http://192.168.11.201/inform.php"

set HTTP_TIMEOUT 10000

set undeclaredRecordingsFileName [file join $ROOT_DIR2 undeclared.txt]

proc MakeRemoteDir {dirName} {
  global serverAddress
  global serverUser
  global serverPassword
  global plinkCmdName

  set tty [open "putty_commands.scr" w]
  puts $tty "mkdir $dirName"
  close $tty

  puts "$plinkCmdName -ssh ${serverUser}@$serverAddress -pw $serverPassword mkdir $dirName"
  set errorFlag [catch {exec $plinkCmdName -ssh ${serverUser}@$serverAddress -pw $serverPassword mkdir $dirName} response]
  if {$errorFlag} {
    puts "Making directory failed with $response"
  }
}


# Main program

while 1 {
  if ![catch {set filelist [lsort [glob [file join $sourcedir "*$ext"]]]}] {

    # remove the latest file
    set filelist [lrange $filelist 0 [expr [llength $filelist] - 2]]

    foreach file $filelist {
		# Parse file name into YMD
		regexp {.*_(....)-(..)-(..)_} $file match year month day

		puts "considering $file"
		
		# move the original file to the "organised" directory 
		set subdirname [file join $destdir $typedir "${year}-${month}-${day}"]
		if ![file exists $subdirname] {
		 file mkdir $subdirname
		}
		
		file rename $file $subdirname
		
		# TODO: lock the output file
		set undeclaredRecordingsFile [open $undeclaredRecordingsFileName a]
		puts $undeclaredRecordingsFile [file tail $file]
		close $undeclaredRecordingsFile
		# TODO: unlock the output file

	}
  }

  #transfer the list
set undeclaredRecordingsFile [open $undeclaredRecordingsFileName r]
#catch {set token [http::geturl $serverURL -method PUT -querychannel $undeclaredRecordingsFile -timeout $HTTP_TIMEOUT]}
set token [http::geturl $serverURL -querychannel $undeclaredRecordingsFile -type "text/plain" -timeout $HTTP_TIMEOUT]
puts "about to do the post"
#set token [http::geturl $serverURL -method PUT -timeout $HTTP_TIMEOUT]
#check what happened
upvar #0 $token state
#puts [join [array names state] ","]
#puts $state(body)
puts [http::status $token]

#remove succesfully declared lines from the file
seek $undeclaredRecordingsFile 0
set file_data [read $undeclaredRecordingsFile]
set undeclaredRecordings [split $file_data "\n"]
puts "length is [llength $undeclaredRecordings]"
set declaredRecordings [split $state(body) "\n"]

foreach recording $declaredRecordings {
	#puts "Would ideally like to expunge $recording"
	#set i [lsearch -regexp $undeclaredRecordings $recording]
	set i [lsearch $undeclaredRecordings "*[string trim $recording]*"]
	if {$i != -1} {
		puts "Expunging file $recording"
		set undeclaredRecordings [lreplace $undeclaredRecordings $i $i]
	}
}

close $undeclaredRecordingsFile
#re-write the file
set undeclaredRecordingsFile [open $undeclaredRecordingsFileName w]
puts -nonewline $undeclaredRecordingsFile [join $undeclaredRecordings "\n"]
close $undeclaredRecordingsFile
puts [join $undeclaredRecordings "\n"]
  
  set waitVar 0
  puts "Waiting..."
  after 5000 {set waitVar 1}
  vwait waitVar
}