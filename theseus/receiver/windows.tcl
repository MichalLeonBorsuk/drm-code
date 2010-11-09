#** Path to where you copied the TCL-files, remember to user "/" instead of "\" !
set DATA_DIR "F:/datafiles"
set LOG_DIR "F:/logging"
set PUTTY_DIR "C:/Program Files/putty/"
set EXE_SUFFIX ".exe"

#** Windows Time Server - Specify hostname to synchronise time to DCF-clock using NTP
# set TIME_SERVER "ntsvr"


# This is needed to adjust the different <clock clicks> resolutions
set CLICKS_PER_MS 1;

# Set name of the tcl shell
set TCLSH_CMD "tclsh"

# Sending Emails
proc EmailDataFile {fileName} {

    global MAIL_SMTP MAIL_FROM MAIL_TO RX_NAME ROOT_DIR

    set cmdName [file join $ROOT_DIR postie]
    
    set errorFlag [catch {exec $cmdName -host:$MAIL_SMTP \
			      -from:$MAIL_FROM\
			      -to:$MAIL_TO\
			      -s:\"[file tail $fileName]\"\
			      -nomsg\
			      -gmt\
			      -a:$fileName} response]

    # Return response
    if {$errorFlag == 0} {
	return ""
    } else {
	return $response
    }
}

# Synchronise with time server
proc SynchoniseWithTimeServer {time_server} {

    catch {set id [exec net time \\\\$time_server /set /y]}
}

proc MakeRemoteDir {dirName} {
  global SERVER_ADDRESS
  global serverUser
  global serverPassword
  global PUTTY_DIR
  set plinkCmdName [file join $PUTTY_DIR "plink.exe"]

  puts "$plinkCmdName -ssh ${serverUser}@$SERVER_ADDRESS -pw $serverPassword mkdir -p $dirName"
  set errorFlag [catch {exec $plinkCmdName -ssh ${serverUser}@$SERVER_ADDRESS -pw $serverPassword mkdir -p $dirName} response]
  if {$errorFlag} {
    puts "Making directory failed with $response"
  }
}

proc PutFile {gzFileName remoteFileName} {
  global SERVER_ADDRESS
  global serverUser
  global serverPassword
  global PUTTY_DIR
  set pscpCommand [file join $PUTTY_DIR "pscp.exe"]

  set pscpCommand "$pscpCmdName -pw $serverPassword $gzFileName $serverUser@$SERVER_ADDRESS:$remoteFileName"
  puts $pscpCommand
  set errorFlag [catch {exec $pscpCmdName -pw $serverPassword $gzFileName $serverUser@$SERVER_ADDRESS:$remoteFileName} response]
  puts "returned $errorFlag $response"
  if {!$errorFlag} {
    catch {file delete $gzFileName}
  }
}
