# OpenKiln

OpenKiln was born from the need to control my wifes "new" old kiln that she bought. If the dusty and rusty rheostats on the side of it were not bad enough, the fact that we had to continuously check up on it drove the inspiration for this project.

OpenKiln uses an ESP8266 (NodeMCU) to control 2 heating elements inside the kiln via high current solid state relays. In the beginning OpenKiln served its own web page - which was awesome - but I decided to move away from that for safety reasons. I didn't want the webservice to lock up the controller and cause a fire. So, now OpenKiln hosts it interface via Modbus/TCP and I use Node-Red to host the webpage. You can find the flow [here](/Source/Node-Red/Flow.md).

More details on the project will be coming soon but for now feel free to snoop around. There is a lot of documentation that needs to take place and I will try to add it as time permits. There is also a lot of legacy code here that is no longer in use that I will remove when I can.
