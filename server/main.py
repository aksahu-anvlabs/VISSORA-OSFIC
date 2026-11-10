from flask import Flask, request, jsonify
from datetime import datetime, timedelta
from flask_cors import CORS
import traceback
import time
import json

app = Flask(__name__)
CORS(app)

@app.route("/update")
def update():
	data = request.args.get('data', default="{}", type=str)
	stop = "Master Canteen" # sample example
	try:
		data = json.loads(data)
		with open(f"server/data/{data[0]}.json", "r") as f:
			record = json.loads(f.read())
		t_date = datetime.now().strftime("%d-%m-%Y")
		record["_lat"] = data[1]
		record["_lon"] = data[2]
		if t_date not in record:
			record[t_date] = {}
		if stop not in record[t_date]:
			record[t_date][stop] = [0] * 4
		record[t_date][stop][0] = data[3]
		record[t_date][stop][1] = data[4]
		record[t_date][stop][2] = data[5]
		record[t_date][stop][3] = 45

		with open(f"server/data/{data[0]}.json", "w") as f:
			f.write(json.dumps(record))
	except Exception:
		return traceback.format_exc()
		
	return "Done"

@app.route("/fetch_stops")
def fetch_stops():
	index = request.args.get('index', default="0", type=str)
	try:
		with open(f"server/stop_data/{index}.json", "r") as f:
			stops = f.read()
		return stops
	except Exception:
		return "{}"

@app.route("/fetch_info")
def fetch_info():
	index = request.args.get('index', default="0", type=str)
	t_stamp = request.args.get('time', default=int(time.time()), type=int)
	req_info = request.args.get('info', 'false').lower() == 'true'

	try:
		with open(f"server/data/{index}.json", "r") as f:
			record = json.loads(f.read())
		data = {"_lat": record["_lat"], "_lon": record["_lon"]}

		if req_info:
			dt = datetime.fromtimestamp(t_stamp)
			monday = dt - timedelta(days=dt.weekday())
			week_dates = [
			    (monday + timedelta(days=i)).strftime("%d-%m-%Y")
			    for i in range(7)
			]
			print(week_dates)
			info = {}
			for i in week_dates:
				if i in record:
					info[i] = record[i]
			data["data"] = info
		return jsonify(data)
	except Exception:
		return jsonify({"error": traceback.format_exc()})

app.run(debug=True, port=8080)