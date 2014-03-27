#!/usr/bin/env python3
# Simple manual IPC client for debugging

import os
import socket
import threading

display = os.environ["DISPLAY"] if "DISPLAY" in os.environ else ""
redshift_id = os.environ["REDSHIFT_ID"] if "REDSHIFT_ID" in os.environ else ""
path = "/dev/shm/.redshift-socket-%i-%s:%s" % (os.getuid(), display, redshift_id)

socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
socket.connect(path)

def recv_loop():
    got = socket.recv(1024)
    if len(got) > 0:
        print(got.decode("utf-8"));

thread = threading.Thread(target = recv_loop)
thread.setDaemon(True)
thread.start()

while True:
    line = input()
    if line == "":
        break
    socket.send((line + "\n").encode("utf-8"))

socket.close()

