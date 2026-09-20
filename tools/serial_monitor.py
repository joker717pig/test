"""串口监听：读取 ESP32 console 输出并实时打印（用于捕获力度日志）。"""
import serial
import sys

PORT = "COM12"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)
print(f"[serial_monitor] listening on {PORT} @{BAUD}", flush=True)

while True:
    try:
        line = ser.readline()
        if line:
            sys.stdout.write(line.decode("utf-8", errors="ignore"))
            sys.stdout.flush()
    except KeyboardInterrupt:
        break
    except Exception as e:
        print(f"[serial_monitor] error: {e}", flush=True)
        break
