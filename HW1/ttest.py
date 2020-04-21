import signal
import psutil
import sys
import time
import os

def getProcess(name):
    for p in psutil.process_iter(attrs=['name']):
        try:
            if name == p.info['name']:
                return p
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return None

smash = getProcess("smash")
assert smash != None

#file_name = "test" + sys.argv[1] +".in"
file_name = "test15.in"

with open(file_name) as in_file:
    lines = in_file.read().splitlines(True)

for line in lines:
    if line.startswith("CtrlZ"):
        smash.send_signal(signal.SIGTSTP)
        time.sleep(1)
        continue
    if line.startswith("CtrlC"):
        smash.send_signal(signal.SIGINT)
        time.sleep(1)
        continue
    sys.stdout.write(line)
    sys.stdout.flush()
    time.sleep(1)
    
in_file.close()
time.sleep(2)
