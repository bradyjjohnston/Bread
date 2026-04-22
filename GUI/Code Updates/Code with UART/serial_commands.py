import serial
import threading
import time

class STM32Serial:
    def __init__(self, port, baud=115200):
        self.ser = serial.Serial(port, baud, parity=serial.PARITY_NONE, timeout=1)
        time.sleep(2)
        self.ser.reset_input_buffer()
        self._running = True
        self._rx_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._rx_thread.start()

    def _read_loop(self):
        while self._running:
            try:
                line = self.ser.readline()  # blocks until \n received
                if line:
                    print(f"[RX RAW] {line.hex()}")
                    # print(f"[RX STR] {line.decode('utf-8', errors='replace').strip()}")
            except serial.SerialException as e:
                print(f"[ERROR] {e}")
                break
            # if self.ser.in_waiting:
            #     data = self.ser.read(self.ser.in_waiting)
            #     if data:
            #         print(f"[RX RAW] {data.hex()}")

    def send_packet(self, packet):
        raw = packet.to_bytes()
        self.ser.write(raw)
        print(f"[TX] {raw.hex()}")

    def close(self):
        self._running = False
        self.ser.close()

# Usage
# dev = STM32Serial("/dev/ttyACM0", 115200)
# import time; time.sleep(2)
# dev.send("ping")
# time.sleep(5)
# dev.close()