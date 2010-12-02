source settings.tcl
exec "sed" "-i" "-e" "/latitude/s/^.*$/latitude=${LATITUDE}/" "Dream.ini"
exec "sed" "-i" "-e" "/longitude/s/^.*$/longitude=${LONGITUDE}/" "Dream.ini"
exec "sed" "-i" "-e" "/serialnumber/s/^.*$/serialnumber=${RX_NAME}/" "Dream.ini"
set ARGS [list exec dream]
set ARGS [concat $ARGS [list "--rsiout" "127.0.0.1:${RECORD_STATUS_PORT}" "--rciin" $RECORD_CONTROL_PORT]]
set ARGS [concat $ARGS [list "--rsiout" "127.0.0.1:${FORWARD_STATUS_PORT}" "--rciin" $FORWARD_CONTROL_PORT]]
set errorFlag [catch {eval $ARGS } response]
puts "returned $errorFlag $response"
