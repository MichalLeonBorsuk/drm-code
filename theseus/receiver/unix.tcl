#** Path to where you copied the TCL-files, remember to user "/" instead of "\" !
set DATA_DIR "/data/datafiles"
set LOG_DIR "/data/logging"
set EXE_SUFFIX ""

# This is needed to adjust the different <clock clicks> resolutions
set CLICKS_PER_MS 1000;

# Set name of the tcl shell
set TCLSH_CMD "tclsh8.3"

# Sending Emails
proc EmailDataFile {fileName} {

    global MAIL_TO

    set errorFlag [catch {exec mpack \
			      -s \"[file tail $fileName]\"\
			      $fileName $MAIL_TO} response]
    
 
    # Create .snd file to show that the email was successfully sent. Contains the current time.
    if {$errorFlag == 0} {
	return ""
    } else {
	return $response
    }
}

# Synchronise with time server
proc SynchoniseWithTimeServer {time_server} {

}

proc MakeRemoteDir {dirName} {
  global SERVER_ADDRESS
  global serverUser

  puts "ssh ${serverUser}@$SERVER_ADDRESS mkdir -p $dirName"
  set errorFlag [catch {exec ssh ${serverUser}@${SERVER_ADDRESS} mkdir -p $dirName} response]
  if {$errorFlag} {
    puts "Making directory failed with $response"
  }
}

proc PutFile {gzFileName remoteFileName} {
  global SERVER_ADDRESS
  global serverUser
  global PUTTY_DIR
  set pscpCommand [file join $PUTTY_DIR "pscp.exe"]

  set pscpCommand "scp $gzFileName ${serverUser}@${SERVER_ADDRESS}:${remoteFileName}"
  puts $pscpCommand
  set errorFlag [catch {exec scp ${gzFileName} ${serverUser}@${SERVER_ADDRESS}:${remoteFileName}} response]
  puts "returned $errorFlag $response"
  if {!$errorFlag} {
    catch {file delete $gzFileName}
  }
}

package require syslog
