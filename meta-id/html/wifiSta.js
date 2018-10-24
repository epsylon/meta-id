var currAp = "";
var blockScan = 0;

function createInputForAp(ap) {
  if (ap.essid=="" && ap.rssi==0) return;

  var input = e("input");
  input.type = "radio";
  input.name = "essid";
  input.value=ap.essid;
  input.id   = "opt-" + ap.essid;
  if (currAp == ap.essid) input.checked = "1";

  var bars    = e("div");
  var rssiVal = -Math.floor(ap.rssi/51)*32;
  bars.className = "lock-icon";
  bars.style.backgroundPosition = "0px "+(rssiVal-1)+"px";

  var rssi = e("div");
  rssi.innerHTML = "" + ap.rssi +"dB";

  var encrypt = e("div");
  var encVal  = "-65"; //assume wpa/wpa2
  if (ap.enc == "0") encVal = "0"; //open
  if (ap.enc == "1") encVal = "-33"; //wep
  encrypt.className = "lock-icon";
  encrypt.style.backgroundPosition = "-32px "+encVal+"px";

  var label = e("div");
  label.innerHTML = ap.essid;

  var div = m('<label for=\"opt-' + ap.essid + '"></label>').childNodes[0];
  div.appendChild(input);
  div.appendChild(encrypt);
  div.appendChild(bars);
  div.appendChild(rssi);
  div.appendChild(label);
  return div;
}

function getSelectedEssid() {
  var e = document.forms.wifiform.elements;
  for (var i=0; i<e.length; i++) {
    if (e[i].type == "radio" && e[i].checked) {
      var v = e[i].value;
      if (v == "_hidden_ssid_") v = $("#hidden-ssid").value;
      return v;
    }
  }
  return currAp;
}

var scanTimeout = null;
var scanReqCnt = 0;

function scanResult() {
  if (scanReqCnt > 60) {
    return scanAPs();
  }
  scanReqCnt += 1;
  ajaxJson('GET', "/wifi/scan", function(data) {
      currAp = getSelectedEssid();
      if (data.result.inProgress == "0" && data.result.APs.length > 0) {
        $("#aps").innerHTML = "";
        var n = 0;
        for (var i=0; i<data.result.APs.length; i++) {
          if (data.result.APs[i].essid == "" && data.result.APs[i].rssi == 0) continue;
          if($("#opt-"+data.result.APs[i].essid) != null) continue; // skip duplicate essids
          $("#aps").appendChild(createInputForAp(data.result.APs[i]));
          n = n+1;
        }
        showNotification("Scan found " + n + " networks");
        var cb = $("#connect-button");
        cb.className = cb.className.replace(" pure-button-disabled", "");
        if (scanTimeout != null) clearTimeout(scanTimeout);
//        scanTimeout = window.setTimeout(scanAPs, 20000);
      } else {
        window.setTimeout(scanResult, 1000);
      }
    }, function(s, st) {
      window.setTimeout(scanResult, 5000);
  });
}

function scanAPs() {
//  console.log("scanning now");
  if (blockScan) {
    scanTimeout = window.setTimeout(scanAPs, 1000);
    return;
  }
  scanTimeout = null;
  scanReqCnt = 0;
  ajaxReq('POST', "/wifi/scan", function(data) {
    showNotification("Wifi scan started");
    window.setTimeout(scanResult, 1000);
  }, function(s, st) {
    if (s == 400) {
      showWarning("Cannot scan in AP mode");
      $("#aps").innerHTML =
        "Switch to <a href=\"#\" onclick=\"changeWifiMode(3)\">STA+AP mode</a> to scan.";
    } else showWarning("Failed to scan: " + st);
    //window.setTimeout(scanResult, 1000);
  });
}

function getStatus() {
  ajaxJsonSpin("GET", "/wifi/connstatus", function(data) {
      if (data.status == "idle" || data.status == "connecting") {
        $("#aps").innerHTML = "Connecting...";
        showNotification("Connecting...");
        window.setTimeout(getStatus, 1000);
      } else if (data.status == "got IP address") {
        var txt = "Connected! Got IP "+data.ip;
        showNotification(txt);
        showWifiInfo(data);
        blockScan = 0;
        if (data.modechange == "yes") {
          var txt2 = "meta-id will switch to STA-only mode in a few seconds";
          window.setTimeout(function() { showNotification(txt2); }, 4000);
        }
        $("#reconnect").removeAttribute("hidden");
        $("#reconnect").innerHTML =
          "If you are in the same network, go to <a href=\"http://"+data.ip+
          "/\">"+data.ip+"</a>, else connect to network "+data.ssid+" first.";
      } else {
        blockScan = 0;
        showWarning("Connection failed: " + data.status + ", " + data.reason);
        $("#aps").innerHTML =
          "Check password and selected AP. <a href=\"wifi.tpl\">Go Back</a>";
      }
    }, function(s, st) {
      //showWarning("Can't get status: " + st);
      window.setTimeout(getStatus, 2000);
    });
}

