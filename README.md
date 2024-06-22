# DashIO

So, what is **DashIO**? It is a quick effortless way to connect your IoT device to your phone, tablet or iPad using the free **Dash** app. It allows easy setup of controls such as Dials, Text Boxes, Charts, Graphs, Notifications..., from your IoT device. You can define the look and layout of the controls on your phone from your IoT device. There are three methods to connect to your phone; Bluetooth Low Energy (BLE), TCP or MQTT.

What is **dash** then? **dash** is an IoT platform based on an MQTT server with extra bits added in to allow you to store data, manage your devices, send notifications, share your devices, and save your **Dash** app setup. 

## Documentation

For the big picture on **DashIO**, take a look at our website: <a href="https://dashio.io">dashio.io</a>

For all documentation and software guides: <a href="https://dashio.io/documents">dashio.io/documents</a>

For the **DashIO** Python library documentation: <a href="https://dashio.io/guide-python">dashio.io/guide-python</a>

## Dash IoT Application

The **Dash** app is free and available for both Apple and Android devices. Use it to create beautiful and powerful user interfaces to you IoT devices.


<img src="https://dashio.io/wp-content/uploads/2020/11/IMG_4154.jpeg" width="200" />

<img src="https://dashio.io/wp-content/uploads/2020/12/IMG_4203.jpeg" width="600" />

## Release Notes

### 1.1.4 (22 June 2024)
- Fix issues when Espressif modified their library for new hardware variants. For ESP32, now need to include WiFi.h and change how macAddress is obtained.

### 1.1.3 (28 April 2024)

- StatusCode enum & onStatusCallback() added to provide information on connection status changes (only used in ESP32 at this stage).
- Bug fixes (removed spurious incomingBufferSize & improve function addTimeGraphLineFloats)

### 1.1.2 (16 April 2024)

- Fixed bug in MQTT input buffer.
- Added features for future Dash app release (TextBox caption, Dash server Store & Forward for intermittently connected IoT devices).

### 1.1.1 (1 March 2024)

- Improved String handling for TimeGraph control graph lines.
- Other minor improvements and fixes.

### 1.1.0 (9 February 2024)

- String handling has been improved, particularly where long strings are created from array of data. For example, the method getTimeGraphLineFloats has been raplaced with addTimeGraphLineFloats which appends the graph line text to an existing string.
- Other minor improvements.
