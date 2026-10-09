#!/usr/bin/env python3
"""
Automated OpenSDA PEMicro Flash & Reset Utility for S32K144W
"""
import subprocess
import time
import os
import sys

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    elf_path = os.path.join(repo_root, "Debug_FLASH", "Gpio_Dio_Ip_Example_S32K144W.elf")
    
    if not os.path.exists(elf_path):
        print(f"Error: Target ELF not found at {elf_path}")
        print("Please build the project first: make -C Debug_FLASH all")
        sys.exit(1)

    pegdb = r"C:\NXP\S32DS.3.6.11\eclipse\plugins\com.pemicro.debug.gdbjtag.pne_6.1.8.202603121731\win32\pegdbserver_console.exe"
    gdb = r"C:\NXP\S32DS.3.6.11\S32DS\tools\gdb-arm\arm32-eabi\bin\arm-none-eabi-gdb.exe"
    script_path = os.path.join(repo_root, "gdb_flash.cmd")

    if not os.path.exists(pegdb) or not os.path.exists(gdb):
        print("Warning: Standard S32DS 3.6.11 debugger paths not found.")
        print(f"PE GDB Server: {pegdb} (exists: {os.path.exists(pegdb)})")
        print(f"ARM GDB: {gdb} (exists: {os.path.exists(gdb)})")
        sys.exit(1)

    print(f"[*] Programming: {elf_path}")
    print("[*] Launching PEMicro OpenSDA GDB server on port 7224...")
    server_cmd = [
        pegdb,
        "-startserver",
        "-device=NXP_S32K1xx_S32K144WF512M8",
        "-serverport=7224",
        "-interface=OPENSDA",
        "-use_swd=1"
    ]
    server_proc = subprocess.Popen(server_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(5)

    elf_posix = elf_path.replace("\\", "/")
    with open(script_path, "w") as f:
        f.write(f"""target remote localhost:7224
load {elf_posix}
monitor reset
continue &
quit
""")

    try:
        print("[*] Connecting GDB to flash ELF image...")
        res = subprocess.run([gdb, "-x", script_path, "--batch"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=30)
        print("[*] GDB Flash Output:\n", res.stdout.strip())
        if res.stderr:
            print("[*] GDB Flash Stderr:\n", res.stderr.strip())
        if res.returncode == 0:
            print("[+] Flash and target resume completed successfully!")
        else:
            print("[-] GDB returned error code:", res.returncode)
    except Exception as e:
        print("[-] GDB Exception:", e)
    finally:
        time.sleep(1)
        server_proc.terminate()
        if os.path.exists(script_path):
            os.remove(script_path)

if __name__ == "__main__":
    main()
