from flask import Flask
from datetime import datetime

app = Flask(__name__)

@app.route('/datetime', methods=['GET'])
def get_datetime():
    now = datetime.now()
    date_time_str = now.strftime("%d/%m/%Y %H:%M:%S")
    return date_time_str

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=8000)
