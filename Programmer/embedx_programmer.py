import subprocess
import sys
from pathlib import Path

PORT = input("Enter target ESP32 COM port (example: COM4): ").strip()

BASE_DIR = Path(__file__).resolve().parent

BOOTLOADER = BASE_DIR / "_receiveing123.ino.bootloader.bin"
PARTITIONS = BASE_DIR / "_receiveing123.ino.partitions.bin"
BOOT_APP0 = BASE_DIR / "boot_app0.bin"
FIRMWARE = BASE_DIR / "_receiveing123.ino.bin"


def run_command(command):
    print("\n----------------------------------------")
    print("Running:")
    print(" ".join(command))
    print("----------------------------------------")

    result = subprocess.run(command)

    if result.returncode != 0:
        print("\nERROR: Flashing failed.")
        sys.exit(1)


def check_files():
    files = [
        BOOTLOADER,
        PARTITIONS,
        BOOT_APP0,
        FIRMWARE
    ]

    for file in files:
        if not file.exists():
            print(f"\nERROR: File not found:")
            print(file)
            sys.exit(1)


print("\n========================================")
print("        EMBEDX ESP32 PROGRAMMER")
print("========================================")

check_files()

print("\nFirmware files found successfully.")

print("\nTarget COM port:", PORT)

print("\nErasing ESP32 flash...")
run_command([
    sys.executable,
    "-m",
    "esptool",
    "--port",
    PORT,
    "erase-flash"
])

print("\nFlashing EmbedX diagnostic firmware...")

run_command([
    sys.executable,
    "-m",
    "esptool",
    "--port",
    PORT,
    "--chip",
    "esp32",
    "--baud",
    "460800",
    "write-flash",
    "--flash-mode",
    "dio",
    "--flash-freq",
    "80m",
    "--flash-size",
    "4MB",
    "0x1000",
    str(BOOTLOADER),
    "0x8000",
    str(PARTITIONS),
    "0xe000",
    str(BOOT_APP0),
    "0x10000",
    str(FIRMWARE)
])

print("\n========================================")
print("       EMBEDX PROGRAMMING COMPLETE")
print("========================================")
print("Target:", PORT)
print("Status: SUCCESS")
print("========================================")