<?php

function html_out($arrValues)
{
    print "<table>\n";
    print "<tr>\n";
    $names = array_keys($arrValues[0]);
    foreach($names as $col)
    {
	print "<th>$col</th>";
    }
    print "</tr>\n";
    foreach($arrValues as $row) {
	$entry = array();
	print "<tr>\n";
        foreach($row as $col) {
		print "<td>$col</td>";
	}
	print "</tr>\n";
    }
    print "</table>\n";
}

function json_out($arrValues)
{
    header('Content-type: text/json');
    print json_encode($arrValues);
}

$rx_id = "%";
$state = "%";
$maxnum = 100;
$start = 0;
$fmt='html';

$sql = "SELECT * FROM rsci_recordings";
$condition = array();

if(isset($_GET['fmt']))
{
	$fmt = $_GET['fmt'];
}
if(isset($_GET['rx_id']))
{
	$rx_id = $_GET['rx_id'];
	$condition[] = "rx_id = '$rx_id'";
}
if(isset($_GET['state']))
{
	$state = $_GET['state'];
	$condition[] = "state = '$state'";
}
if(isset($_GET['maxnum']))
{
	$maxnum = $_GET['maxnum'];
}
if(isset($_GET['start']))
{
	$start = $_GET['start'];
}

if(count($condition)>0)
{
	$sql .= " WHERE ".implode(" AND ", $condition);
}
$sql .= " LIMIT $maxnum OFFSET $start";

try {
    require_once("../connect.php");
    $stmt = $dbh->prepare($sql);
    $stmt->setFetchMode(PDO::FETCH_ASSOC);
    $stmt->execute();
    $arrValues = $stmt->fetchAll();
    if($fmt == 'html')
    {
        html_out($arrValues);
    }
    if($fmt == 'json')
    {
        json_out($arrValues);
    }
    $dbh = null;
} catch (PDOException $e) {
    print "Error!: " . $e->getMessage() . "<br/>";
    die();
}
?>
