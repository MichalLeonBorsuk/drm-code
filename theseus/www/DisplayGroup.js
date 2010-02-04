function DisplayGroup(name, imageFilenamePath, items) {
  this._nodeImageFilename = imageFilenamePath + "/node.png";
  this._nodeLastImageFilename = imageFilenamePath + "/node_last.png";

  // preload icons
  // this avoids flickering when we insert html with img tags into the DOM
  if (document.images) {
    this._nodeImage = new Image(16, 22);
    this._nodeImage.src = this._nodeImageFilename;

    this._nodeLastImage = new Image(16,22);
    this._nodeLastImage.src = this._nodeLastImageFilename;
  }

  this._groupings = new Object();
  this._name = name;
  this._items = items;

  this._updateDisplayFunction = function() {};
  this._currentSortAttribute = "None";
  this._previousItemCount = 0;

  this._nodeActionFunctionName = "";
  this._nodeActionHtml = "";

  this._groupActionFunctionName = "";
  this._groupActionHtml = "";
}

DisplayGroup.prototype.setNodeActionHtml = function(nodeActionHtml) {
  this._nodeActionHtml = nodeActionHtml;
}

DisplayGroup.prototype.setNodeActionFunctionName = function(nodeActionFunctionName) {
  this._nodeActionFunctionName = nodeActionFunctionName;
}

DisplayGroup.prototype.setGroupActionHtml = function(groupActionHtml) {
  this._groupActionHtml = groupActionHtml;
}

DisplayGroup.prototype.setGroupActionFunctionName = function(groupActionFunctionName) {
  this._groupActionFunctionName = groupActionFunctionName;
}

DisplayGroup.prototype._previousItemCount;
DisplayGroup.prototype._name;
DisplayGroup.prototype._items;
DisplayGroup.prototype._updateDisplayFunction;
DisplayGroup.prototype._nodeActionFunctionName;
DisplayGroup.prototype._nodeActionHtml;
DisplayGroup.prototype._groupActionFunctionName;
DisplayGroup.prototype._groupActionHtml;
DisplayGroup.prototype._currentSortAttribute;

DisplayGroup.prototype._nodeImage;
DisplayGroup.prototype._nodeImageFilename

DisplayGroup.prototype._nodeLastImage;
DisplayGroup.prototype._nodeLastImageFilename;

DisplayGroup.prototype._groupings;

DisplayGroup.prototype._showDefaults;

DisplayGroup.prototype.create = function(divID, attributes) {
  this._attributes = attributes;
  this._attributes.push("None");

  var html = "";
  html += "<div style=\"height: 100%\">";
  html += "<div>";
  html += "<b>Sort by:</b><br>";
  html += "<form style=\"margin: 0\" name=\"display_groups\" action=\"\">";

  for (var i=0; i < this._attributes.length; i++) {
    html += "<input type=\"radio\" name=\"group\" id=\"radio_" + this._attributes[i] + "\" value=\"" + this._attributes[i] + "\" onClick=\"" + this._name + ".readRadio('" + this._attributes[i]  + "')\">" + this._attributes[i]+ "<br>";
  }

  html += "</form>";

  html += "<form style=\"margin: 0\" name=\"display_buttons\" action=\"\">";
  html += "<input type=\"button\" name=\"select_all\" value=\"See all\" onClick=\"" + this._name + ".showAllGroups(true)\">";
  html += "<input type=\"button\" name=\"deselect_all\" value=\"Hide all\" onClick=\"" + this._name + ".showAllGroups(false)\">";
  html += "</form>";
  html += "<hr>";
  html += "</div>";

  html += "<div style=\"height: 350px; margin: 0; overflow: auto\" id=\"display_nodes\"></div>";
  html += "</div>";
  document.getElementById(divID).innerHTML = html;

  this._showDefaults = new Object();
}

DisplayGroup.prototype.defaultShow = function(attribute, valueArray) {
  this.setSortAttribute(attribute);
  this._showDefaults = valueArray;
}

DisplayGroup.prototype.showAllGroups = function(show) {
  for (var group in this._groupings) {
    document.getElementById("checkbox_"+group).checked = show;
    this._groupings[group].display = show;
    this.updateItemDisplays(group);
  }
  this._updateDisplayFunction();
}

DisplayGroup.prototype.readCheckbox = function(group) {
  var checked=document.getElementById("checkbox_"+group).checked;
  this._groupings[group].display = checked;
  this.updateItemDisplays(group);
  this._updateDisplayFunction();
}

DisplayGroup.prototype.writeCheckboxes = function() {
  for (var group in this._groupings) {
    document.getElementById("checkbox_"+group).checked = this._groupings[group].display;
  }
}

DisplayGroup.prototype.setUpdateDisplayFunction = function(func) {
  this._updateDisplayFunction = func;
}

DisplayGroup.prototype.updateItemDisplays = function(group) {
  var checked = this._groupings[group].display;

  if (this._groupings[group].items.length == 0) {  // special case for grouping "none"
    if (checked)
      this._items[group].display = true;
    else
      this._items[group].display = false;
  } else {
    for (var i=0; i < this._groupings[group].items.length; i++) {
      if (checked)
        this._items[this._groupings[group].items[i]].display = true;
      else
        this._items[this._groupings[group].items[i]].display = false;
    }
  }
}