function changeWifiMode(m) {
  blockScan = 1;
  hideWarning();
  ajaxSpin("POST", "/wifi/setmode?mode=" + m, function(resp) {
    showNotification("Mode changed");
    window.setTimeout(getWifiInfo, 100);
    blockScan = 0;
    window.setTimeout(scanAPs, 500);
    $("#aps").innerHTML = 'Scanning... <div class="spinner spinner-small"></div>';
  }, function(s, st) {
    showWarning("Error changing mode: " + st);
    window.setTimeout(getWifiInfo, 100);
    blockScan = 0;
  });
}

function changeWifiAp(e) {
  e.preventDefault();
  var passwd = $("#wifi-passwd").value;
  var essid = getSelectedEssid();
  showNotification("Connecting to " + essid);
  var url = "/wifi/connect?essid="+encodeURIComponent(essid)+"&passwd="+encodeURIComponent(passwd);

  hideWarning();
  $("#reconnect").setAttribute("hidden", "");
  $("#wifi-passwd").value = "";
  var cb = $("#connect-button");
  var cn = cb.className;
  cb.className += ' pure-button-disabled';
  blockScan = 1;
  ajaxSpin("POST", url, function(resp) {
      $("#spinner").removeAttribute('hidden'); // hack
      showNotification("Waiting for network change...");
      window.scrollTo(0, 0);
      window.setTimeout(getStatus, 2000);
    }, function(s, st) {
      showWarning("Error switching network: "+st);
      cb.className = cn;
      window.setTimeout(scanAPs, 1000);
    });
}

function changeSpecial(e) {
  e.preventDefault();
  var url = "/wifi/special";
  url += "?dhcp=" + document.querySelector('input[name="dhcp"]:checked').value;
  url += "&staticip=" + encodeURIComponent($("#wifi-staticip").value);
  url += "&netmask=" + encodeURIComponent($("#wifi-netmask").value);
  url += "&gateway=" + encodeURIComponent($("#wifi-gateway").value);

  hideWarning();
  var cb = $("#special-button");
  addClass(cb, 'pure-button-disabled');
  ajaxSpin("POST", url, function(resp) {
      removeClass(cb, 'pure-button-disabled');
      //getWifiInfo(); // it takes 1 second for new settings to be applied
    }, function(s, st) {
      showWarning("Error: "+st);
      removeClass(cb, 'pure-button-disabled');
      getWifiInfo();
    });
}

function doDhcp() {
  $('#dhcp-on').removeAttribute('hidden');
  $('#dhcp-off').setAttribute('hidden', '');
}

function doStatic() {
  $('#dhcp-off').removeAttribute('hidden');
  $('#dhcp-on').setAttribute('hidden', '');
}

//===== MQTT cards

function changeMqtt(e) {
  e.preventDefault();
  var url = "mqtt?1=1";
  var i, inputs = document.querySelectorAll('#mqtt-form input');
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type != "checkbox")
      url += "&" + inputs[i].name + "=" + inputs[i].value;
  };

  hideWarning();
  var cb = $("#mqtt-button");
  addClass(cb, 'pure-button-disabled');
  ajaxSpin("POST", url, function (resp) {
    showNotification("MQTT updated");
    removeClass(cb, 'pure-button-disabled');
  }, function (s, st) {
    showWarning("Error: " + st);
    removeClass(cb, 'pure-button-disabled');
    window.setTimeout(fetchMqtt, 100);
  });
}

function displayMqtt(data) {
  Object.keys(data).forEach(function (v) {
    el = $("#" + v);
    if (el != null) {
      if (el.nodeName === "INPUT") el.value = data[v];
      else el.innerHTML = data[v];
      return;
    }
    el = document.querySelector('input[name="' + v + '"]');
    if (el != null) {
      if (el.type == "checkbox") el.checked = data[v] > 0;
      else el.value = data[v];
    }
  });

  var i, inputs = $("input");
  for (i = 0; i < inputs.length; i++) {
    if (inputs[i].type == "checkbox")
      inputs[i].onclick = function () { setMqtt(this.name, this.checked) };
  }
}

function fetchMqtt() {
  ajaxJson("GET", "/mqtt", displayMqtt, function () {
    window.setTimeout(fetchMqtt, 1000);
  });
}

function changeMqttStatus(e) {
  e.preventDefault();
  var v = document.querySelector('input[name="mqtt-status-topic"]').value;
  ajaxSpin("POST", "/mqtt?mqtt-status-topic=" + v, function () {
    showNotification("MQTT status settings updated");
  }, function (s, st) {
    showWarning("Error: " + st);
    window.setTimeout(fetchMqtt, 100);
  });
}

function changeMqttTest(e) {
  e.preventDefault();
  var v = document.querySelector('input[name="mqtt-test-topic"]').value;
  ajaxSpin("POST", "/mqtt?mqtt-test-topic=" + v, function () {
    showNotification("MQTT test settings updated");
  }, function (s, st) {
    showWarning("Error: " + st);
    window.setTimeout(fetchMqtt, 100);
  });
}

function setMqtt(name, v) {
  ajaxSpin("POST", "/mqtt?" + name + "=" + (v ? 1 : 0), function () {
    var n = name.replace("-enable", "");
    showNotification(n + " is now " + (v ? "enabled" : "disabled"));
  }, function () {
    showWarning("Enable/disable failed");
    window.setTimeout(fetchMqtt, 100);
  });
}