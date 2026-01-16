//document.getElementById("datetime").innerHTML = "WebSocket is not connected";

const websocket = new WebSocket('ws://'+location.hostname+'/');
const map = L.map('map');

function sendText(name) {
	console.log('sendText');
	var data = {};
	data["id"] = name;
	console.log('data=', data);
	json_data = JSON.stringify(data);
	console.log('json_data=' + json_data);
	websocket.send(json_data);
}

websocket.onopen = function(evt) {
	console.log('WebSocket connection opened');
	var data = {};
	data["id"] = "init";
	console.log('data=', data);
	json_data = JSON.stringify(data);
	console.log('json_data=' + json_data);
	websocket.send(json_data);
	//document.getElementById("datetime").innerHTML = "WebSocket is connected!";
}

websocket.onmessage = function(evt) {
	var msg = evt.data;
	console.log("msg=" + msg);
	var values = msg.split('\4'); // \4 is EOT
	//console.log("values=" + values);
	switch(values[0]) {
		case 'HEAD':
			console.log("HEAD values[1]=" + values[1]);
			break;

		case 'DATA':
			console.log("DATA values[1]=" + values[1]);
			console.log("DATA values[2]=" + values[2]);
			console.log("DATA values[3]=" + values[3]);
			console.log("DATA values[4]=" + values[4]);
			console.log("DATA values[5]=" + values[5]);

			// Convert sexagesimal to decimal
			// 30 --> 0.5
			var lat = parseFloat(values[2]);
			lat = lat / 60.0;
			lat = parseInt(values[1],10) + lat;
			console.log("DATA lat=" + lat);
			var lon = parseFloat(values[4]);
			lon = lon / 60.0;
			lon = parseInt(values[3],10) + lon;
			console.log("DATA lon=" + lon);
			map.setView([lat, lon], 15);

			const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
				maxZoom: 19,
				attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
			}).addTo(map);

			const marker = L.marker([lat, lon]).addTo(map)
				.bindPopup('<b>Hello world!</b><br />You are here.').openPopup();

			// Enable fullscreen control
			// https://github.com/Leaflet/Leaflet.fullscreen
			if ((values[5] & 0x01) == 0x01) {
				map.addControl(new L.Control.Fullscreen());
			}

			// Disable zoom function
			// https://stackoverflow.com/questions/16537326/leafletjs-how-to-remove-the-zoom-control
			//map.removeControl(map.zoomControl);
			if ((values[5] & 0x02) == 0x02) {
				map.zoomControl.remove();
				map.scrollWheelZoom.disable();
				map.doubleClickZoom.disable();
				map.touchZoom.disable();
				map.boxZoom.disable();
			}

			break;

		default:
			break;
	}
}

websocket.onclose = function(evt) {
	console.log('Websocket connection closed');
	//document.getElementById("datetime").innerHTML = "WebSocket closed";
}

websocket.onerror = function(evt) {
	console.log('Websocket error: ' + evt);
	//document.getElementById("datetime").innerHTML = "WebSocket error????!!!1!!";
}
