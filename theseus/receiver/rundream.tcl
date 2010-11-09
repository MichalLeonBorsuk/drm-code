source settings.tcl
set ARGS [list exec dream]
set ARGS [concat $ARGS [list "--rsiout" "127.0.0.1:${RECORD_STATUS_PORT}" "--rciin" $RECORD_CONTROL_PORT]]
set ARGS [concat $ARGS [list "--rsiout" "127.0.0.1:${FORWARD_STATUS_PORT}" "--rciin" $FORWARD_CONTROL_PORT]]
set errorFlag [catch {eval $ARGS } response]
puts "returned $errorFlag $response"
