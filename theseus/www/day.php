<html>
<head>
<link rel="shortcut icon" href="/images/favicon.ico" type="image/vnd.microsoft.icon">
<link rel=StyleSheet href="/style.css" type="text/css">
<title>THESEUS Remote Monitoring System</title>
</head>
<body>
<center><img src="/images/banner.png" width="990" height="40" border="0" vspace="0" hspace="0"></center>


<?php
   $date = $_GET["date"];

   echo "<CENTER><P><h2>Archive ($date)</h2><CENTER>";

   require_once("../connect.php");

   $stmt = $dbh->prepare("SELECT DISTINCT rx_id FROM rsci_recordings WHERE CAST(recording_start AS DATE) = '$date'");
   $stmt->setFetchMode(PDO::FETCH_ASSOC);
   $stmt->execute();
   $rxValues = $stmt->fetchAll();

   $numRxs = count($rxValues);

   $stmt = $dbh->prepare("SELECT recording_start, state, rx_id FROM rsci_recordings WHERE CAST(recording_start AS DATE) = '$date'");
   $stmt->setFetchMode(PDO::FETCH_ASSOC);
   $stmt->execute();
   $timeValues = $stmt->fetchAll();

   echo "<table>";

   echo "<tr>";
   echo "<th>Start time</th><th COLSPAN=$numRxs>Receiver IDs</th>";
   echo "</tr>";

   echo "<tr>";
   echo "<td></td>";

   foreach($rxValues as $row) {
       $id = $row['rx_id'];
       echo "<td>$id</td>";
   }
   echo "</tr>";

   $numTimes = count($timeValues);

   sort($timeValues);

   $table = array();

   foreach($timeValues as $row) {
     $hour = substr($row["recording_start"], 11, 2);
     $min = substr($row["recording_start"], 14, 2);
     $time = "$hour:$min";

     $state = $row["state"];
     $id = $row["rx_id"];

     if (!isset($table[$time])) {
       $table[$time] = array();
     }

     $table[$time][$id] = $state;
   }

   foreach($table as $key => $value) {
     echo "<tr>";
     echo "<td>$key</td>";

     foreach($rxValues as $row) {
      $rxValuesId = $row["rx_id"];
       if ( isset($value[$rxValuesId]) ) {
         echo "<td>$value[$rxValuesId]</td>";
       } else {
         echo "<td></td>";
       }
     }

     echo "</tr>";
   }

   echo "</table>";

   $dbh = null;

?>

</body>
</html>
