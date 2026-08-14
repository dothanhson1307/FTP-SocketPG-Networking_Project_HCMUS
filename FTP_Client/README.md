# Hybrid FTP - Client Package

This directory contains the standalone Hybrid FTP Client. You can copy this entire `FTP_Client/` folder to another laptop (Windows WSL Ubuntu / macOS / Linux) to connect to the server over Wi-Fi / LAN.

---

## 🪟 Windows Setup (via WSL Ubuntu)

If you are using **Windows with WSL (Ubuntu)**:

### 1. Install Build Tools
Open your WSL terminal and run:
```bash
sudo apt update && sudo apt install -y build-essential libssl-dev
```

### 2. Build the Client
```bash
make clean
make
```

### 3. Connect to the Server
Run the client with the server host laptop's IP address and Port:
```bash
./client_app 10.122.2.63 80
```

---

## 🍏 macOS / Linux Setup

### 1. Prerequisites
- **macOS**: `brew install openssl@3`
- **Linux / Debian / Ubuntu**: `sudo apt install build-essential libssl-dev`

### 2. Build & Run
```bash
make clean
make
./client_app 10.122.2.63 80
```

> **Syntax:** `./client_app [SERVER_IP] [SERVER_PORT]`
> - If port is omitted: defaults to `80`.
> - If both IP and port are omitted: defaults to `127.0.0.1:80`.

---

## Example FTP Session

```
$ ./client_app 10.122.2.63 80
[Client] Connecting to server at 10.122.2.63:80...
[Client] Connected successfully!
220 Hybrid FTP Server Ready.
ftp> USER son
331 Password required for son.
ftp> PASS son123
230 User logged in, proceed.
ftp> PASV
227 Entering Passive Mode (10,122,2,63,31,145)
ftp> RETR daydreaming.txt
150 Opening UDP data connection for daydreaming.txt (RETR, STOP-AND-WAIT).
226 Transfer complete.
ftp> QUIT
221 Goodbye.
```

---

## Local User Staging Directory

- Local files for upload (`STOR`, `STOU`) and downloaded files (`RETR`) reside in:
  `Repository/user_data/<username>/`
