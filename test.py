# app.py (Flask + Flask-SocketIO + Arduino Live Graph)

from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
from datetime import datetime

app = Flask(__name__)
socketio = SocketIO(app)

@app.route('/')
def index():
    return render_template('graph.html')

@app.route('/test', methods=['GET', 'POST'])
def test():
    if request.method == 'GET':
        keys = request.args.to_dict()
        value = keys.get('capacitance')

        if value:
            print("value")
            timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-3]  # Up to milliseconds
            socketio.emit('new_data', {'time': timestamp, 'value': value})
            print(f"Sent {value} at {timestamp}", flush=True)
            
            with open('data_log.csv', 'a') as f:
                f.write(f"{timestamp},{value}\n")


        return jsonify({"message": "Success"}), 200

if __name__ == '__main__':
    socketio.run(app, debug=True, host='172.20.10.5', port=5000)