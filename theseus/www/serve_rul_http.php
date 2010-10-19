<?php
set_time_limit(0);
ob_implicit_flush();

$receiver_serial_number = $_GET['receiver'];

header("Content-type: text/rsi-url");
header("Content-Disposition: inline; filename=\"$receiver_serial_number.rsu\"");

$http_rsi_host = "theseus.dyndns.ws";
$http_rsi_path = "/theseus/listen_http.php";

print "http://" . $http_rsi_host . $http_rsi_path . "?receiver=" . $receiver_serial_number;
?>

