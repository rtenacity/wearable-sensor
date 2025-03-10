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
    if request.method == 'GET': 

        print(request.args, flush=True)
        keys = request.args.to_dict()
        print(keys, flush=True)
        
        
        # print(request.args(), flush=True)
        
        value = keys.get('capacitance')
        print("Received Distance:", value, flush=True)
        
        
        return jsonify({"message": "Success"}), 200
    
if __name__ == '__main__':
    app.run(debug=True, host='172.20.10.5', port=5000)