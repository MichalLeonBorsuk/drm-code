<?php
function show_archive($dbh, $date)
{
   echo "<CENTER><P><h2>Archive ($date)</h2><CENTER>";

   $stmt = $dbh->prepare("SELECT DISTINCT rx_id FROM rsci_recordings WHERE CAST(recording_start AS DATE) = ?");
   $stmt->setFetchMode(PDO::FETCH_ASSOC);
   $stmt->execute(array($date));
   $rxValues = $stmt->fetchAll();

   $numRxs = count($rxValues);

   $stmt = $dbh->prepare("SELECT recording_start, state, rx_id, filename FROM rsci_recordings WHERE CAST(recording_start AS DATE) = ?");
   $stmt->setFetchMode(PDO::FETCH_ASSOC);
   $stmt->execute(array($date));
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
     $recDate = substr($row["recording_start"], 0, 10);

     $state = $row["state"];
     $id = $row["rx_id"];
     $fileName = $row["filename"];

     if (!isset($table[$time])) {
       $table[$time] = array();
     }
     
     $tdString = "";
     switch ($state) {
       case "R":
         $tdString = "<A title=\"click to request upload from receiver\""
		." href=\"day.php?action=request&amp;filename=$fileName\">"
		."<img src=\"images/requestFile.gif\"/></A>";
         break;
       case "Q":
         $tdString = "<span title=\"file has been requested\">Q</span>";
         break;
       case "A":
         $tdString = "<A title=\"click to listen\""
		." href=\"/rsci/$id/$recDate/$fileName\">"
		."<img src=\"images/listen_icon.gif\"/></A>";
         break;
     }

     $table[$time][$id] = $tdString;
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
}


/* Change the state of an entry in 'rsci_recordings' table of the theseus database from "Remote" to "reQuested" */
function request_file($dbh, $fileName)
{
    try {
	$stmt = $dbh->prepare("UPDATE rsci_recordings SET state='Q' WHERE filename=?"); 
	$stmt->execute(array($fileName));
    } catch (PDOException $e) {
        print "Error!: " . $e->getMessage() . "<br/>";
    }
}

?>
