import serial
import time

CONTROLLER_PORT = "COM5"
BAUD_RATE = 115200

print("Connecting to EmbedX controller...")

controller = serial.Serial(
    CONTROLLER_PORT,
    BAUD_RATE,
    timeout=2
)

time.sleep(2)

controller.reset_input_buffer()

print("Connected to controller.")
print("Sending RUN ALL TESTS command...")

controller.write(b"9\n")

start_time = time.time()

while time.time() - start_time < 30:
    if controller.in_waiting:
        data = controller.readline().decode("utf-8", errors="ignore").strip()

        if data:
            print(data)

    time.sleep(0.05)

controller.close()

print("\nController communication test complete.")