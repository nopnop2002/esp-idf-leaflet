//document.getElementById("datetime").innerHTML = "WebSocket is not connected";

const websocket = new WebSocket('ws://'+location.hostname+'/');
//const map = L.map('map');
const map = L.map('map', {messagebox: true});

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
			console.log("DATA values[1]=" + values[1]); // latitude(degrees)
			console.log("DATA values[2]=" + values[2]); // latitude(minutes)
			console.log("DATA values[3]=" + values[3]); // longitude(degrees)
			console.log("DATA values[4]=" + values[4]); // longitude(minutes)
			console.log("DATA values[5]=" + values[5]); // zoom level
			console.log("DATA values[6]=" + values[6]); // zoom option

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
			zoom = parseInt(values[5],10);
			map.setView([lat, lon], zoom);

			const tiles = L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
				maxZoom: 19,
				attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
			}).addTo(map);

			const marker = L.marker([lat, lon]).addTo(map)
				.bindPopup('<b>Hello world!</b><br />You are here.').openPopup();

			// Disable zoom function
			// https://stackoverflow.com/questions/16537326/leafletjs-how-to-remove-the-zoom-control
			//map.removeControl(map.zoomControl);
			if ((values[6] & 0x02) == 0x02) {
				map.zoomControl.remove();
				map.scrollWheelZoom.disable();
				map.doubleClickZoom.disable();
				map.touchZoom.disable();
				map.boxZoom.disable();
			}

			break;

		case 'ZOOMLEVEL':
			console.log("ZOOMLEVEL values[1]=" + values[1]); // zoom level
			zoom_level = parseInt(values[1],10);
			map.setZoom(zoom_level);

			break;

		case 'ZOOMCONTROL':
			console.log("ZOOMCONTROL values[1]=" + values[1]); // zoom control
			zoom_control = parseInt(values[1],10);
			if (zoom_control == 0) {
				map.zoomControl.remove();
				map.scrollWheelZoom.disable();
				map.doubleClickZoom.disable();
				map.touchZoom.disable();
				map.boxZoom.disable();
			} else {
				map.zoomControl.addTo(map);
				map.scrollWheelZoom.enable();
				map.doubleClickZoom.enable();
				map.touchZoom.enable();
				map.boxZoom.enable();
			}

			break;

		case 'FULLSCREEN':
			// Enable/Disable fullscreen control
			// https://github.com/Leaflet/Leaflet.fullscreen
			console.log("FULLSCREEN values[1]=" + values[1]);
			fullscreeen = parseInt(values[1],10);
			map.addControl(new L.Control.Fullscreen());

			break;

		case 'MESSAGEBOX':
			console.log("MESSAGEBOX values[1]=" + values[1]); // position
			console.log("MESSAGEBOX values[2]=" + values[2]); // timeout
			console.log("MESSAGEBOX values[3]=" + values[3]); // message
			//var message = '<p>messagebox</p>';
			//message += '<p><a href="https://github.com/tinuzz/leaflet-messagebox">GitHub Leaflet.Messagebox</a></p>';
			var message = values[3];

			map.messagebox.setPosition(values[1]);	// 'topleft', 'topright', 'bottomleft', 'bottomright'
			map.messagebox.options.timeout = parseInt(values[2],10); // default timeout 3000 millisecond
			map.messagebox.show(message);

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
