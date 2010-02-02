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

    