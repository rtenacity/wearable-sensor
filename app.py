# Flask app for Flask + Firebase + Arduino
# Coded By:  
# Filename:  app.py

from flask import Flask, render_template, url_for, request, jsonify
from datetime import datetime
# import pyrebase

app = Flask(__name__)

config = {}

key = 0
firebase = None
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/register')
def register():
    return render_template('register.html')

@app.route('/signIn')
def signIn():
    return render_template('signIn.html')

@app.route('/home')
def home():
    return render_template('home.html')


key = 0
db = None
userID = None
timestamp = None

@app.route('/test', methods=['GET', 'POST'])
def test():
    
    global config, key, db, userID, timestamp
    
    print("Request Received", flush=True)

    config = request.get_json()
    print("Received Data:", config, flush=True)

    if 'userID' not in config.keys():
        return jsonify({"error": "Missing userID in request"}), 400

    userID = config.pop('userID')
    timestamp = datetime.now().strftime('%d-%m-%Y %H:%M:%S')
    capacitance = config.pop('capacitance')
    # firebase = pyrebase.initialize_app(config)
    db = firebase.database()

    # Log data to Firebase
    db.child(f'users/{userID}/data/{timestamp}').update({'capicitance': capacitance})

    return jsonify({"message": "Success"}), 200
    
if __name__ == '__main__':
    app.run(debug=True, host='192.168.208.71', port=50000)