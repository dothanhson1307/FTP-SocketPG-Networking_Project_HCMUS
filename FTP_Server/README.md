# Hybrid FTP - Server Package

This directory contains the standalone Hybrid FTP Server.

## Server Network Info

- **Server Local IP**: `10.122.2.63` (Check using `ifconfig` or `ip a` if connected to another network)
- **Control Port (TCP)**: `80` (or configured port in `SERVER/Server.h`)
- **Transfer Mode Ports (UDP)**: `8080`, `8081`

---

## How to Build

### Using `make`:
```bash
make clean
make
```

### Using `cmake`:
```bash
mkdir -p build && cd build
cmake ..
make
```

---

## How to Run the Server

Since port `80` is a privileged port (< 1024), execute with `sudo`:
```bash
sudo ./server_app
```

The server will display:
```
[Server] Listening on port 80...
```

---

## Server Data Directories

- **`Repository/server_data/Downloadable_files/`**: Public files available for clients to download via `RETR`.
- **`Repository/server_data/Appendables/`**: Files available for append transfers (`APPE`).
- **`Repository/server_data/uploaded_user_data/<username>/`**: Storage location for files uploaded by clients via `STOR` / `STOU`.
