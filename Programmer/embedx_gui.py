import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import serial
import threading
from pathlib import Path
import sys
import time


# ============================================================
# EMBEDX CONFIGURATION
# ============================================================

BASE_DIR = Path(__file__).resolve().parent

BOOTLOADER = BASE_DIR / "_receiveing123.ino.bootloader.bin"
PARTITIONS = BASE_DIR / "_receiveing123.ino.partitions.bin"
BOOT_APP0 = BASE_DIR / "boot_app0.bin"
FIRMWARE = BASE_DIR / "_receiveing123.ino.bin"

CONTROLLER_BAUD = 115200


# ============================================================
# EMBEDX GUI
# ============================================================

class EmbedXGUI:

    def __init__(self, root):

        self.root = root

        self.root.title("EmbedX - Embedded Diagnostic Tool")

        self.root.geometry("1200x720")

        self.root.resizable(False, False)

        self.upload_success = False

        self.build_interface()

        self.refresh_ports()


    # ========================================================
    # BUILD GUI
    # ========================================================

    def build_interface(self):

        # ----------------------------------------------------
        # TITLE
        # ----------------------------------------------------

        title = tk.Label(
            self.root,
            text="EMBEDX",
            font=("Segoe UI", 24, "bold")
        )

        title.pack(
            pady=(15, 0)
        )


        subtitle = tk.Label(
            self.root,
            text="Embedded Diagnostic Tool",
            font=("Segoe UI", 11)
        )

        subtitle.pack(
            pady=(0, 15)
        )


        # ====================================================
        # MAIN TWO-COLUMN AREA
        # ====================================================

        main_frame = tk.Frame(
            self.root
        )

        main_frame.pack(
            padx=20,
            pady=5,
            fill="both",
            expand=True
        )


        # ====================================================
        # LEFT SIDE
        # ====================================================

        left_frame = tk.Frame(
            main_frame,
            width=470
        )

        left_frame.pack(
            side="left",
            fill="y",
            padx=(0, 10)
        )

        left_frame.pack_propagate(False)


        # ----------------------------------------------------
        # CONNECTION FRAME
        # ----------------------------------------------------

        connection_frame = ttk.LabelFrame(
            left_frame,
            text="Connection",
            padding=15
        )

        connection_frame.pack(
            fill="x",
            pady=(0, 10)
        )


        # Target Board

        ttk.Label(
            connection_frame,
            text="Target Board:"
        ).grid(
            row=0,
            column=0,
            padx=10,
            pady=8,
            sticky="w"
        )


        self.target_board = ttk.Combobox(
            connection_frame,
            values=["ESP32"],
            state="readonly",
            width=20
        )

        self.target_board.current(0)

        self.target_board.grid(
            row=0,
            column=1,
            padx=10
        )


        # Target COM

        ttk.Label(
            connection_frame,
            text="Target COM:"
        ).grid(
            row=1,
            column=0,
            padx=10,
            pady=8,
            sticky="w"
        )


        self.target_port = ttk.Combobox(
            connection_frame,
            state="readonly",
            width=20
        )

        self.target_port.grid(
            row=1,
            column=1,
            padx=10
        )


        # Controller COM

        ttk.Label(
            connection_frame,
            text="Controller COM:"
        ).grid(
            row=2,
            column=0,
            padx=10,
            pady=8,
            sticky="w"
        )


        self.controller_port = ttk.Combobox(
            connection_frame,
            state="readonly",
            width=20
        )

        self.controller_port.grid(
            row=2,
            column=1,
            padx=10
        )


        # Refresh Ports

        refresh_button = ttk.Button(
            connection_frame,
            text="Refresh Ports",
            command=self.refresh_ports
        )

        refresh_button.grid(
            row=1,
            column=2,
            rowspan=2,
            padx=20
        )


        # ----------------------------------------------------
        # FIRMWARE FRAME
        # ----------------------------------------------------

        firmware_frame = ttk.LabelFrame(
            left_frame,
            text="Firmware",
            padding=15
        )

        firmware_frame.pack(
            fill="x",
            pady=(0, 10)
        )


        ttk.Label(
            firmware_frame,
            text="Firmware:"
        ).grid(
            row=0,
            column=0,
            padx=10
        )


        self.firmware_label = ttk.Label(
            firmware_frame,
            text="_receiveing123.ino.bin"
        )

        self.firmware_label.grid(
            row=0,
            column=1,
            padx=10
        )


        # ----------------------------------------------------
        # BUTTON FRAME
        # ----------------------------------------------------

        button_frame = ttk.LabelFrame(
            left_frame,
            text="Control",
            padding=15
        )

        button_frame.pack(
            fill="x",
            pady=(0, 10)
        )


        # Upload button

        self.upload_button = ttk.Button(
            button_frame,
            text="UPLOAD CODE",
            command=self.start_upload
        )

        self.upload_button.pack(
            fill="x",
            padx=15,
            pady=8,
            ipady=8
        )


        # Run test button

        self.test_button = ttk.Button(
            button_frame,
            text="RUN TEST",
            command=self.start_test,
            state="disabled"
        )

        self.test_button.pack(
            fill="x",
            padx=15,
            pady=8,
            ipady=8
        )


        # ----------------------------------------------------
        # STATUS FRAME
        # ----------------------------------------------------

        status_frame = ttk.LabelFrame(
            left_frame,
            text="Status",
            padding=15
        )

        status_frame.pack(
            fill="x"
        )


        self.status_label = ttk.Label(
            status_frame,
            text="Ready",
            font=("Segoe UI", 10, "bold"),
            wraplength=380
        )

        self.status_label.pack(
            anchor="w"
        )


        # ====================================================
        # RIGHT SIDE
        # ====================================================

        right_frame = tk.Frame(
            main_frame,
            width=680
        )

        right_frame.pack(
            side="right",
            fill="both",
            expand=True
        )


        # ----------------------------------------------------
        # DIAGNOSTIC RESULT FRAME
        # ----------------------------------------------------

        result_frame = ttk.LabelFrame(
            right_frame,
            text="Diagnostic Result",
            padding=15
        )

        result_frame.pack(
            fill="both",
            expand=True
        )


        # ----------------------------------------------------
        # RESULT TEXT AREA
        # ----------------------------------------------------

        self.result_text = tk.Text(
            result_frame,
            height=30,
            width=78,
            font=("Consolas", 10),
            state="disabled",
            wrap="none",
            relief="solid",
            borderwidth=1
        )

        self.result_text.pack(
            fill="both",
            expand=True
        )


    # ========================================================
    # PORT DETECTION
    # ========================================================

    def refresh_ports(self):

        try:

            result = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "serial.tools.list_ports"
                ],
                capture_output=True,
                text=True
            )


            ports = []


            for line in result.stdout.splitlines():

                line = line.strip()


                if line.startswith("COM"):

                    ports.append(
                        line.split()[0]
                    )


            self.target_port["values"] = ports

            self.controller_port["values"] = ports


            # Target = COM4

            if "COM4" in ports:

                self.target_port.set(
                    "COM4"
                )

            elif ports:

                self.target_port.current(
                    0
                )


            # Controller = COM5

            if "COM5" in ports:

                self.controller_port.set(
                    "COM5"
                )

            elif len(ports) > 1:

                self.controller_port.current(
                    1
                )


            self.set_status(
                f"Ports detected: {', '.join(ports)}"
            )


        except Exception as e:

            messagebox.showerror(
                "Port Error",
                str(e)
            )


    # ========================================================
    # STATUS
    # ========================================================

    def set_status(self, text):

        self.status_label.config(
            text=text
        )


    # ========================================================
    # CHECK FIRMWARE FILES
    # ========================================================

    def check_firmware_files(self):

        required_files = [

            BOOTLOADER,

            PARTITIONS,

            BOOT_APP0,

            FIRMWARE
        ]


        for file in required_files:

            if not file.exists():

                messagebox.showerror(
                    "Firmware Error",
                    f"Missing file:\n\n{file.name}"
                )

                return False


        return True


    # ========================================================
    # START UPLOAD
    # ========================================================

    def start_upload(self):

        target = self.target_port.get()


        if not target:

            messagebox.showwarning(
                "Target Port",
                "Select the target COM port first."
            )

            return


        if not self.check_firmware_files():

            return


        self.upload_button.config(
            state="disabled"
        )


        self.test_button.config(
            state="disabled"
        )


        self.set_status(
            f"Uploading firmware to {target}..."
        )


        thread = threading.Thread(
            target=self.upload_firmware,
            args=(target,),
            daemon=True
        )


        thread.start()


    # ========================================================
    # UPLOAD FIRMWARE
    # ========================================================

    def upload_firmware(self, target):

        try:

            # ------------------------------------------------
            # ERASE FLASH
            # ------------------------------------------------

            erase_command = [

                sys.executable,

                "-m",
                "esptool",

                "--port",
                target,

                "erase-flash"
            ]


            erase_result = subprocess.run(
                erase_command,
                capture_output=True,
                text=True
            )


            if erase_result.returncode != 0:

                self.upload_failed(
                    erase_result.stdout +
                    erase_result.stderr
                )

                return


            # ------------------------------------------------
            # WRITE FLASH
            # ------------------------------------------------

            flash_command = [

                sys.executable,

                "-m",
                "esptool",

                "--port",
                target,

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

                # Bootloader

                "0x1000",

                str(
                    BOOTLOADER
                ),

                # Partitions

                "0x8000",

                str(
                    PARTITIONS
                ),

                # boot_app0

                "0xe000",

                str(
                    BOOT_APP0
                ),

                # Diagnostic firmware

                "0x10000",

                str(
                    FIRMWARE
                )
            ]


            flash_result = subprocess.run(
                flash_command,
                capture_output=True,
                text=True
            )


            if flash_result.returncode != 0:

                self.upload_failed(
                    flash_result.stdout +
                    flash_result.stderr
                )

                return


            self.upload_completed()


        except Exception as e:

            self.upload_failed(
                str(e)
            )


    # ========================================================
    # UPLOAD SUCCESS
    # ========================================================

    def upload_completed(self):

        self.root.after(
            0,
            self._upload_completed_ui
        )


    def _upload_completed_ui(self):

        self.upload_success = True


        self.set_status(
            "Firmware uploaded successfully."
        )


        self.upload_button.config(
            state="normal"
        )


        self.test_button.config(
            state="normal"
        )


        self.clear_result()


        self.append_result(

            "========================================\n"

            "       EMBEDX PROGRAMMING COMPLETE\n"

            "========================================\n\n"

            "Target firmware uploaded successfully.\n\n"

            "Target reset completed.\n\n"

            "RUN TEST is now available.\n"
        )


        messagebox.showinfo(
            "Upload Successful",
            "Diagnostic firmware uploaded successfully."
        )


    # ========================================================
    # UPLOAD FAILURE
    # ========================================================

    def upload_failed(self, output):

        self.root.after(
            0,
            lambda: self._upload_failed_ui(
                output
            )
        )


    def _upload_failed_ui(self, output):

        self.upload_button.config(
            state="normal"
        )


        self.test_button.config(
            state="disabled"
        )


        self.set_status(
            "Firmware upload failed."
        )


        self.clear_result()


        self.append_result(
            output
        )


        messagebox.showerror(
            "Upload Failed",
            "Firmware upload failed.\n\n"
            "Check the result window for details."
        )


    # ========================================================
    # START TEST
    # ========================================================

    def start_test(self):

        controller = self.controller_port.get()


        if not controller:

            messagebox.showwarning(
                "Controller Port",
                "Select the controller COM port first."
            )

            return


        self.test_button.config(
            state="disabled"
        )


        self.upload_button.config(
            state="disabled"
        )


        self.clear_result()


        self.set_status(
            "Running diagnostics..."
        )


        thread = threading.Thread(
            target=self.run_diagnostics,
            args=(controller,),
            daemon=True
        )


        thread.start()


    # ========================================================
    # RUN DIAGNOSTICS
    # ========================================================

    def run_diagnostics(self, controller_port):

        controller = None


        try:

            controller = serial.Serial(
                controller_port,
                CONTROLLER_BAUD,
                timeout=1
            )


            time.sleep(2)


            controller.reset_input_buffer()


            # Option 9 = RUN ALL TESTS

            controller.write(
                b"9\n"
            )


            controller.flush()


            output_lines = []


            start_time = time.time()


            while time.time() - start_time < 30:

                if controller.in_waiting:

                    data = controller.readline().decode(
                        "utf-8",
                        errors="ignore"
                    ).strip()


                    if data:

                        output_lines.append(
                            data
                        )


                        # Final result detected

                        if "OVERALL RESULT" in data:

                            time.sleep(1)


                            while controller.in_waiting:

                                extra = controller.readline().decode(
                                    "utf-8",
                                    errors="ignore"
                                ).strip()


                                if extra:

                                    output_lines.append(
                                        extra
                                    )


                            break


                time.sleep(0.05)


            controller.close()


            output = "\n".join(
                output_lines
            )


            self.root.after(
                0,
                lambda: self.display_test_result(
                    output
                )
            )


        except Exception as e:

            if controller and controller.is_open:

                controller.close()


            self.root.after(
                0,
                lambda: self.test_failed(
                    str(e)
                )
            )


    # ========================================================
    # EXTRACT UNIFIED RESULT
    # ========================================================

    def extract_unified_result(self, output):

        marker = "EMBEDX DIAGNOSTIC RESULT"


        marker_index = output.rfind(
            marker
        )


        if marker_index == -1:

            return output.strip()


        start_index = output.rfind(
            "========================================",
            0,
            marker_index
        )


        if start_index == -1:

            start_index = marker_index


        result = output[
            start_index:
        ]


        # Remove controller menu after result

        menu_marker = "========== EMBEDX =========="


        menu_index = result.find(
            menu_marker
        )


        if menu_index != -1:

            result = result[
                :menu_index
            ]


        return result.strip()


    # ========================================================
    # DISPLAY UNIFIED RESULT
    # ========================================================

    def display_test_result(self, output):

        unified_result = self.extract_unified_result(
            output
        )


        self.clear_result()


        self.append_result(
            unified_result
        )


        self.set_status(
            "Diagnostic test completed successfully."
        )


        self.upload_button.config(
            state="normal"
        )


        self.test_button.config(
            state="normal"
        )


    # ========================================================
    # TEST FAILURE
    # ========================================================

    def test_failed(self, error):

        self.clear_result()


        self.append_result(

            "========================================\n"

            "       EMBEDX DIAGNOSTIC ERROR\n"

            "========================================\n\n"

            f"{error}\n"
        )


        self.set_status(
            "Diagnostic test failed."
        )


        self.upload_button.config(
            state="normal"
        )


        self.test_button.config(
            state="normal"
        )


    # ========================================================
    # CLEAR RESULT
    # ========================================================

    def clear_result(self):

        self.result_text.config(
            state="normal"
        )


        self.result_text.delete(
            "1.0",
            tk.END
        )


        self.result_text.config(
            state="disabled"
        )


    # ========================================================
    # ADD RESULT
    # ========================================================

    def append_result(self, text):

        self.result_text.config(
            state="normal"
        )


        self.result_text.insert(
            tk.END,
            text
        )


        self.result_text.see(
            tk.END
        )


        self.result_text.config(
            state="disabled"
        )


# ============================================================
# MAIN PROGRAM
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = EmbedXGUI(
        root
    )

    root.mainloop()