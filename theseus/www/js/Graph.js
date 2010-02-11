function Graph(name, lineColour) {
  this._name = name;
  this._lineColour = lineColour;
}

Graph.prototype._name;
Graph.prototype._lineColour;

Graph.prototype._xStart;
Graph.prototype._xEnd;
Graph.prototype._xLength;
Graph.prototype._xTickStep;
Graph.prototype._xUnit;

Graph.prototype._yStart;
Graph.prototype._yEnd;
Graph.prototype._yTickStep;
Graph.prototype._yUnit;

Graph.prototype._layout;
Graph.prototype._plotter;

Graph.prototype.clear = function() {
  if (typeof(this._plotter) != 'undefined') {
    this._plotter.clear();
    delete this._plotter;
  }
}

Graph.prototype.draw = function(data, xStart, xEnd, xTickStep, xUnit, yStart, yEnd, yTickStep, yUnit) {
  var rescaleGraph = false;
  if (typeof(this._plotter) != 'undefined') {
    if (xStart != this._xStart)
      rescaleGraph = true;
    if (xEnd != this._xEnd)
      rescaleGraph = true;

    if (yStart != this._yStart)
      rescaleGraph = true;
    if (yEnd != this._yEnd)
      rescaleGraph = true;
    if (data.length != this._xLength)
      rescaleGraph = true;
  }

  this._xStart = xStart;
  this._xEnd = xEnd;
  this._xTickStep = xTickStep;
  this._xLength = data.length;
  this._xUnit = xUnit;

  this._yStart = yStart;
  this._yEnd = yEnd;
  this._yTickStep = yTickStep;
  this._yUnit = yUnit;

  if (typeof(this._plotter) == 'undefined')
    rescaleGraph = true;

  if (rescaleGraph) {
    this.clear();
    this._create();
    this._data = new Array(this._xLength);

    for (var i=0; i < this._xLength; i++) {
      this._data[i] = new Array(2);
      this._data[i][0] = i;
    }
  }

  for (var i=0; i < this._xLength; i++) {
    this._data[i][1] = data[i]-this._yStart;
  }

  this._layout.removeDataset("data");
  this._layout.addDataset("data", this._data);
  this._layout.evaluate();
  this._plotter.clear();
  this._plotter.render();
}


Graph.prototype._create = function() {
    var xStepSize = Number((this._xEnd-this._xStart)/this._xLength);

    // use ""+ to convert to string
    var lowestxTickNumber = Math.ceil(this._xStart);
    var highestxTickNumber = Math.ceil(this._xEnd);

    var numxTicks = Math.round((highestxTickNumber - lowestxTickNumber)/this._xTickStep)+1;

    var xTickLabels = new Array();
    var xTickNumber = lowestxTickNumber;

    for (var i=0; i < numxTicks; i++)
    {
      xTickLabels[i] = new Array();

      var xTickPosition = Math.round((xTickNumber - this._xStart)/xStepSize);
      xTickLabels[i].label = xTickNumber + this._xUnit;
      xTickLabels[i].v = xTickPosition;
      xTickNumber += this._xTickStep;
    }

    var numyTicks = Math.abs(Math.round((this._yEnd-this._yStart)/this._yTickStep))+1;

    // if yStart is < 0 work ticks out backwards from yEnd

    var yTickLabels = new Array();

    if (this._yStart >= 0) {
      var yTickPosition = 0;
      for (var i=0; i < numyTicks; i++) {
        yTickLabels[i] = new Array();

        var yTickLabel = Math.round(this._yStart + (i*this._yTickStep));

        yTickLabels[i].label = yTickLabel + this._yUnit;
        yTickLabels[i].v = yTickPosition;

        yTickPosition += this._yTickStep;
      }
    } else {
      var minYTickValue = -(numyTicks-1)*this._yTickStep - this._yStart + this._yEnd;
    
      var yTickPosition = minYTickValue; 
      for (var i=0; i < numyTicks; i++) {
        yTickLabels[i] = new Array();

        var yTickLabel = Math.round(this._yEnd - ((numyTicks-1-i)*this._yTickStep));
        yTickLabels[i].label = yTickLabel + this._yUnit;
        yTickLabels[i].v = yTickPosition;
        yTickPosition += this._yTickStep;
      }
    }

    var yTickLabelMaxLen = 0;
    for (var i=0; i < numyTicks; i++) {
      if (yTickLabels[i].label.length > yTickLabelMaxLen)
      yTickLabelMaxLen = yTickLabels[i].label.length;
    }

    var layoutOptions = {
      "yAxis": [0, this._yEnd-this._yStart],
      "yTickPrecision": 0,
      "yTicks" : yTickLabels,
      "xTicks": xTickLabels
   };

    var renderOptions = {
      "shouldFill": false,
      "strokeWidth": 1.5,
      "axisLineColor" : Color.grayColor(),
      "axisLabelWidth": 10*yTickLabelMaxLen,
      "strokeColor": this._lineColour,
      "axisLabelFontSize": 9,
      "axisLineWidth": 2,
      "padding": {left:Math.floor(7.5*yTickLabelMaxLen), right:8, top:8, bottom:10}
    };

    this._layout = new PlotKit.Layout("line", layoutOptions);
    var canvas = MochiKit.DOM.getElement(this._name);
    this._plotter = new PlotKit.CanvasRenderer(canvas, this._layout, renderOptions);
}
