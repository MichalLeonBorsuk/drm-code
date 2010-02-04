function findGraph(oNode, tagName, graphData) {
  var data = findText(oNode, tagName);

  if (data != "") {
    graphData.start = parseFloat(oNode.getElementsByTagName(tagName)[0].getAttribute("start"));
    graphData.end = parseFloat(oNode.getElementsByTagName(tagName)[0].getAttribute("end"));

    var dataValues = data.split(",");
    graphData.points = new Array(dataValues.length);

    for (var p=0; p < dataValues.length; p++) {
      if (dataValues[p]=="?")
        graphData.points[p] = 1;
      else
        graphData.points[p] = parseFloat(dataValues[p]);
    }
    return true;
  } else {
    return false;
  }
}

function findText(oNode, tagName)  {
  var sText="";
  try {
    sText = oNode.getElementsByTagName(tagName)[0].childNodes[0].nodeValue;
  }
  catch(error) {
   sText = "";
  }
  return sText;
}

function findAudioStats(oNode, tagName, audioStatsData)  {

  var percentCorrect = findText(oNode, tagName);

  if (percentCorrect != "") {
    audioStatsData.percentCorrect = percentCorrect;
    audioStatsData.totalAudioFrames = oNode.getElementsByTagName(tagName)[0].getAttribute("numAudioFrames");
    return true;
  } else {
    return false;
  }
}

function findStats(oNode, tagName, statsData) {
  if (findText(oNode, tagName) != "") {
    statsData.numFrames = oNode.getElementsByTagName(tagName)[0].getAttribute("numFrames");
    var stats = oNode.getElementsByTagName(tagName)[0].getElementsByTagName("statistic");
    var data = new Array;
    for (var i=0; i<stats.length; i++) {
      percentile = stats[i].getAttribute("percentile");
      value = stats[i].childNodes[0].nodeValue;
      data[percentile] = value;
    }
    statsData.stats = data;
    return true;
  } else {
    return false;
  }
}
