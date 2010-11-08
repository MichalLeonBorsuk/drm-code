#** Path to where you copied the TCL-files, remember to user "/" instead of "\" !
set ROOT_DIR "/logging/$RX_NAME"
set ROOT_DIR2 "F:/datafiles"
set PUTTY_DIR "C:/Program Files/putty/"
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

    
