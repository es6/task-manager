# Linux Task Manager

### Team Members
- Ben Lin
- Andrew Bradley
- Rafee Adnan

### Technologies Used
- C++
- Qt 5

### Overview
This project aims to develop a system monitor similar to the Windows Task Manager or Linux System Monitor. The project utilizes the `/proc` filesystem and creating a graphical user interface to display relevant system information.

### Features
1. **Basic System Information**: Displays OS release, kernel version, memory, processor version, and disk storage.
2. **Process Information**: Includes a table to display all running processes, with options for different views and refresh capabilities.
3. **Process-specific Actions**: Allows for actions like stopping, continuing, or killing processes, and listing memory maps and open files.
4. **Detailed View**: Shows detailed information for individual processes including state, memory usage, CPU time, etc.
5. **Graphs**: Tracks and displays graphs for CPU usage, memory and swap usage, and network usage.
6. **File System**: Shows usage for each mount point.

### Usage
On a Linux computer, go into the project directory, type `make`, and run the executable