DisplayGroup.prototype.setSortAttribute = function(attribute) {
  this._currentSortAttribute = attribute;
  this.writeRadio();
}

DisplayGroup.prototype.readRadio = function(attribute) {
  this._currentSortAttribute = attribute;
  this._updateGroups(true);
}

DisplayGroup.prototype.writeRadio = function() {
  document.getElementById("radio_"+this._currentSortAttribute).checked = true;
  this._updateGroups(true);
}

DisplayGroup.prototype.update = function(redoGroups) {
  this._updateGroups(redoGroups);
}

DisplayGroup.prototype._updateGroups = function(groupsChange) {
  var groupingsNew = new Object();
  current = this._currentSortAttribute;
  var value = "";

  var itemCount = 0;
  for (var item in this._items) {
    itemCount++;
    if (current != "None") {
      value = this._items[item].attributes[current];
    } else {
      value = item;
    }

    if (typeof(groupingsNew[value]) == 'undefined')  {
      groupingsNew[value] = new Object();
      groupingsNew[value].items = new Array();
    }

    if (current != "None")
      groupingsNew[value].items.push(item);
  }

  var html="";
  html += "<form style=\"margin: 0\" name=\"form\" action=\"\">";

  for (var group in groupingsNew) {
    if (!groupsChange) {
      if (typeof(this._groupings[group]) != 'undefined') {
        if (this._groupings[group].display)
          groupingsNew[group].display = true;
        else
          groupingsNew[group].display = false;
      }
    }

    html += "<input type=\"checkbox\" id=\"checkbox_" + group + "\" onClick=\"" + this._name + ".readCheckbox('" + group + "')\"";
    html += ">";

    if (groupingsNew[group].items.length == 0) {
      if (this._nodeActionFunctionName != "")
        html += "<a href=\"javascript:" + this._nodeActionFunctionName + "('" + group + "')\">";

      html += this._nodeActionHtml;

      if (this._nodeActionFunctionName != "")
        html += "</a>";

      if (typeof(this._items[group].nodeActionHtml) != 'undefined') {
        html += "&nbsp;" + this._items[group].nodeActionHtml;
      }
    }

    if (groupingsNew[group].items.length == 0) {
      if (this._nodeActionHtml != "")
        html += "&nbsp;";

      html += group + "<br>";
    } else {
      var numItems = groupingsNew[group].items.length;

      if (this._groupActionFunctioName != "") {
        html += "<a href=\"javascript:" + this._groupActionFunctionName + "([";
        for (var i=0; i < numItems; i++) {
          html += "'" + groupingsNew[group].items[i] + "'";
          if (i != numItems-1)
            html += ", ";
        }
        html += "])\">";
      }

      html += this._groupActionHtml;

      if (this._groupActionFunctioName != "")
        html += "<\a>";

      if (this._groupActionHtml != "")
        html += "&nbsp;";

      html += group + "<br>";

      for (var i=0; i < numItems; i++) {
        var nodeImage = this._nodeImageFilename;

        if (i == numItems-1)
          nodeImage = this._nodeLastImageFilename;

        html += "<img src=\"" + nodeImage + "\" align=\"middle\" vspace=\"0\" height=\"22\" width=\"16\" border=0>&nbsp;";

        if (this._nodeActionFunctionName != "") 
          html += "<a href=\"javascript:" + this._nodeActionFunctionName + "('" + groupingsNew[group].items[i] + "')\">";

        html += this._nodeActionHtml;

        if (this._nodeActionFunctionName != "")
          html += "</a>";

        if (typeof(this._items[groupingsNew[group].items[i]].nodeActionHtml) != 'undefined') {
          html += "&nbsp;" + this._items[groupingsNew[group].items[i]].nodeActionHtml;
        }

        if (this._nodeActionHtml != "")
          html += "&nbsp;";

        html += groupingsNew[group].items[i] + "<br>";
      }
    }
  }
  html += "</form>";

  if (this._previousItemCount == 0 && itemCount > 0)
    groupsChange = true;

  if (groupsChange) {
    if (typeof(this._showDefaults) != "undefined") {
      for (var group in groupingsNew)
        groupingsNew[group].display = false;

      if (typeof(this._showDefaults[current]) != "undefined") {
        for (var j=0; j < this._showDefaults[current].length; j++) {
          var gr = this._showDefaults[current][j];
          if (typeof(groupingsNew[gr]) != "undefined")
            groupingsNew[gr].display = true;
        }
      }
    } else {
      for (var group in groupingsNew)
        groupingsNew[group].display = true;
    }
  }

  // copy new groupings across
  this._groupings = groupingsNew;

  for (var group in this._groupings) {
    this.updateItemDisplays(group);
  }
  document.getElementById("display_nodes").innerHTML = html;
  this.writeCheckboxes();

  this._updateDisplayFunction();
  this._previousItemCount = itemCount;
}
