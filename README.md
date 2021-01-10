# Bioreactor

#### About the Bioreactor
- Our team of computer science and electronic and electrical engineering students produced a bioreactor control system that regulates the pH, stirrer RPM and temperature of a bioreactor.
- The user inputs their set points using a GUI, for example:  
`pH: 3.4, Stirrer RMP: 1180, Temperature: 32.4°C`  
- These values are then sent to the Arduino which controls the circuits that physically change the actuators of their respective subsystems.
- As part of this project we wrote code using interrupts and timers, and a device driver for a sensor attached to the board. 

##### How to run Inventory Manager:
- The bioreactor itself relies on proprietary simulation software. The link below is a video of it running on a virtual machine where me and one of my group project partners talk through the bioreactor and its design.
https://youtu.be/0RML-l0z9cI
- The file uiMac.py can run on any computer with Python, Tkinter and Matplotlib installed and is a demo of the UI. It uses periodically generated random values for the pH, stirrer RPM and temperature. The commented lines are what would be needed to run it on the working Arduino with the sketch.ino file.  

##### My role in this project
- I was mainly responsible for the UI which you can see in the uiMac.py and ArduinoCode/ui.py files.
- I was also heavily involved in the communication over the serial port between the Ardunio and Python files.
- I also had to fully understand the code written in the sketch.ino file to proud documentation.
    

