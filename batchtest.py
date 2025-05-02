from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime

app = Flask(__name__)
socketio = SocketIO(app)

@app.route('/')
def index():
    return render_template('graph.html')

@app.route('/test', methods=['GET', 'POST'])
def test():
    # old single‑GET path left in for backwards compatibility:
    if request.method == 'GET':
        value = request.args.get('capacitance')
        if value:
            emit_and_log(value)
        return jsonify({"message": "Success"}), 200

    # NEW: batch POST
    if request.method == 'POST':
        data = request.get_json(silent=True)
        if not data or 'capacitances' not in data:
            return jsonify({"error": "Invalid payload"}), 400

        for val in data['capacitances']:
            emit_and_log(val)
        return jsonify({"message": f"Received {len(data['capacitances'])} readings"}), 200

def emit_and_log(value):
    # timestamp to millisecond precision
    timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]
    socketio.emit('new_data', {'time': timestamp, 'value': value})
    with open('data_log.csv', 'a') as f:
        f.write(f"{timestamp},{value}\n")
    print(f"Logged {value} @ {timestamp}", flush=True)

if __name__ == '__main__':
    socketio.run(app, debug=True, host='172.20.10.5', port=5000)
