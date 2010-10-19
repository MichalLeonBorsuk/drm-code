<?php
  header("Content-type: application/xml");
$fp = stream_socket_client("tcp://localhost:8082", $errno, $errstr, 30);
if (!$fp) {
    echo "$errstr ($errno)<br />\n";
} else {
    while (!feof($fp)) {
        echo fgets($fp, 1024);
    }
    fclose($fp);
}
?>
