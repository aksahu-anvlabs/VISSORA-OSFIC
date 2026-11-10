var map = null;
var busIndex = 0;
var reachIndex = 0;

var reqData = true;
var locInterval = null;
var unixTime = 0;

var points;
var stopData = {};
var week;
var src = "Khordha";
var dst = "Bhubaneswar";

var marker = null;


function addPointer(lat, lng) {
    clearPointer();

    if (!Number.isFinite(Number(lat)) || !Number.isFinite(Number(lng))) {
        console.error("Invalid coordinates:", lat, lng);
        return;
    }

    const icon = L.divIcon({
        className: "pulse-icon",
        html: '<div class="pulse-marker"></div>',
        iconSize: [18, 18],
        iconAnchor: [9, 9]
    });

    marker = L.marker([Number(lat), Number(lng)], {
        icon: icon,
        zIndexOffset: 1000
    }).addTo(map);
}

function clearPointer() {
    if (marker) {
        marker.remove();
        marker = null;
    }
}

function $(val){
	return document.querySelector(val);
}

const busData = {
	"Dash Maa Laxmi Bus": ["Khordha", "Bhubaneswar"],
	"Kumar's Western AC Couch Bus": ["Khordha", "Puri"],
	"Sidhibinayak Bus": ["Khordha", "Bhubaneswar"],
};

function getWeekDates(){
	var monday = 0;
	monday = new Date(unixTime);
	week = Array.from({ length: 7 }, (_, i) => {
	    const date = new Date(monday);
	    date.setDate(monday.getDate() + i);
	    return date.toLocaleDateString("en-GB").replaceAll("/", "-")
	});
}

function inRange(lat1, lon1, lat2, lon2, range) {
    const R = 6371000; // Earth radius in meters

    const toRad = deg => deg * Math.PI / 180;

    const dLat = toRad(lat2 - lat1);
    const dLon = toRad(lon2 - lon1);

    const a =
        Math.sin(dLat / 2) ** 2 +
        Math.cos(toRad(lat1)) *
        Math.cos(toRad(lat2)) *
        Math.sin(dLon / 2) ** 2;

    const distance = 2 * R * Math.atan2(
        Math.sqrt(a),
        Math.sqrt(1 - a)
    );

    return distance < range;
}

function loadPoints(){
	$(".line").innerHTML = "";
	let point = document.createElement("div");
	point.setAttribute("class", "point");

	let bar = document.createElement("div");
	bar.setAttribute("class", "bar");

	let textArea = document.createElement("p");
	textArea.setAttribute("class", "small_text");

	Object.keys(points).forEach((key, index) => {
		let text = textArea.cloneNode(true);
		let circle = point.cloneNode(true);
		let space = bar.cloneNode(true);
		if(index < Object.keys(points).length - 1 && index >= reachIndex){
			space.style.boxShadow = "0px 0px transparent";
			space.style.background = "transparent";
		}
		if(Object.values(points)[index][1] == false){
			circle.style.border = "0px";
			circle.style.background = "#b2d9ff";
		}
		text.innerHTML = key;
		circle.appendChild(text);
		$(".line").appendChild(circle);

		if(index != Object.keys(points).length - 1){
			$(".line").appendChild(space);
		}
	});
}

function assignData(){
	getWeekDates();
	let usage = $(".usage");
	for(let i = 0; i < 9; i++){
		for(let j = 1; j <= 6; j++){
			usage.children[i].children[j].innerHTML = "";
			usage.children[i].children[j].style.backgroundColor = "transparent";
		}
	}
	let stops = Object.keys(points);
	let sums = Array(7).fill(0);
	stops.forEach((key, index) => {
		usage.children[0].children[index + 1].innerHTML = key;
	});
	if("data" in stopData){
		Object.keys(stopData["data"]).forEach((key) => {
			let dt = stopData["data"][key];
			for(let z = 0; z < Object.keys(dt).length; z++){
				let j = week.indexOf(key);
				if(j != -1){
					let k = Object.keys(points).indexOf(Object.keys(dt)[z]);
					if(k != -1){
						let num = Object.values(dt)[z];
						usage.children[j + 1].children[k + 1].innerHTML = `<strong><sup>${num[2]}</sup>&frasl;<sub>${num[3]}</sub></strong><div class="flex"><p>+${num[0]}</p>&nbsp;<p>-${num[1]}</p></div>`;
						sums[k] += num[2];
						let opacity = num[2]/45 - 0.2;
						usage.children[j + 1].children[k + 1].style.backgroundColor = `rgba(30, 144, 255, ${opacity})`;
					}
				}
			}
		});
	}
	for(let i = 0; i < 6; i++){
		usage.children[8].children[i + 1].innerHTML = `<big>${sums[i]}<big>`;
	}
}

