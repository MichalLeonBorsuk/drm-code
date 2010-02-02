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
