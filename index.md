## Home Environment Monitor
<img align="right" width="45%" height="350" src="helloworld.png"></img>
##### A system is presented that collects data from sensors distributed over a local area network. A simple method is used 
##### The system is implemented with an esp32 wroom as the hub with the remotes being various types of esp8266 modules at hand (mostly mini D1s and a few esp01s). Sensors included in project included one bmp280 atmospheric pressure sensor and an handful of humidity/temperature sensors (ath10, dht11 and dht22s). The host esp32 is running a tcp server task and periododicly ok nyerfac ok ng with three on board i2c devices- data is collected from an aht10 (humidity and temperature) and a bmp280 (atmospheric pressure) data is written to a ssd1306 display. A potentially large number of remote esp8266s, running a tcp client, are equiped with a humity and temperature sensor and distributed around the house and backyard.
##### The esp8266 tcp client remotes, independently, periodically collect data from it's sensors and sends a tcp packet with the following resources 
1-regitration number assigned by host
2-remote ip address
3-station name assigned by host
4-humidity measurement
5-temperature measurement. 
##### The remotes waits for response from host consisting of 
1-regisration number
2-assigned station name
3-the rate to take samples.
##### The esp32 tcp server host receives three different types of requests, namely 
1-index.html request
2-a client request as described above
3-a host request
##### The host request can be originated from the JavaScript within the index.html being run in a browser or from a perl program running netcat commands every few minutes for the purpose of data logging. The url resources contained in a request consist of 
1-regisration number
2 -station name to be assigned to previous regration number
3-rate in seconds of updates for data collection and webpage. 
##### The tcp response is 
1-number of registered devices
2-name assigned to station
3-humidity measurement
4-temperature measurement
5-name of station register to slot 1
6-humidty measurement
7-temperature measurement
    item 5-7 will repeat for each registered station.
##### The webpage displays the state of all the sensor measurement across the system with Google chart gauge visualizations, as new motes are added new gauges appear on wepage and as guages timeout they disappear. The webpage also includes text action boxes for station naming - for example stations can be named back porch or kitchen, ect... The rate of data aquistion is also set at this level.
##### A perl program is used to log environmental data. It can be set to collect data every 10 minutes, for example, and save data to disk. Notice that the webpage gives a snap shot updated every few seconds or minutes while the perl program provides a history of weeks or years worth of data.
