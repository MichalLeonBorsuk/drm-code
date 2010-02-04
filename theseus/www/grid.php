<html>
<head>
<link rel="shortcut icon" href="http://132.185.142.42/favicon.ico" type="image/vnd.microsoft.icon">
<link rel=StyleSheet href="/style.css" type="text/css">
<title>THESEUS Remote Monitoring System</title>
</head>
<body>
<center><img src="/images/banner.png" width="990" height="40" border="0" vspace="0" hspace="0"></center>

<?php
    include("lib/calendar.php");

    $user = "root";
    $pass = "bbc";
    $dbh = new PDO('mysql:host=localhost;dbname=theseus', $user, $pass);

    // go through a month at a time
    echo "<p><table align=center>";
    for ($year=2007; $year < 2015; $year++) {
      echo "<tr><td ALIGN=CENTER COLSPAN=6><h2>$year</h2></td></tr>";
      for ($month=1; $month <= 12; $month++) {
        if ($month == 1 || $month == 7) { echo "<tr>"; }
        echo "<td valign=top>";

       $stmt = $dbh->prepare("SELECT DISTINCT EXTRACT(DAY FROM recording_start) FROM available WHERE Year(recording_start) = $year AND MONTH(recording_start) = $month");

       $stmt->setFetchMode(PDO::FETCH_ASSOC);
       $stmt->execute();
       $arrValues = $stmt->fetchAll();

       $links = array();
       foreach($arrValues as $row) {
         foreach($row as $col) {
          $date = sprintf("%04d-%02d-%02d", $year, $month, $col);
          $links["$col"] = array('day.php?date='.$date, 'linked-day');
         }
       }

        echo generate_calendar($year, $month, $links);
        echo "</td>";
        if ($month == 12 || $month == 6) { echo "</tr>"; }
      }
    }
    echo "</table>";

    $dbh = null;
?>

</body>
</html>

