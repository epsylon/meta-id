function displayMeta(data) {
  Object.keys(data).forEach(function (v) {
    el = $("#" + v);
    if (el != null) {
      if (el.nodeName === "DIV"){
		var toouthigh=true;
		var tooutlow=true;
		var toin=true;
			el.innerHTML = "BLANK";
		if (data[v]==-2){
			tooutlow=false;
			el.innerHTML = "OUT LOW";
			}
		if (data[v]==-1){
			toouthigh=false;
			el.innerHTML = "OUT HIGH";
			}
		if (data[v]==0){
			el.innerHTML = "not configured";
			}
		if (data[v]>0){
			toin=false;
			el.innerHTML = "IN "+data[v];
			}
		if(toin){
			el.innerHTML+=" | <input type='button' value='TO INPUT' onclick='setMeta(\""+v.substr(-2)+"\",\"1\")'/>";
		}
		if(tooutlow){
			el.innerHTML+=" | <input type='button' value='TO OUTPUT LOW' onclick='setMeta(\""+v.substr(-2)+"\",\"-2\")'/>";
		}
		if(toouthigh){
			el.innerHTML+=" | <input type='button' value='TO OUTPUT HIGH' onclick='setMeta(\""+v.substr(-2)+"\",\"-1\")'/>";
		}
    }}});

}

function toggle(el){
	if(!$('#'+el).hasAttribute("hidden")){
  $("#"+el).setAttribute("hidden", "");}
  else{$("#meta-form").removeAttribute("hidden");}
}
	
function fetchMeta() {
	console.log("fetchMeta !");
  ajaxJson("GET", "/meta/gpio", displayMeta, function () {
    window.setTimeout(fetchMeta, 1000);
  });
}

function changeMetaStatus(e) {
  e.preventDefault();
  var v = document.querySelector('input[name="meta-status-topic"]').value;
  ajaxSpin("POST", "/meta?meta-status-topic=" + v, function () {
    showNotification("meta status settings updated");
  }, function (s, st) {
    showWarning("Error: " + st);
    window.setTimeout(fetchMeta, 100);
  });
}

function setMeta(name, v) {
  ajaxSpin("POST", "/meta/gpio?num=" + name+"&v=" + v, function () {
    var n = name.replace("-enable", "");
    showNotification(n + " is now " + (v ? "enabled" : "disabled"));
    window.setTimeout(fetchMeta, 100);
  }, function () {
    showWarning("Enable/disable failed");
    window.setTimeout(fetchMeta, 100);
  });
}
