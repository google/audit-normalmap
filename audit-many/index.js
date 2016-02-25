/**
 * @license
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @fileoverview Viewer for normalmap audits.
 */

var rows = [];
var reports = {};
var fields = {};

var hide_fields = {
  // Input fields we don't show.
  "image": true,
  "output_channel_r": true,
  "output_channel_g": true,
  "output_channel_b": true,

  // Our own fields.
  "height_name": true,
  "height_name_t": true,
  "norm_name": true,
  "norm_name_t": true,
  "output_name": true,
  "output_name_t": true,
};

function addRow(filename, prefix, report) {
  report["norm_name"] = prefix + "-norm.png";
  report["norm_name_t"] = prefix + "-norm-t.png";
  report["height_name"] = prefix + "-height.png";
  report["height_name_t"] = prefix + "-height-t.png";
  report["output_name"] = prefix + "-report.png";
  report["output_name_t"] = prefix + "-report-t.png";
  for (var field in report) {
    if (!hide_fields[field] && !field.startsWith("error_")) {
      fields[field] = null;
    }
  }
  rows.push(filename);
  reports[filename] = report;
}

function formatValue(x) {
  if (x == null) {
    return "";
  }
  if (x === true) {
    return "x";
  }
  if (typeof x == "number") {
    return x.toPrecision(3);
  }
  return x;
}

function displayRows() {
  var headersRow = document.getElementById("headers");
  var reportTable = document.getElementById("report");
  var sortedFields = Object.keys(fields).sort();

  // Static fields.
  var th = document.createElement("th");
  th.appendChild(document.createTextNode("filename"));
  headersRow.appendChild(th);

  var th = document.createElement("th");
  th.appendChild(document.createTextNode("normalmap"));
  headersRow.appendChild(th);

  var th = document.createElement("th");
  th.appendChild(document.createTextNode("heightmap"));
  headersRow.appendChild(th);

  var th = document.createElement("th");
  th.appendChild(document.createTextNode("report"));
  headersRow.appendChild(th);

  var th = document.createElement("th");
  th.appendChild(document.createTextNode("errors"));
  headersRow.appendChild(th);

  // Report fields.
  sortedFields.forEach(function(field) {
    var th = document.createElement("th");
    th.appendChild(document.createTextNode(field));
    headersRow.appendChild(th);
  });

  rows.forEach(function(filename) {
    var report = reports[filename];
    var tr = document.createElement("tr");

    // Static fields.
    var th = document.createElement("th");
    th.appendChild(document.createTextNode(filename));
    tr.appendChild(th);

    var td = document.createElement("td");
    var a = document.createElement("a");
    a.href = report["norm_name"];
    var img = document.createElement("img");
    img.src = report["norm_name_t"];
    a.appendChild(img);
    td.appendChild(a);
    tr.appendChild(td);

    var td = document.createElement("td");
    var a = document.createElement("a");
    a.href = report["height_name"];
    var img = document.createElement("img");
    img.src = report["height_name_t"];
    a.appendChild(img);
    td.appendChild(a);
    tr.appendChild(td);

    var td = document.createElement("td");
    var a = document.createElement("a");
    a.href = report["output_name"];
    var img = document.createElement("img");
    img.src = report["output_name_t"];
    a.appendChild(img);
    td.appendChild(a);
    tr.appendChild(td);

    var errors = "";
    for (var field in report) {
      if (field.startsWith("error_")) {
        if (errors != "") {
          errors += "\n";
        }
        errors += field.substr(6);
      }
    }
    var td = document.createElement("td");
    td.appendChild(document.createTextNode(errors));
    tr.appendChild(td);

    // Report fields.
    sortedFields.forEach(function(field) {
      var td = document.createElement("td");
      td.appendChild(document.createTextNode(formatValue(report[field])));
      tr.appendChild(td);
    });

    reportTable.appendChild(tr);
  });
}
