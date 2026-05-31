# this script hopefully displays the "downloading firmware" message on the device and preserves climate history between uploads.

import serial
import time
from platformio import util

Import("env") 

def before_upload(source, target, env):
    port = env.GetProjectOption("upload_port") or env.GetProjectOption("monitor_port")
    baud = 115200 

    if not port:
        print("No serial port specified, skipping pre-upload message.")
        return

    try:
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = 115200
        ser.timeout = 1
        ser.setDTR(False)
        ser.open()
        time.sleep(1.5) 
        ser.write(b'\xC1')
        ser.flush()
        time.sleep(0.5)
        ser.close()
        print(f"Sent pre-upload message to {port}")
        time.sleep(2.0)
    except Exception as e:
        print(f"Could not send pre-upload message: {e}")

env.AddPreAction("upload", before_upload)
