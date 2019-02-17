#!/usr/bin/env python

import json
from flask import Flask, render_template, request

app = Flask(__name__)
queue1 = 0
queue2 = 0

@app.route("/size")
def size():
    data = {"queue1": queue1, "queue2": queue2,}
    return json.dumps(data)

@app.route("/take")
def take():
    global queue1
    global queue2
    q1 = request.args.get('queue1')
    if not q1 == None: queue1 = queue1 - int(q1)
    if queue1 < 0: queue1 = 0
    q2 = request.args.get('queue2')
    if not q2 == None: queue2 = queue2 - int(q2)
    if queue2 < 0: queue2 = 0
    return json.dumps({"queue1": queue1, "queue2": queue2,})

@app.route("/put")
def put():
    global queue1
    global queue2
    q1 = request.args.get('queue1')
    if not q1 == None: queue1 = queue1 + int(q1)
    q2 = request.args.get('queue2')
    if not q2 == None: queue2 = queue2 + int(q2)
    return json.dumps({"queue1": queue1, "queue2": queue2,})

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8000)
