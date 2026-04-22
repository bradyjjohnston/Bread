import tkinter as tk
import datetime
import threading
import serial
import time
from serial_commands import STM32Serial
from Communication_Classes import *
import os
from tkinter import messagebox
LOG_DIR = "system_logs"
os.makedirs(LOG_DIR, exist_ok=True)
from tkinter import ttk
from tkinter import messagebox
from filterpy.kalman import KalmanFilter
import numpy as np

PORT = "/dev/ttyACM0"   # Windows: "COM3", Mac: "/dev/tcu.usbmodem..."
BAUD = 115200

running = True

root = tk.Tk()
root.title("Winter Automation GUI")

notebook = ttk.Notebook(root)
notebook.pack(expand=True, fill="both")

main_frame = tk.Frame(notebook)
logs_frame = tk.Frame(notebook)

notebook.add(main_frame, text="Main")
notebook.add(logs_frame, text="Logs")

# Logs tab UI
log_listbox = tk.Listbox(logs_frame, width=40)
log_listbox.pack(side=tk.LEFT, fill=tk.Y, padx=5, pady=5)

log_text = tk.Text(logs_frame, width=60)
log_text.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5, pady=5)

dev = STM32Serial(PORT, BAUD)

# Labels
weather_label = tk.Label(root, text="Weather: ---")
weather_label.pack()

prediction_label = tk.Label(root, text="Prediction: ---")
prediction_label.pack()

# Log box
log_box = tk.Text(root, height=10, width=50)
log_box.pack()

def update_weather():
    weather_label.config(text="Weather: 28°F, Light Snow")
    log("Weather updated.")

def update_prediction():
    prediction_label.config(text="Prediction: Salt spreading in 45 minutes.")
    log("Prediction updated.")

prediction_label = tk.Label(main_frame, text="Prediction: ---")
prediction_label.pack()

def spread_salt():
    pkt = Packet(Slice.ACTUATION, ActuationPortion.SALT_SPREADING, b'')
    dev.send_packet(pkt)
    log("Manual command: Spread Salt")

def boot_dryer():
    pkt = Packet(Slice.ACTUATION, ActuationPortion.BOOT_MECHANISM, b'')
    dev.send_packet(pkt)
    log("Manual command: Boot Dryer")

def stop_all():
    pkt = Packet(Slice.POWER, PowerPortion.ESTOP, b'')
    dev.send_packet(pkt)
    log("Manual command: STOP ALL")

def log(msg):
    timestamp = datetime.datetime.now().strftime("%H:%M:%S")
    full_msg = f"[{timestamp}] {msg}"

    # GUI log
    def append():
        log_box.insert(tk.END, full_msg + "\n")
        log_box.see(tk.END)

    root.after(0, append)

    # File log
    with open(get_log_filename(), "a") as f:
        f.write(full_msg + "\n")


def fake_mcu_listener():
    while True:
        time.sleep(5)  # every 5 seconds
        noisy_temp = 30 + np.random.rand() * 5
        update_prediction_with_kalman(noisy_temp)
#        log("MCU: Weather sensor heartbeat OK")
#        log("MCU: Actuator idle"

# Buttons
btn_weather = tk.Button(root, text="Test Weather Update", command=update_weather)
btn_weather.pack()

btn_predict = tk.Button(root, text="Test Prediction Update", command=update_prediction)
btn_predict.pack()

btn_salt = tk.Button(root, text="Spread Salt", command=spread_salt)
btn_salt.pack()

btn_dryer = tk.Button(root, text="Boot Dryer", command=boot_dryer)
btn_dryer.pack()

btn_stop = tk.Button(root, text="STOP ALL", command=stop_all)
btn_stop.pack()

# System Logs
def get_log_filename():
    date = datetime.datetime.now().strftime("%Y-%m-%d")
    return os.path.join(LOG_DIR, f"{date}.log")

# -----------------------------
# Text Input + Send Button
# -----------------------------
input_frame = tk.Frame(root)
input_frame.pack(pady=10)

command_entry = tk.Entry(input_frame, width=40)
command_entry.pack(side=tk.LEFT, padx=5)

def send_command():
    cmd = command_entry.get().strip()
    if cmd:
        log(f"User command sent: {cmd}")
        command_entry.delete(0, tk.END)

send_button = tk.Button(input_frame, text="Send", command=send_command)
send_button.pack(side=tk.LEFT)

# Camera feed placeholder
camera_frame = tk.Frame(root, width=320, height=240, bg="black")
camera_frame.pack(pady=10)

camera_label = tk.Label(camera_frame, text="Camera Feed Placeholder", fg="white", bg="black")
camera_label.place(relx=0.5, rely=0.5, anchor="center")

threading.Thread(target=fake_mcu_listener, daemon=True).start()

# -----------------------------
# Menu Bar
# -----------------------------
menubar = tk.Menu(root)

# File Menu
file_menu = tk.Menu(menubar, tearoff=0)
file_menu.add_command(label="Exit", command=root.quit)
menubar.add_cascade(label="File", menu=file_menu)

# Help Menu
help_menu = tk.Menu(menubar, tearoff=0)
help_menu.add_command(label="About", command=lambda: messagebox.showinfo(
    "About",
    "Winter Automation GUI\nVersion 0.1\nCreated by GUI Subteam"
))
menubar.add_cascade(label="Help", menu=help_menu)

root.config(menu=menubar)

# -----------------------------
# Simple 1D Kalman Filter for temperature smoothing
# -----------------------------
kf = KalmanFilter(dim_x=1, dim_z=1)

kf.F = np.array([[1]])   # state transition
kf.H = np.array([[1]])   # measurement function

kf.x = np.array([[0]])   # initial state estimate
kf.P = np.array([[1000]])  # initial uncertainty
kf.Q = np.array([[0.1]])   # process noise
kf.R = np.array([[2]])     # measurement noise

def preload_kalman_with_history():
    # Simulate last year's daily temperatures (simple sine wave + noise)
    days = np.linspace(0, 365, 365)
    temps = 30 + 15 * np.sin(2 * np.pi * days / 365) + np.random.randn(365) * 2

    for t in temps:
        kf.predict()
        kf.update(t)

preload_kalman_with_history()

def update_prediction_with_kalman(measured_temp):
    global kf

    # Run the filter
    kf.predict()
    kf.update(measured_temp)

    filtered = float(kf.x.item())

    # Update GUI safely
    def update_label():
        prediction_label.config(text=f"Predicted Temp: {filtered:.1f}°F")
        log(f"Kalman: {measured_temp:.1f}°F -> {filtered:.1f}°F")

    root.after(0, update_label)

def refresh_log_list():
    log_listbox.delete(0, tk.END)
    for filename in os.listdir(LOG_DIR):
        if filename.endswith(".log"):
            log_listbox.insert(tk.END, filename)

def load_selected_log(event=None):
    selection = log_listbox.curselection()
    if not selection:
        return
    filename = log_listbox.get(selection[0])
    path = os.path.join(LOG_DIR, filename)

    with open(path, "r") as f:
        contents = f.read()

    log_text.delete("1.0", tk.END)
    log_text.insert(tk.END, contents)

log_listbox.bind("<<ListboxSelect>>", load_selected_log)
refresh_button = tk.Button(logs_frame, text="Refresh Logs", command=refresh_log_list)
refresh_button.pack(pady=5)

refresh_log_list()

def on_close():
    global running
    running = False
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)

root.mainloop()