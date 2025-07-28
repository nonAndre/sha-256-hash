# 🔐 SHA-256 Hashing in C with System V IPC (Shared Memory & Semaphores)

This project implements SHA-256 hashing using multiple processes in C, coordinated through System V shared memory and System V semaphores.

It demonstrates how processes can communicate and synchronize using classic IPC mechanisms.

# 🚀 Features

Computes the SHA-256 hash of a file or string.

Uses System V shared memory (shmget, shmat, etc.) for inter-process data exchange.

Synchronization via System V semaphores (semget, semop, etc.).

Implements multi-process parallelism using fork().

# 🛠 Requirements

Linux (or any System V–compliant UNIX)

GCC compiler

OpenSSL development library (for SHA-256 hashing)

# 📦 Setup Instructions

```bash
   git clone https://github.com/nonAndre/sha-256-hash.git
   cd sha-256-hash
   make
```
# 🖥️ Running the Program

To run the project, you need to use two terminal windows:

In the first terminal, start the server:
```bash
  ./server
```

In the second terminal, start the client:
```bash
  ./client
```






