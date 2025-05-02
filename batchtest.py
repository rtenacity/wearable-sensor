from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
from datetime import datetime
from collections import deque
import numpy as np
from threading import Thread
import time

app = Flask(__name__)
socketio = SocketIO(app)

# In‑memory buffer of (timestamp, value) for last ~10 s
buffer = deque()
MAX_SAMPLES = 2000  # cap absolute size to avoid memory blow‑up

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
    now = datetime.now()
    timestamp = now.strftime('%H:%M:%S.%f')[:-3]

    # emit to clients
    socketio.emit('new_data', {'time': timestamp, 'value': value})

    # append to log file
    with open('data_log.csv', 'a') as f:
        f.write(f"{timestamp},{value}\n")

    print(f"Logged {value} @ {timestamp}", flush=True)

    # also append into our in‑memory buffer
    t_secs = now.timestamp()
    buffer.append((t_secs, float(value)))
    # drop data older than ~11 s
    while buffer and (t_secs - buffer[0][0] > 11):
        buffer.popleft()
    # cap absolute buffer length
    if len(buffer) > MAX_SAMPLES:
        buffer.popleft()

def blink_rate_worker():
    """Background thread: every second, compute FFT over last 10 s and emit blink_rate."""
    while True:
        time.sleep(1)
        # require at least some data
        if len(buffer) < 10:
            continue

        # slice last 10 s
        times, vals = zip(*buffer)
        cutoff = times[-1] - 10
        sel = [(t, v) for t, v in zip(times, vals) if t >= cutoff]
        if len(sel) < 2:
            continue

        t_arr = np.array([t for t, _ in sel])
        v_arr = np.array([v for _, v in sel])

        # estimate uniform sample spacing
        dt = np.mean(np.diff(t_arr))
        # zero‑mean
        sig = v_arr - np.mean(v_arr)

        # FFT (real‐valued)
        fft_res = np.fft.rfft(sig)
        freqs = np.fft.rfftfreq(len(sig), d=dt)

        # ignore DC (index 0), find peak
        idx = np.argmax(np.abs(fft_res)[1:]) + 1
        peak_hz = freqs[idx]

        # convert to blinks per minute
        blink_per_min = peak_hz * 60.0

        # emit to clients
        socketio.emit('blink_rate', {'rate': round(blink_per_min, 1)})

if __name__ == '__main__':
    # start background FFT thread
    thread = Thread(target=blink_rate_worker, daemon=True)
    thread.start()

    socketio.run(app, debug=True, host='172.20.10.5', port=5000)
