#***************************************************************************************
# General settings
#***************************************************************************************

set PATH_UTILS /project/drm/mayflower/packages/local/bin

# Set IP addresses and ports

# UDP port to which all datagrams are sent
set COLLECTOR_PORT 13000 

# receivers will be removed from the list after this time
set RECEIVER_TIMEOUT 360 

# length of history plots (in frames)
set HISTORY_LENGTH 100

# number of frames before reaping label
set MAX_LABEL_COUNT 48

# TCP server port which will deliver an HTML page to any connecting clients
set HTML_PORT 8080 

# TCP server port which will deliver a KML page to any connecting clients
set KML_PORT 8081 

# TCP server port which will deliver a KML page to any connecting clients
set XML_PORT 8082

set KML_INITIAL_VIEW_LAT 51.288592
set KML_INITIAL_VIEW_LONG -0.213709
set KML_INITIAL_VIEW_RANGE 2000000

set KML_HTTP_SERVER_ROOT http://drmvcs.rd.bbc.co.uk/drm

set RSI_FILES_ROOT /var/www/rsci
# TCP server port which receiving sites will connect to (probably using netcat)
set SITES_SERVER_PORT 12001 

# TCP server port which clients subscribing to an RSI stream will connect to
set SUBSCRIBERS_SERVER_PORT 12000 

# TCP port for Mediator Diagnostics
set MEDIATOR_DIAG_SERVER_PORT 8085

# UDP port for (XML) control packets (e.g. re-tuning receivers)
set CONTROL_PORT 12002

# Higher RSI profile when a client has subscribed to the stream
set STREAMING_PROFILE A 

# Lower RSI profile when only being used to update the table
set NORMAL_PROFILE A 


# Specify here, which version of the SE127 protocol the receiver provides.
# Options are: 
# 2	Version 2.x
# 3	Version 3.x
set SE127_VERSION 3
set SE127_PROTOCOL DCP

set STATUS_PROTOCOL UDP

# set whether to use TCL-DP, "DP" or TCL-UDP, "UDP"
set UDP_PACKAGE UDP

#***************************************************************************************
# OS-specific functions
# Select the OS by uncommenting the source
#***************************************************************************************

set OS_CODE windows.tcl
#set OS_CODE unix.tcl

# needed to avoid error in include.tcl
set USE_EXTERNAL_SIGNAL_STRENGTH 0
