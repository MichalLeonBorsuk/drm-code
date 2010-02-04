<?php
try {
    $user = "root";
    $pass = "bbc";
    $dbh = new PDO('mysql:host=localhost;dbname=theseus', $user, $pass);
    $stmt = $dbh->prepare("SELECT * FROM available");
    $stmt->setFetchMode(PDO::FETCH_ASSOC);
    $stmt->execute();
    print "<table>\n";
    print "<tr>\n";
    for($i=0; $i<$stmt->columnCount(); $i++) {
	$meta = $stmt->getColumnMeta($i);
	print "<th>".$meta['name']."</th>";
    }
    print "</tr>\n";
    $arrValues = $stmt->fetchAll();
    foreach($arrValues as $row) {
	print "<tr>\n";
        foreach($row as $col) {
		print "<td>$col</td>";
	}
	print "</tr>\n";
    }
    print "</table>\n";
    $dbh = null;
} catch (PDOException $e) {
    print "Error!: " . $e->getMessage() . "<br/>";
    die();
}
?>
