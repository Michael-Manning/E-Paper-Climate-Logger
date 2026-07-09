# this script hopefully displays the "downloading firmware" message on the device and preserves climate history between uploads.

import serial
import time
from platformio import util

Import("env") 

def before_upload(source, target, env):
    # Try project-defined port first so an explicit config still works.
    port = env.GetProjectOption("upload_port") or env.GetProjectOption("monitor_port")

    if not port:
        # Auto-detect the ESP32-S3 built-in USB CDC interface (VID 0x303A, PID 0x1001).
        from serial.tools import list_ports
        for p in list_ports.comports():
            if p.vid == 0x303A and p.pid == 0x1001:
                port = p.device
                break

    if not port:
        print("No serial port found, skipping pre-upload message.")
        return

    try:
        ser = serial.Serial()
        ser.port = port
        ser.baudrate = 115200
        ser.timeout = 1
        ser.setDTR(False)   # keep DTR de-asserted so opening the port does not reset the device
        ser.open()
        time.sleep(1.5)     # allow the device to finish any pending operation
        ser.write(b'\xC1')
        ser.flush()

        # Wait for the device to acknowledge. The firmware emits this line only after
        # EndRefresh() completes, so receiving it guarantees both the EEPROM write and
        # the display update are fully done. USB no longer suspends during the refresh
        # (displayBusyCallback skips light sleep when USB is connected), so the port
        # can safely stay open while we wait.
        print("Waiting for device to acknowledge...")
        deadline = time.time() + 6.0
        while time.time() < deadline:
            if b"Update notification received" in ser.readline():
                print(f"Device acknowledged. Proceeding with upload.")
                break
        else:
            print("Timeout waiting for acknowledgment. Proceeding anyway.")

        ser.setDTR(False)   # keep DTR low; asserting it resets the device
        ser.close()
    except Exception as e:
        print(f"Could not send pre-upload message: {e}")

env.AddPreAction("upload", before_upload)
