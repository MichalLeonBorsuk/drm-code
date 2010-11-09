package require http
package require json
source settings.tcl

set ext ".rs*"
set typedir "RSCI"
#set ext ".IQ*"
#set typedir "IQ"

set sourcedir [file join $DATA_DIR $RX_NAME]
set destdir $sourcedir

set informURL "http://$SERVER_ADDRESS/theseus/inform.php"
set requestURL "http://$SERVER_ADDRESS/theseus/rsci_recordings.php?rx_id=$RX_NAME&state=Q&fmt=json"
set putfileURL "http://$SERVER_ADDRESS/theseus/put_rsci_file.php"

set HTTP_TIMEOUT 10000

set undeclaredRecordingsFileName [file join $DATA_DIR "undeclared.txt"]

# Main program
if ![file exists $DATA_DIR] {
 file mkdir $DATA_DIR
}

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

# create the undeclared recordings file if it doesn't exist
close [open $undeclaredRecordingsFileName a]

  #transfer the list
set undeclaredRecordingsFile [open $undeclaredRecordingsFileName r]
#catch {set token [http::geturl $serverURL -method PUT -querychannel $undeclaredRecordingsFile -timeout $HTTP_TIMEOUT]}
set token [http::geturl $informURL -querychannel $undeclaredRecordingsFile -type "text/plain" -timeout $HTTP_TIMEOUT]
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

# Get list of requested rsci files
set token [http::geturl $requestURL -type "text/plain" -timeout $HTTP_TIMEOUT]
upvar #0 $token state
set requests [json::json2dict $state(body)]
#puts $state(body)
puts "requests:"
foreach request $requests {
	set fileTail [dict get $request filename]
	if {[regexp {.*_(....)-(..)-(..)_} $fileTail match year month day]} {
		set filename [file join $destdir $typedir "${year}-${month}-${day}" $fileTail]
		if [file exists $filename] {
		set url "$putfileURL?filename=$fileTail"

		set rsciFile [open $filename "r"]
		puts "putfileURL=$putfileURL"
		puts "about to put $fileTail to $url"
		set token [http::geturl $url -querychannel $rsciFile -type "text/plain" -timeout $HTTP_TIMEOUT]
		puts [http::status $token]
		upvar #0 $token state
		puts $state(body)

		close $rsciFile

			puts -nonewline "File: $filename Size:" 
			puts [file size $filename]
		} else {
			#puts "file $filename n'existe pas"
		}
	}
}
  
  set waitVar 0
  puts "Waiting..."
  after 5000 {set waitVar 1}
  vwait waitVar
}
