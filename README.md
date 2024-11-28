# T.U.K.I - The Ultimate Kernel Implementation
## Overview
T.U.K.I is a modular project designed to simulate the functionality of a real operating system. It provides a structured framework to understand and explore core OS concepts through its five interconnected modules.
## Features
The proyect was written in C and tested on a VM running Lubuntu 4.0
## Installation
1. Set up a virtual machine running Lubuntu 4.0
2. Clone the repository
  ```bash
   git clone https://github.com/santiago-MV/T.U.K.I.git
  ```
3. Navigate to the scripts directory
  ```bash
   cd ./T.U.K.I/scripts
  ```
4. Run the scripts to install the dependencies and to make all the files
  ```bash
   ./instalar_librerias.sh
   ./makeall.sh
  ```
6. Run the five modules
  ```bash
    cd ./<module>
     ./module
  ```
7. Run the test scripts
## Modules
- **Console**
This module handles the input of instructions, allowing users to interact with the system through a command-line interface. It simulates the user input process in a real operating system.
- **Memory**
The memory module simulates the rapid access memory (RAM) that the CPU works with. It manages memory allocation, deallocation, and ensures efficient use of the system's resources.
- **Kernel**
The kernel module manages the interconnections between the different components of the system. It acts as the central core that handles communication between the CPU, memory, console, and file system.
- **File System**
This module manages files within the operating system. It simulates file creation, reading, writing, and deletion, providing basic file management functionality.
- **CPU** 
This module is responsible for processing the instructions provided by the Console. It simulates a basic CPU operation, including instruction fetching, decoding, and execution.
