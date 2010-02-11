function MonitoringItemData(type) {
  this.psdGraphData = new Object();
  this.impGraphData = new Object();
  this.rdbvHistoryGraphData = new Object();
  this.rwmmHistoryGraphData = new Object();
  this.audioStats = new Object();
  this.wmfStats = new Object();
  this.wmmStats = new Object();
  this.merStats = new Object();
  this.dbvStats = new Object();

  this.type = type;
}


MonitoringItemData.prototype.CreateMarker = function(point, iconMarkerBase) {
  this.marker = new GMarker(point, iconMarkerBase);
}

MonitoringItemData.prototype.type;

MonitoringItemData.prototype.marker;

MonitoringItemData.prototype.psdGraphData;
MonitoringItemData.prototype.impgraphData;
MonitoringItemData.prototype.rdbvHistoryGraphData;
MonitoringItemData.prototype.rwmmHistoryGraphData;

MonitoringItemData.prototype.name;

MonitoringItemData.prototype.latitude;
MonitoringItemData.prototype.longitude;

MonitoringItemData.prototype.lastUpdate;

MonitoringItemData.prototype.rdmo;
MonitoringItemData.prototype.mode;

MonitoringItemData.prototype.frequency;
MonitoringItemData.prototype.rfLevel;
MonitoringItemData.prototype.wmm;

MonitoringItemData.prototype.audioStatus;
MonitoringItemData.prototype.stationLabel;

MonitoringItemData.prototype.psdPresent;
MonitoringItemData.prototype.impPresent;
MonitoringItemData.prototype.rdbvHistoryPresent;
MonitoringItemData.prototype.rwmmHistoryPresent;        

MonitoringItemData.prototype.audioStats;
MonitoringItemData.prototype.audioStatsPresent;
MonitoringItemData.prototype.wmfStats;
MonitoringItemData.prototype.wmfStatsPresent;
MonitoringItemData.prototype.wmmStats;
MonitoringItemData.prototype.wmmStatsPresent;
MonitoringItemData.prototype.merStats;
MonitoringItemData.prototype.merStatsPresent;
MonitoringItemData.prototype.dbvStats;
MonitoringItemData.prototype.dbvStatsPresent;

MonitoringItemData.prototype.parse = function(xmlElement)
{
  this.name = xmlElement.getAttribute("name");

  this.latitude = xmlElement.getAttribute("latitude");
  this.longitude = xmlElement.getAttribute("longitude")

  if (this.latitude == null)
    this.latitude = "0.0";

  if (this.longitude == null)
    this.longitude = "0.0";

  this.lastUpdate = findText(xmlElement, "last_update");
  this.rdmo = findText(xmlElement, "mode");
  this.frequency = findText(xmlElement, "frequency");
  this.rfLevel = findText(xmlElement, "signal");
  this.wmm = findText(xmlElement, "wmm");
  this.audioStatus = findText(xmlElement, "audio");
  this.stationLabel = findText(xmlElement, "label");
  this.audioStatsPresent = findAudioStats(xmlElement, "audio_stats", this.audioStats);
  this.wmfStatsPresent = findStats(xmlElement, "xwmf", this.wmfStats);
  this.wmmStatsPresent = findStats(xmlElement, "xwmm", this.wmmStats);
  this.merStatsPresent = findStats(xmlElement, "xmer", this.merStats);
  this.dbvStatsPresent = findStats(xmlElement, "xdbv", this.dbvStats);

  this.psdPresent = findGraph(xmlElement, "psd", this.psdGraphData);
  this.impPresent = findGraph(xmlElement, "pir", this.impGraphData);
  this.rdbvHistoryPresent = findGraph(xmlElement, "rdbv_history", this.rdbvHistoryGraphData);
  this.rwmmHistoryPresent = findGraph(xmlElement, "rwmm_history", this.rwmmHistoryGraphData);

  this.mode = "";
  if (this.rdmo == "drm_")
    this.mode = "Drm";
  else if (this.rdmo == "am__")
    this.mode = "AM";
  else if (this.rdmo == "wbfm")
    this.mode = "FM";
}
