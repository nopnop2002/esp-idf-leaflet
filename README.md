# esp-idf-leaflet
Leaflet demo for esp-idf.   
Leaflet is the leading open-source JavaScript library for mobile-friendly interactive maps.   
Leaflet's homepage is [here](https://leafletjs.com/).   
This project demonstrates how to use Leaflet maps with ESP-IDF.   
![Image](https://github.com/user-attachments/assets/05fde83c-0934-4cdd-b3f4-540e88f93b89)   
![Image](https://github.com/user-attachments/assets/9765ed50-1162-4b8f-9644-b7ecbb3cb545)   

This project use [this](https://components.espressif.com/components/igrr/libnmea) component as NMEA parser.   
This project use [this](https://github.com/Molorius/esp32-websocket) component for WebSocket communication.   
Both are great components.   

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

# Hardware requirements
GPS module like NEO-6M.   
The GPS module is optional.   
This project will work without a GPS module.    

# Wireing to GPS module
If you are using a GPS module, wire it as follows:   
|GPS||ESP32|ESP32-S2/S3|ESP32-C2/C3/C6|
|:-:|:-:|:-:|:-:|:-:|
|VCC|--|3.3V|3.3V|3.3V|
|GND|--|GND|GND|GND||
|TXD|--|GPIO16|GPIO1|GPIO0|

You can change GPIO to any pin using menuconfig.

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-leaflet
cd esp-idf-leaflet
idf.py menuconfig
idf.py flash
```

# Configuration
![Image](https://github.com/user-attachments/assets/9772a458-a42a-40c0-9173-38112975fef0)
![Image](https://github.com/user-attachments/assets/01717382-bf58-4a8f-a30a-e52faf29f615)

This project has three modes of operation.   
- Demonstration Mode   
	Works without GPS.   
	Displays a map of the Statue of Liberty.   
	![Image](https://github.com/user-attachments/assets/e3d129a1-c427-48ae-a9c0-c64449a1d32b)

- GPS Mode   
	GPS required.   
	Displays a map of your current location obtained from GPS.   
	![Image](https://github.com/user-attachments/assets/1c8469f1-931f-4e8b-8efa-46615cc7309c)

- Move Mode   
	Works without GPS.   
	Moving from south to north through Central Park.
	![Image](https://github.com/user-attachments/assets/53544a9c-b3ef-40c3-bf3c-dbe65c3b1ac0)

- Setpoint Mode   
	Works without GPS.   
	Moving to a pre-determined location.   
	The location is defined in ```setpoint.def``` in setpoint directory.   
	![Image](https://github.com/user-attachments/assets/a432d484-beb1-47e3-90ee-563b6e726fde)

# How to use
Open a web browser and enter the ESP32's IP address in the address bar.   
In Demonstration Mode, it displays a map of the Statue of Liberty.   
![Image](https://github.com/user-attachments/assets/f5284f8d-c7f8-4ea8-af84-170734748e8d)   

You can use an mDNS hostname instead of an IP address.   
![Image](https://github.com/user-attachments/assets/27788adb-8f10-4b4b-9a7d-0090e088fe9b)   

You can use the zoom function.   
![Image](https://github.com/user-attachments/assets/01b44ac7-476c-422f-b9f6-d2306c6268d5)   
![Image](https://github.com/user-attachments/assets/5b1ed297-d965-4225-ad91-ea9e93604672)   

You can switch to full screen view.   
![Image](https://github.com/user-attachments/assets/d932c06e-ae92-493a-9f76-fb7160385ee3)   

In GPS Mode, it displays a map of your current location obtained from GPS.   
You can use the zoom function.   

In Move Mode, move through Central Park from south to north.   
In this mode, the zoom function is disabled.   
![Image](https://github.com/user-attachments/assets/028d2dc8-6de5-46f3-b1b1-f6f5b3f8d910)   

In Setpoint Mode, you will travel by Shinkansen from Tokyo to Shin-Osaka.
![Image](https://github.com/user-attachments/assets/d8ab3f5f-8804-444a-a5c6-dfdb75011f8a)
![Image](https://github.com/user-attachments/assets/02fd778b-8c16-4aa8-80f8-97509119e5de)
![Image](https://github.com/user-attachments/assets/4bddba46-875a-48ea-b7ac-50bff03ee65d)
![Image](https://github.com/user-attachments/assets/5c39e043-4766-4c78-8ae1-c1ca196c3c96)
