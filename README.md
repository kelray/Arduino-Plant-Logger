# Arduino-Plant-Logger
This is a plant monitor using NodeMCU-32S board (based on ESP32 microcontroller). It incorporate three sensors to capture four readings from the ambient environment of the plant:
- Ambient Temperature & humidity (using DHT11 sensor)
- Soil temperature (using DS18B20 sensor).
- Soil moisture (generic sensor, no part number available).
The monitor collects the data from all sensors in a time-stamped CSV file saved to an on-board 1GB Micro SD card. The file is emailed to user-defined email every 300 seconds (5 minutes). This interval can be changed, please refer to the code. 
A sample data CSV can be found with the source code. 

**Plant-Logger Hardware:**
![Plantlogger_labeled](https://user-images.githubusercontent.com/8460504/90962339-19dd0880-e464-11ea-91fe-0c3be8fc0a62.png)

**Circuito IO Schematic:**

<img width="500" alt="plant_logger_circuito io" src="https://user-images.githubusercontent.com/8460504/90962373-57da2c80-e464-11ea-9942-8fa87110e23b.png">

Notes:
- If you are planning to use GMail with this project for sensor data communication then you have to create a machine/app password in order for this application (or any IoT device) to access the email. 
- Don't use pin 12 of the NodeMCU32S with any 1-Wire data pin where a pull-up resistor is installed, as it's used during boot-up procees and uploading the program to the ESP32 EEPROM will fail because of this pull-up resistor. 

**License:**
No warranty, no liability, no guarantees on using this project (code and hardware). Use it at your own risk.