function fetchInfo(){
	fetch(`http://127.0.0.1:8080/fetch_info?index=${busIndex}&time=${unixTime/1000}&info=${reqData}`)
	.then(response => {
	    if (!response.ok) {
	      throw new Error(`HTTP error! Status: ${response.status}`);
	    }
	    return response.json();
	})
	.then(data => {
		stopData = data;
		addPointer(data["_lat"], data["_lon"]);
		for(let i = reachIndex; i < Object.keys(points).length; i++){
			if(inRange(data["_lat"], data["_lon"], Object.values(points)[i][0], Object.values(points)[i][1], 50)){
				reachIndex = i;
				break;
			}
		}
		assignData();
		loadPoints();
	})
	.catch(error => console.error('Fetch error:', error));
}

function setData(data){
	points = data;
	fetchInfo();
	locInterval = setInterval(() => {
		//fetchInfo();
	}, 5000);
}

function sortBuses(){
	$("#buses").innerHTML = "";
	Object.keys(busData).forEach((key) => {
		if(busData[key][0] == src && busData[key][1] == dst && key.toLowerCase().indexOf($("#busName").value.toLowerCase()) != -1){
			let opt = document.createElement("option");
			opt.innerHTML = key;
			$("#buses").appendChild(opt);
		}
	});
}

function onStart(){
	map = L.map('map').setView([20.2961, 85.8245], 13);
	L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
	    maxZoom: 19,
	    attribution: '&copy; OpenStreetMap'
	}).addTo(map);
	L.marker([20.2961, 85.8245]).addTo(map).bindPopup('Bhubaneswar').openPopup();
	let today = new Date();
	let day = today.getDay();
	monday = new Date(today);
	monday.setDate(today.getDate() - (day === 0 ? 6 : day - 1));
	unixTime = monday.getTime();

	$("#src").addEventListener("change", (e) => {
		src = e.target.value;
		sortBuses();
	});

	$("#dst").addEventListener("change", (e) => {
		dst = e.target.value;
		sortBuses();
	});

	$("#busName").addEventListener("input", (e) => {
		sortBuses();
	});

	Object.keys(busData).forEach((key) => {
		if(busData[key][0] == src && busData[key][1] == dst){
			let opt = document.createElement("option");
			opt.innerHTML = key;
			$("#buses").appendChild(opt);
		}
	});

	$("#buses").addEventListener("change", (e) => {
		$("#busName").value = e.target.value;
		busIndex = Object.keys(busData).indexOf(e.target.value);
		fetch('http://127.0.0.1:8080/fetch_stops?index=' + busIndex)
		.then(response => {
		    if (!response.ok) {
		      throw new Error(`HTTP error! Status: ${response.status}`);
		    }
		    return response.json();
		})
		.then(data => {
			reachIndex = 0;
			setData(data);
			$("#bus_name").innerHTML = e.target.value;
			$("#src_btn").innerHTML = busData[e.target.value][0];
			$("#dst_btn").innerHTML = busData[e.target.value][1];
			if(locInterval != null){
				clearInterval(locInterval);
			}
		})
		.catch(error => console.error('Fetch error:', error));
		reachIndex = 0;
	});

	$("#buses").selectedIndex = -1;

	$('input[type="week"]').addEventListener("change", (e) => {
	    const [year, week] = e.target.value.split("-W").map(Number);

	    const jan4 = new Date(Date.UTC(year, 0, 4));
	    const monday = new Date(jan4);

	    monday.setUTCDate(jan4.getUTCDate() - ((jan4.getUTCDay() + 6) % 7) + (week - 1) * 7);
	    unixTime = monday.getTime();
	    fetchInfo();
	});
}

window.addEventListener("resize", () => {
	if (map) {
		map.invalidateSize();
	}
});