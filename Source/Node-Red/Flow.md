# Node-Red #

I use Node-Red as an interface between OpenKiln and an Influxdb database and a webserver HMI to control OpenKiln. Below is the flow that you can import into Node-Red as well as information on the nodes you will need to install.

http://Node-Red:1880/Kiln
![Webserver-HMI](/Media/Node-Red/Webserver-HMI.png)

## Nodes ##

node-red-contrib-modbus
node-red-contrib-influxdb

## Flows ##

Here is the code for the flows.

### Kiln ###

![Kiln-Flow](/Media/Node-Red/Kiln-Flow.png)

```json
[
    {
        "id": "2df22b78ebc5ab05",
        "type": "tab",
        "label": "Kiln",
        "disabled": false,
        "info": ""
    },
    {
        "id": "60ea0c09a07a7e7e",
        "type": "websocket in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "server": "6b5cc424.028694",
        "client": "",
        "x": 210,
        "y": 240,
        "wires": [
            [
                "9065fd74764d96c9",
                "89be4e5a7216b0be"
            ]
        ]
    },
    {
        "id": "1f64a6a39b41f09d",
        "type": "websocket out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "server": "6b5cc424.028694",
        "client": "",
        "x": 790,
        "y": 240,
        "wires": []
    },
    {
        "id": "d58831b84159a37c",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 180,
        "y": 500,
        "wires": [
            [
                "2261d10c9abe6e62"
            ]
        ]
    },
    {
        "id": "dc4fb41b8422037d",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 790,
        "y": 500,
        "wires": []
    },
    {
        "id": "2261d10c9abe6e62",
        "type": "template",
        "z": "2df22b78ebc5ab05",
        "name": "JavaScript",
        "field": "payload.script",
        "fieldType": "msg",
        "format": "javascript",
        "syntax": "plain",
        "template": "var Socket;\n  function init() \n  {\n    Socket = new WebSocket(\"ws://\" + window.location.hostname + \":1880/ws/Kiln\");\n    Socket.onmessage = function(event) { processReceivedCommand(event); };\n  }\n\n\n  function processReceivedCommand(evt) \n  {\n\n      var obj = JSON.parse(evt.data);\n      if (obj.topic == \"status\") {\n          document.getElementById('STATUS_CURRENT_SCHEDULE_NAME').innerHTML = obj.data.id1;\n          document.getElementById('STATUS_CURRENT_SEGMENT_NAME').innerHTML = obj.data.id2;\n          var status = \"\";\n          switch (obj.data.id3) {\n            case 0: // idle\n              status = \"IDLE\";\n              break;\n            case 1: // ramp\n              status = \"RAMP\";\n              break;\n            case 2: // soak\n              status = \"SOAK\";\n              break;\n            case 3: // hold\n              status = \"HOLD\";\n              break;\n            case 4: // init\n              status = \"INIT\";\n              break;\n            case 5: // start\n              status = \"START\";\n              break;\n          }\n          document.getElementById('STATUS_CURRENT_SEGMENT_STATUS').innerHTML = status;\n          document.getElementById('STATUS_TIME_REMAINING').innerHTML = obj.data.id4;\n          document.getElementById('STATUS_SETPOINT').innerHTML = obj.data.id5;\n          document.getElementById('STATUS_UPPER_TEMPERATURE').innerHTML = obj.data.id6;\n          document.getElementById('STATUS_LOWER_TEMPERATURE').innerHTML = obj.data.id7;\n          var modebtn = document.getElementById('MODE_BUTTON');\n          switch (obj.data.id8) {\n            case 1: \n              modebtn.innerHTML = \"AUTOMATIC\";\n              modebtn.className = \"BTN_GREEN\";\n              break;\n            case 2: \n              modebtn.innerHTML = \"MANUAL\";\n              modebtn.className = \"BTN_BLUE\";\n              break;\n            case 3: \n              modebtn.innerHTML = \"SIMULATION\";\n              modebtn.className = \"BTN_ORANGE\";\n              break;\n          }\n          if (obj.data.id9) {\n            document.getElementById('HOLD_BUTTON').style.visibility='visible';\n          } else {\n            document.getElementById('HOLD_BUTTON').style.visibility='hidden';\n          }\n          var thermalbanner = document.getElementById('THERMAL_BANNER');\n          var thermalbannertext = document.getElementById('THERMAL_BANNER_TEXT');\n          if (!obj.data.id13) {\n              if (!obj.data.id10) {\n                thermalbannertext.innerText = \"THERMAL MONITORING OK\";\n                thermalbanner.style.backgroundColor = \"yellowGreen\";\n              } else {\n                thermalbannertext.innerText = \"THERMAL MONITORING NOT OK\";\n                thermalbanner.style.backgroundColor = \"tomato\";\n              }\n          } else {\n            thermalbannertext.innerText = \"THERMAL MONITORING DISABLED\";\n            thermalbanner.style.backgroundColor = \"darkOrange\";\n          }\n          var safetybanner = document.getElementById('SAFETY_BANNER');\n          var safetybannertext = document.getElementById('SAFETY_BANNER_TEXT');\n          if (obj.data.id11) {\n            safetybannertext.innerText = \"SAFETY CIRCUIT OK\";\n            safetybanner.style.backgroundColor = \"yellowGreen\";\n          } else {\n            safetybannertext.innerText = \"SAFETY CIRCUIT NOT OK\";\n            safetybanner.style.backgroundColor = \"tomato\";\n          }\n      }\n  }\n  \n    document.getElementById('START_BUTTON').addEventListener('click', startButtonClicked);\n    function startButtonClicked()\n    {   \n      var btn = document.getElementById('START_BUTTON');\n      var btnText = btn.textContent || btn.innerText;\n      sendText('{\"topic\":\"CMD-START_PROFILE\",\"val\":\"true\"}');\n    }\n\n    document.getElementById('STOP_BUTTON').addEventListener('click', stopButtonClicked);\n    function stopButtonClicked()\n    {   \n      sendText('{\"topic\":\"CMD-STOP_PROFILE\",\"val\":\"true\"}');\n    }\n\n    document.getElementById('MODE_BUTTON').addEventListener('click', modeButtonClicked);\n    function modeButtonClicked()\n    {   \n      sendText('{\"topic\":\"CMD-CHANGE_MODE\",\"val\":\"true\"}');\n    }\n\n    document.getElementById('HOLD_BUTTON').addEventListener('click', holdButtonClicked);\n    function holdButtonClicked()\n    {   \n      sendText('{\"topic\":\"CMD-RELEASE_HOLD\",\"val\":\"true\"}');\n    }\n\n    document.getElementById('NEXT_BUTTON').addEventListener('click', nextButtonClicked);\n    function nextButtonClicked()\n    {   \n      sendText('{\"topic\":\"CMD-NEXT_SCHEDULE\",\"val\":\"true\"}');\n    }\n\n    document.getElementById('PREV_BUTTON').addEventListener('click', prevButtonClicked);\n    function prevButtonClicked()\n    {   \n      sendText('{\"topic\":\"CMD-PREV_SCHEDULE\",\"val\":\"true\"}');\n    }\n  \n    function sendText(data)\n    {\n      Socket.send(data);\n    }\n  \n    window.onload = function(e)\n    { \n      init();\n    }\n",
        "output": "str",
        "x": 350,
        "y": 500,
        "wires": [
            [
                "a48f58f73a32d13b"
            ]
        ]
    },
    {
        "id": "a48f58f73a32d13b",
        "type": "template",
        "z": "2df22b78ebc5ab05",
        "name": "CSS",
        "field": "payload.style",
        "fieldType": "msg",
        "format": "html",
        "syntax": "mustache",
        "template": ":root {\n        --test-variable: true\n    }\n    body     { font-size:100%;\n              margin: 0;\n              padding: 0;\n              width: 100%;\n              height: 100%;\n              box-sizing: border-box;} \n    p        { font-size: 75%; }\n    /* https://www.bestcssbuttongenerator.com/#/4 */\n    .BTN_GREEN {\n      box-shadow: 0px 10px 14px -7px #d9fbbe;\n      background:linear-gradient(to bottom, #b8e356 5%, #a5cc52 100%);\n      background-color:#b8e356;\n      border-radius:4px;\n      border:1px solid #83c41a;\n      cursor:pointer;\n      color:#ffffff;\n      font-family:Arial;\n      font-size:13px;\n      font-weight:bold;\n      text-decoration:none;\n      text-shadow:0px 1px 0px #86ae47;\n    }\n    .BTN_GREEN:hover {\n      background:linear-gradient(to bottom, #a5cc52 5%, #b8e356 100%);\n\t    background-color:#a5cc52;\n    }\n    .BTN_RED {\n      box-shadow: 0px 10px 14px -7px #f7c5c0;\n      background:linear-gradient(to bottom, #fc8d83 5%, #e4685d 100%);\n      background-color:#fc8d83;\n      border-radius:4px;\n      border:1px solid #d83526;\n      cursor:pointer;\n      color:#ffffff;\n      font-family:Arial;\n      font-size:13px;\n      font-weight:bold;\n      text-decoration:none;\n      text-shadow:0px 1px 0px #b23e35;\n    }\n    .BTN_RED:hover {\n      background:linear-gradient(to bottom, #e4685d 5%, #fc8d83 100%);\n      background-color:#e4685d;\n    }\n    .BTN_ORANGE {\n      box-shadow: 0px 10px 14px -7px #fff6af;\n      background: linear-gradient(to bottom, #ffec64 5%, #ffab23 100%);\n      background-color: #ffec64;\n      border-radius:4px;\n      border: 1px solid #ffaa22;\n      cursor:pointer;\n      color: #ffffff;;\n      font-family:Arial;\n      font-size:13px;\n      font-weight:bold;\n      text-decoration:none;\n      text-shadow: 0px 1px 0px #ffee66;\n    }\n    .BTN_ORANGE:hover {\n      background: linear-gradient(to bottom, #ffab23 5%, #ffec64 100%);\n      background-color: #ffab23;\n    }\n    .BTN_GREY {\n      box-shadow: 0px 10px 14px -7px #ffffff;\n      background: linear-gradient(to bottom, #ededed 5%, #dfdfdf 100%);\n      background-color: #ededed;\n      border-radius:4px;\n      border: 1px solid #dcdcdc;\n      cursor:pointer;\n      color: #777777;\n      font-family:Arial;\n      font-size:13px;\n      font-weight:bold;\n      text-decoration:none;\n      text-shadow: 0px 1px 0px #ffffff;\n    }\n    .BTN_GREY:hover {\n      background: linear-gradient(to bottom, #dfdfdf 5%, #ededed 100%);\n      background-color: #dfdfdf;\n    }\n    .BTN_BLUE {\n      background:linear-gradient(to bottom, #79bbff 5%, #378de5 100%);\n      background-color:#79bbff;\n      border-radius:4px;\n      border:1px solid #337bc4;\n      cursor:pointer;\n      color:#ffffff;\n      font-family:Arial;\n      font-size:13px;\n      font-weight:bold;\n      text-decoration:none;\n      text-shadow:0px 1px 0px #528ecc;\n    }\n    .BTN_BLUE:hover {\n      background:linear-gradient(to bottom, #378de5 5%, #79bbff 100%);\n\t    background-color:#378de5;\n    }\n    .debug_raw {\n      position:absolute;\n      top: 35px;\n      left: 0px;\n    }\n    .midbuttons {\n    width:90%;\n    margin-left:5%;\n    margin-right:0px;\n    padding-left:0px;\n    padding-right:0px;\n    display:inline-block;\n    text-align:center; \n    }\n    .statuslabel {\n      width: 50%;\n      float: left;\n      text-align: right;\n      font-weight: bold;\n      color: #777777;\n    }\n    .status {\n      width: 50%;\n      float: right;\n      text-align: left;\n      text-indent: 4px;\n      color: #777777;\n    }\n    .BANNER {\n    position: fixed; \n    bottom: 0;\n    left: 0;\n    right: 0;\n    background:dimgray; /*rgba(128, 126, 126, 0.5); */\n    color: white;\n    }",
        "output": "str",
        "x": 510,
        "y": 500,
        "wires": [
            [
                "98d92a92d6e7c086"
            ]
        ]
    },
    {
        "id": "98d92a92d6e7c086",
        "type": "template",
        "z": "2df22b78ebc5ab05",
        "name": "HTML",
        "field": "payload",
        "fieldType": "msg",
        "format": "handlebars",
        "syntax": "mustache",
        "template": "<html>\n    <head>\n        <meta name='viewport' content='width=device-width, initial-scale=1.0'/>\n        <meta charset='utf-8'>\n        <meta name=\"apple-mobile-web-app-title\" content=\"Kiln UI\">\n        <meta name=\"apple-mobile-web-app-capable\" content=\"yes\">\n        <meta name=\"apple-mobile-web-app-status-bar-style\" content=\"translucent black\">\n        <link rel=\"apple-touch-startup-image\" href=\"/favicon-16x16.png\">\n        <link rel=\"apple-touch-icon\" sizes=\"180x180\" href=\"/apple-touch-icon.png\">\n        <link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\n        <link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\n        <link rel=\"manifest\" href=\"/site.webmanifest\">\n        <style>{{{payload.style}}}</style>\n        <title>Kiln Web UI</title>\n    </head>\n    <body>\n        <div id='main'>\n        <h2 style=\"text-align: center; color:#777777\">MAIN</h2>\n      </div>\n        <div class=\"midbuttons\" style=\"position:absolute; top:80px;\" >\n        <div>\n          <div class=\"statuslabel\">SCHEDULE: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">SEGMENT: </div><div class=\"status\" id=\"STATUS_CURRENT_SEGMENT_NAME\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">STATUS: </div><div class=\"status\" id=\"STATUS_CURRENT_SEGMENT_STATUS\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">REMAINING: </div><div class=\"status\" id=\"STATUS_TIME_REMAINING\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">SETPOINT: </div><div class=\"status\" id=\"STATUS_SETPOINT\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">UPPER TEMP: </div><div class=\"status\" id=\"STATUS_UPPER_TEMPERATURE\">loading...</div>\n        </div>\n        <div>\n          <div class=\"statuslabel\">LOWER TEMP: </div><div class=\"status\" id=\"STATUS_LOWER_TEMPERATURE\">loading...</div>\n        </div>\n      </div>\n        <hr style=\"position:absolute; top:225px; width: 80%; margin-left: 10%; color: lightgray;\">\n        <div class=\"midbuttons\" style=\"position:absolute; top:250px;\" >\n            <div style=\"color:dimgray; font-weight:bold;\">Temperature Reference</div>\n            <br>\n            <div class=\"statuslabel\" style=\"width: 37.5%; font-weight: normal;\">\n                <div class=\"statuslabel\">Cone 09: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1688&#x2109-1706&#x2109</div>\n                <div class=\"statuslabel\">Cone 08: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1728&#x2109-1753&#x2109</div>\n                <div class=\"statuslabel\">Cone 07: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1789&#x2109-1809&#x2109</div>\n            </div>\n            <div class=\"status\" style=\"width: 62.5%;\">\n                <div class=\"statuslabel\">Cone 06: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1828&#x2109-1855&#x2109</div>\n                <div class=\"statuslabel\">Cone 05: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1888&#x2109-1911&#x2109</div>\n                <div class=\"statuslabel\">Cone 04: </div><div class=\"status\" id=\"STATUS_CURRENT_SCHEDULE_NAME\">1945&#x2109-1971&#x2109</div>\n            </div>\n        </div>\n        <hr style=\"position:absolute; top:375px; width: 80%; margin-left: 10%; color: lightgray;\">\n        <div class=\"midbuttons\" style=\"position:absolute; bottom:80px;\">\n        <div>\n          <div style=\"width:50%;float:left;\">\n            <button id='PREV_BUTTON'\n            class=\"BTN_GREY\"\n            style=\"width:100%;height:60px;\">PREV</button>\n          </div>\n          <div style=\"width:50%;float:right;\">\n            <button id='NEXT_BUTTON'\n            class=\"BTN_GREY\"\n            style=\"width:100%;height:60px;\">NEXT</button>\n          </div>\n        </div>\n        <div style=\"width:100%;\">\n          <button id='HOLD_BUTTON'\n          class=\"BTN_ORANGE\"\n          style=\"width:100%;height:60px;visibility:hidden;\">RELEASE HOLD</button>\n        </div>\n        <div style=\"width:100%;\">\n          <button id='MODE_BUTTON'\n          class=\"BTN_GREEN\"\n          style=\"width:100%;height:60px;\">AUTOMATIC</button>\n        </div>\n        <div>\n          <div style=\"width:50%;float:left;\">\n            <button id='START_BUTTON'\n            class=\"BTN_GREEN\"\n            style=\"width:100%;height:60px;\">START</button>\n          </div>\n          <div style=\"width:50%;float:right;\">\n            <button id='STOP_BUTTON'\n            class=\"BTN_RED\"\n            style=\"width:100%;height:60px;\">STOP</button>\n          </div>\n        </div>\n        <div style=\"width:100%;\">\n          <button onclick=\"window.location.href='http://raspberrypi-4:3000/d/CFy-Tsggk/kiln-temps?orgId=1&from=now-24h&to=now&refresh=5m&kiosk';\"\n          id='SCHEDULE_BUTTON'\n          class=\"BTN_GREY\"\n          style=\"width:100%;height:60px;\">TRENDING</button>\n        </div>\n      </div>\n        <div class=\"BANNER\" style=\"right: 50%; border-right: 1px solid dimgray;\" id=\"SAFETY_BANNER\">\n        <p style=\"text-align: center;\" id=\"SAFETY_BANNER_TEXT\">SAFETY</p>\n      </div>\n        <div class=\"BANNER\" style=\"left: 50%; border-left: 1px solid dimgray;\" id=\"THERMAL_BANNER\">\n        <p style=\"text-align: center;\" id=\"THERMAL_BANNER_TEXT\">THERMAL MONITORING</p>\n      </div>\n    </body>\n    <script>{{{payload.script}}}</script>\n</html>",
        "output": "str",
        "x": 650,
        "y": 500,
        "wires": [
            [
                "dc4fb41b8422037d"
            ]
        ]
    },
    {
        "id": "9065fd74764d96c9",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Kill Session",
        "func": "msg._session=\"\";\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "x": 510,
        "y": 240,
        "wires": [
            [
                "1f64a6a39b41f09d"
            ]
        ]
    },
    {
        "id": "89be4e5a7216b0be",
        "type": "json",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 350,
        "y": 300,
        "wires": [
            [
                "1be528cd9b54fd61"
            ]
        ]
    },
    {
        "id": "c9a8f98e37d8cd58",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "Input",
        "links": [
            "13aeb082.a20457",
            "4c0e4289.c2abd4",
            "3249fdf6.b46af2",
            "8a12c19f.ca3628",
            "d85ad30e11bf7c81",
            "6ac37c4128e09344",
            "dd50473f43c42c8d",
            "5abe1f1dd3e16716"
        ],
        "x": 95,
        "y": 620,
        "wires": [
            [
                "130e42f16cb1390b"
            ]
        ]
    },
    {
        "id": "c66ef7e12cb0a1db",
        "type": "catch",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "scope": null,
        "uncaught": false,
        "x": 60,
        "y": 20,
        "wires": [
            [
                "2fa0d6c57fa22ea5"
            ]
        ]
    },
    {
        "id": "2fa0d6c57fa22ea5",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "true",
        "targetType": "full",
        "x": 200,
        "y": 20,
        "wires": []
    },
    {
        "id": "67deef91386ccbee",
        "type": "inject",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "2",
        "crontab": "",
        "once": true,
        "onceDelay": "5",
        "topic": "",
        "payloadType": "str",
        "x": 190,
        "y": 160,
        "wires": [
            [
                "085cbb8dcc2ac148"
            ]
        ]
    },
    {
        "id": "e4a94591518297f5",
        "type": "json",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 490,
        "y": 160,
        "wires": [
            [
                "1f64a6a39b41f09d"
            ]
        ]
    },
    {
        "id": "130e42f16cb1390b",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Set Flow Variables",
        "func": "flow.set('kiln_01.'+msg.payload.TagName, msg.payload.TagValue, 'memoryOnly');\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 250,
        "y": 620,
        "wires": [
            [
                "043d67cef12b7030"
            ]
        ]
    },
    {
        "id": "1be528cd9b54fd61",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Parse Incoming",
        "func": "\nvar MB_CMD_SELECT_SCHEDULE    =  1\nvar MB_CMD_START_PROFILE     =   2\nvar MB_CMD_STOP_PROFILE     =    3\nvar MB_CMD_HOLD_RELEASE     =    4\nvar MB_CMD_THERM_OVERRIDE    =   5\nvar MB_CMD_WRITE_EEPROM      =   6\nvar MB_SCH_SEG_ENABLED      =    7\nvar MB_SCH_SEG_HOLD_EN      =    8\nvar MB_CMD_CAL_CH0_LOW      =    9\nvar MB_CMD_CAL_CH1_LOW      =    10\nvar MB_CMD_CAL_CH0_HIGH     =    11\nvar MB_CMD_CAL_CH1_HIGH     =    12\n\nvar MB_MODE = 1\nvar MB_CMD_SELECTED_SCHEDULE =   2\nvar MB_CMD_SETPOINT  =           3\nvar MB_PID_P_01    =             5\nvar MB_PID_I_01   =              7\nvar MB_PID_D_01   =              9\nvar MB_PID_P_02   =              11\nvar MB_PID_I_02   =              13\nvar MB_PID_D_02   =              15\nvar MB_SCH_NAME    =             17\nvar MB_SCH_SEG_NAME   =          25\nvar MB_SCH_SEG_SETPOINT   =      33\nvar MB_SCH_SEG_RAMP_RATE    =    35\nvar MB_SCH_SEG_SOAK_TIME    =    36\nvar MB_SCH_SEG_SELECTED   =      37\nvar MB_SCH_SELECTED     =        38\nvar MB_CAL_TEMP_ACT_CH0   =      39 \nvar MB_CAL_TEMP_ACT_CH1     =    41\n\n\nif (msg.payload.topic == \"CMD-CHANGE_MODE\") {\n    delete msg.payload;\n    var mode = flow.get(\"kiln_01.MB_MODE\",'memoryOnly');\n    var maxModes = 3;\n    if (mode>=maxModes){\n        mode=1;\n    } else {\n        mode++;\n    }\n    msg.payload = {\"type\":\"Holding-Register\",\"Data\":{\"TagName\":\"MB_MODE\",\"TagAddr\":MB_MODE,\"TagVal\":mode}}\n    return msg;\n}\n\nif (msg.payload.topic == \"CMD-START_PROFILE\") {\n    delete msg.payload;\n    msg.payload = {\"type\":\"Coil\",\"Data\":{\"TagName\":\"MB_CMD_START_PROFILE\",\"TagAddr\":MB_CMD_START_PROFILE,\"TagVal\":true}}\n    return msg;\n}\n\nif (msg.payload.topic == \"CMD-STOP_PROFILE\") {\n    delete msg.payload;\n    msg.payload = {\"type\":\"Coil\",\"Data\":{\"TagName\":\"MB_CMD_STOP_PROFILE\",\"TagAddr\":MB_CMD_STOP_PROFILE,\"TagVal\":true}}\n    return msg;\n}\n\nif (msg.payload.topic == \"CMD-RELEASE_HOLD\") {\n    delete msg.payload;\n    msg.payload = {\"type\":\"Coil\",\"Data\":{\"TagName\":\"MB_CMD_HOLD_RELEASE\",\"TagAddr\":MB_CMD_HOLD_RELEASE,\"TagVal\":true}}\n    return msg;\n}\n\nif (msg.payload.topic == \"CMD-NEXT_SCHEDULE\") {\n    delete msg.payload;\n    var currentSchedule_n = flow.get(\"kiln_01.MB_CMD_SELECTED_SCHEDULE\",'memoryOnly');\n    var maxSchedules_n = flow.get(\"kiln_01.MB_NUMBER_OF_SCHEDULES\",'memoryOnly');\n    if (currentSchedule_n>=maxSchedules_n-1){\n        currentSchedule_n=0;\n    } else {\n        currentSchedule_n++;\n    }\n    msg.payload = {\"type\":\"Holding-Register\",\"Data\":{\"TagName\":\"MB_CMD_SELECTED_SCHEDULE\",\"TagAddr\":MB_CMD_SELECTED_SCHEDULE,\"TagVal\":currentSchedule_n}}\n    return msg;\n}\n\nif (msg.payload.topic == \"CMD-PREV_SCHEDULE\") {\n    delete msg.payload;\n    var currentSchedule_p = flow.get(\"kiln_01.MB_CMD_SELECTED_SCHEDULE\",'memoryOnly');\n    var maxSchedules_p = flow.get(\"kiln_01.MB_NUMBER_OF_SCHEDULES\",'memoryOnly');\n    if (currentSchedule_p<=0){\n        currentSchedule_p=maxSchedules_p-1;\n    } else {\n        currentSchedule_p--;\n    }\n    msg.payload = {\"type\":\"Holding-Register\",\"Data\":{\"TagName\":\"MB_CMD_SELECTED_SCHEDULE\",\"TagAddr\":MB_CMD_SELECTED_SCHEDULE,\"TagVal\":currentSchedule_p}}\n    return msg;\n}\n\n",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "x": 560,
        "y": 300,
        "wires": [
            [
                "06f63b677e22c148"
            ]
        ]
    },
    {
        "id": "06f63b677e22c148",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "Ouput",
        "links": [
            "295fd867.2591b8",
            "474707b82f72ecf7"
        ],
        "x": 735,
        "y": 300,
        "wires": []
    },
    {
        "id": "c26b11b8d672536a",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln/favicon.ico",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 220,
        "y": 740,
        "wires": [
            [
                "d0ecc491f87413c1"
            ]
        ]
    },
    {
        "id": "d0ecc491f87413c1",
        "type": "file in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "filename": "c:\\www\\Kiln\\favicon.ico",
        "format": "",
        "sendError": true,
        "x": 470,
        "y": 740,
        "wires": [
            [
                "245b2df1c8d2e1aa"
            ]
        ]
    },
    {
        "id": "245b2df1c8d2e1aa",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Set Headers",
        "rules": [
            {
                "t": "set",
                "p": "headers",
                "pt": "msg",
                "to": "{}",
                "tot": "json"
            },
            {
                "t": "set",
                "p": "headers.content-type",
                "pt": "msg",
                "to": "image/ico",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 730,
        "y": 740,
        "wires": [
            [
                "13a3671af7c78dea"
            ]
        ]
    },
    {
        "id": "13a3671af7c78dea",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 890,
        "y": 740,
        "wires": []
    },
    {
        "id": "508fd1f2e99ee56d",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln/apple-touch-icon.png",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 190,
        "y": 780,
        "wires": [
            [
                "96d64cbcd11be436"
            ]
        ]
    },
    {
        "id": "96d64cbcd11be436",
        "type": "file in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "filename": "c:\\www\\Kiln\\apple-touch-icon.png",
        "format": "",
        "sendError": true,
        "x": 500,
        "y": 780,
        "wires": [
            [
                "7ee303af2180b1ba"
            ]
        ]
    },
    {
        "id": "7ee303af2180b1ba",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Set Headers",
        "rules": [
            {
                "t": "set",
                "p": "headers",
                "pt": "msg",
                "to": "{}",
                "tot": "json"
            },
            {
                "t": "set",
                "p": "headers.content-type",
                "pt": "msg",
                "to": "image/png",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 730,
        "y": 780,
        "wires": [
            [
                "281c1e261997155e"
            ]
        ]
    },
    {
        "id": "281c1e261997155e",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 890,
        "y": 780,
        "wires": []
    },
    {
        "id": "829d460ec632142a",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln/favicon-16x16.png",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 200,
        "y": 820,
        "wires": [
            [
                "94541f3ac2119484"
            ]
        ]
    },
    {
        "id": "94541f3ac2119484",
        "type": "file in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "filename": "c:\\www\\Kiln\\favicon-16x16.png",
        "format": "",
        "sendError": true,
        "x": 490,
        "y": 820,
        "wires": [
            [
                "652d5da0605d6afe"
            ]
        ]
    },
    {
        "id": "652d5da0605d6afe",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Set Headers",
        "rules": [
            {
                "t": "set",
                "p": "headers",
                "pt": "msg",
                "to": "{}",
                "tot": "json"
            },
            {
                "t": "set",
                "p": "headers.content-type",
                "pt": "msg",
                "to": "image/png",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 730,
        "y": 820,
        "wires": [
            [
                "82c7f2742e4739b5"
            ]
        ]
    },
    {
        "id": "82c7f2742e4739b5",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 890,
        "y": 820,
        "wires": []
    },
    {
        "id": "d0aaf1be1948e535",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln/favicon-32x32.png",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 200,
        "y": 860,
        "wires": [
            [
                "b5787e44c93165b7"
            ]
        ]
    },
    {
        "id": "b5787e44c93165b7",
        "type": "file in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "filename": "c:\\www\\Kiln\\favicon-32x32.png",
        "format": "",
        "sendError": true,
        "x": 490,
        "y": 860,
        "wires": [
            [
                "f8c9ebb6fe568ea3"
            ]
        ]
    },
    {
        "id": "f8c9ebb6fe568ea3",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Set Headers",
        "rules": [
            {
                "t": "set",
                "p": "headers",
                "pt": "msg",
                "to": "{}",
                "tot": "json"
            },
            {
                "t": "set",
                "p": "headers.content-type",
                "pt": "msg",
                "to": "image/png",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 730,
        "y": 860,
        "wires": [
            [
                "899b3dc74a4eeb59"
            ]
        ]
    },
    {
        "id": "899b3dc74a4eeb59",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 890,
        "y": 860,
        "wires": []
    },
    {
        "id": "3b8b80b8814439f2",
        "type": "http in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "url": "/Kiln/site.webmanifest",
        "method": "get",
        "upload": false,
        "swaggerDoc": "",
        "x": 210,
        "y": 900,
        "wires": [
            [
                "61814a111161adca"
            ]
        ]
    },
    {
        "id": "61814a111161adca",
        "type": "file in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "filename": "c:\\www\\Kiln\\site.webmanifest",
        "format": "utf8",
        "sendError": true,
        "encoding": "utf8",
        "x": 490,
        "y": 900,
        "wires": [
            [
                "368df6756ef35b69"
            ]
        ]
    },
    {
        "id": "368df6756ef35b69",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Set Headers",
        "rules": [
            {
                "t": "set",
                "p": "headers",
                "pt": "msg",
                "to": "{}",
                "tot": "json"
            },
            {
                "t": "set",
                "p": "headers.content-type",
                "pt": "msg",
                "to": "text/site-manifest",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 730,
        "y": 900,
        "wires": [
            [
                "09515a5e4d3285d4"
            ]
        ]
    },
    {
        "id": "09515a5e4d3285d4",
        "type": "http response",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "statusCode": "",
        "headers": {},
        "x": 890,
        "y": 900,
        "wires": []
    },
    {
        "id": "085cbb8dcc2ac148",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Populate JSON",
        "func": "\nvar id1 = flow.get(\"kiln_01.MB_STS_SCHEDULE_NAME\", 'memoryOnly');\nvar id2 = flow.get(\"kiln_01.MB_STS_SEGMENT_NAME\", 'memoryOnly');\nvar id3 = flow.get(\"kiln_01.MB_STS_SEGMENT_STATE\", 'memoryOnly');\nvar id4 = flow.get(\"kiln_01.MB_STS_REMAINING_TIME\", 'memoryOnly');\nvar id5 = flow.get(\"kiln_01.MB_CMD_SETPOINT\", 'memoryOnly');\nvar id6 = flow.get(\"kiln_01.MB_STS_TEMPERATURE_01\", 'memoryOnly');\nvar id7 = flow.get(\"kiln_01.MB_STS_TEMPERATURE_02\", 'memoryOnly');\nvar id8 = flow.get(\"kiln_01.MB_MODE\", 'memoryOnly');\nvar id9 = flow.get(\"kiln_01.MB_STS_RELEASE_REQ\", 'memoryOnly');\nvar id10 = flow.get(\"kiln_01.MB_STS_THERMAL_RUNAWAY\", 'memoryOnly');\nvar id11 = flow.get(\"kiln_01.MB_STS_SAFETY_OK\", 'memoryOnly');\nvar id12 = flow.get(\"kiln_01.MB_HEARTBEAT\", 'memoryOnly');\nvar id13 = flow.get(\"kiln_01.MB_CMD_THERM_OVERRIDE\", 'memoryOnly');\n\nmsg.payload = '{\"topic\":\"status\",\"data\":{\"id1\":\"'+ id1 +'\",\"id2\":\"'+ id2 +'\",\"id3\":'+ id3 +',\"id4\":\"'+ id4 +'\",\"id5\":'+ Math.round(id5*100)/100 +',\"id6\":'+ Math.round(id6*100)/100  +',\"id7\":'+ Math.round(id7*100)/100  +',\"id8\":'+ id8 +',\"id9\":'+ id9 +',\"id10\":'+ id10 +',\"id11\":'+ id11 +',\"id13\":'+ id13 +'}}';\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 340,
        "y": 160,
        "wires": [
            [
                "e4a94591518297f5"
            ]
        ]
    },
    {
        "id": "c455a15a0ba8359e",
        "type": "inject",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "3",
        "crontab": "",
        "once": true,
        "onceDelay": "10",
        "topic": "",
        "payloadType": "date",
        "x": 1330,
        "y": 880,
        "wires": [
            [
                "d3ed67dc6c8d3f99"
            ]
        ]
    },
    {
        "id": "301b42111e5f2eb2",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "409f97fe.d2701",
            "31b130f940682e38"
        ],
        "x": 2575,
        "y": 2260,
        "wires": []
    },
    {
        "id": "65a977a157501d02",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln in Hold Message",
        "func": "var topic = \"Kiln - Status\";\nvar payload = \"The Kiln is requesting to release the hold.\";\nvar msg_out = {\"payload\":payload, \"topic\":topic};\nreturn msg_out;",
        "outputs": 1,
        "noerr": 0,
        "x": 2500,
        "y": 720,
        "wires": [
            [
                "084c443643309420"
            ]
        ]
    },
    {
        "id": "48c9213f42747ac4",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMPERATURE_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "measurement",
                "pt": "msg",
                "to": "kiln_upper_temperature_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "converted.4",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 2260,
        "wires": [
            [
                "ad5041a63c4a18e3",
                "301b42111e5f2eb2"
            ]
        ]
    },
    {
        "id": "f663af8e72e4088c",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMPERATURE_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "measurement",
                "pt": "msg",
                "to": "kiln_lower_temperature_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "converted.5",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 2280,
        "wires": [
            [
                "2c95d4595fd881d1",
                "301b42111e5f2eb2"
            ]
        ]
    },
    {
        "id": "ad5041a63c4a18e3",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "deadbandEq",
        "gap": "2",
        "start": "",
        "inout": "out",
        "property": "payload.TagValue",
        "x": 2610,
        "y": 2340,
        "wires": [
            []
        ]
    },
    {
        "id": "2c95d4595fd881d1",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "deadbandEq",
        "gap": "2",
        "start": "",
        "inout": "out",
        "property": "payload.TagValue",
        "x": 2610,
        "y": 2360,
        "wires": [
            []
        ]
    },
    {
        "id": "140284ba8fbe21a9",
        "type": "comment",
        "z": "2df22b78ebc5ab05",
        "name": "INPUT STATUS MAPPING",
        "info": "\nMB_STS_SSR_01 = 1 (payload array index 0)\nMB_STS_SSR_02 = 2 (payload array index 1)\nMB_STS_RELEASE_REQ = 3 (payload array index 2)\nMB_STS_SAFETY_OK = 4 (payload array index 3)\nMB_STS_IN_PROCESS = 5 (payload array index 4)\nMB_STS_THERMAL_RUNAWAY = 6 (payload array index 5)\nMB_STS_EEPROM_WRITTEN = 7 (payload array index 6)",
        "x": 1750,
        "y": 500,
        "wires": []
    },
    {
        "id": "28c214f601bacb08",
        "type": "switch",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_RELEASE_REQ",
        "property": "payload.2",
        "propertyType": "msg",
        "rules": [
            {
                "t": "true"
            },
            {
                "t": "false"
            }
        ],
        "checkall": "true",
        "repair": false,
        "outputs": 2,
        "x": 2150,
        "y": 720,
        "wires": [
            [
                "65a977a157501d02"
            ],
            []
        ]
    },
    {
        "id": "8cee67eeabb3a6e8",
        "type": "switch",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SAFETY_OK",
        "property": "payload.3",
        "propertyType": "msg",
        "rules": [
            {
                "t": "false"
            },
            {
                "t": "true"
            }
        ],
        "checkall": "true",
        "repair": false,
        "outputs": 2,
        "x": 2140,
        "y": 740,
        "wires": [
            [
                "fca959118defe4e8"
            ],
            []
        ]
    },
    {
        "id": "2b3b8019454f6ec1",
        "type": "switch",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_IN_PROCESS",
        "property": "payload.4",
        "propertyType": "msg",
        "rules": [
            {
                "t": "true"
            },
            {
                "t": "false"
            }
        ],
        "checkall": "true",
        "repair": false,
        "outputs": 2,
        "x": 2150,
        "y": 760,
        "wires": [
            [
                "c7175cc969bd97d1"
            ],
            []
        ]
    },
    {
        "id": "13168ebe5f8a1bd7",
        "type": "switch",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_THERMAL_RUNAWAY",
        "property": "payload.5",
        "propertyType": "msg",
        "rules": [
            {
                "t": "true"
            },
            {
                "t": "false"
            }
        ],
        "checkall": "true",
        "repair": false,
        "outputs": 2,
        "x": 2170,
        "y": 780,
        "wires": [
            [
                "f4ebb8c9282b0eed"
            ],
            []
        ]
    },
    {
        "id": "af8aa154d9cfef6b",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "rbe",
        "gap": "",
        "start": "",
        "inout": "out",
        "property": "payload.3",
        "x": 1950,
        "y": 740,
        "wires": [
            [
                "8cee67eeabb3a6e8"
            ]
        ]
    },
    {
        "id": "c8362016c3e6378f",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "rbe",
        "gap": "",
        "start": "",
        "inout": "out",
        "property": "payload.4",
        "x": 1950,
        "y": 760,
        "wires": [
            [
                "2b3b8019454f6ec1"
            ]
        ]
    },
    {
        "id": "9a4903da55b6532e",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "rbe",
        "gap": "",
        "start": "",
        "inout": "out",
        "property": "payload.5",
        "x": 1950,
        "y": 780,
        "wires": [
            [
                "13168ebe5f8a1bd7"
            ]
        ]
    },
    {
        "id": "fca959118defe4e8",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln Safety Not Ok",
        "func": "var topic = \"!!! Kiln - Status !!!\";\nvar payload = \"The Kiln safety circuit in not ok!\";\nvar msg_out = {\"payload\":payload, \"topic\":topic};\nreturn msg_out;",
        "outputs": 1,
        "noerr": 0,
        "x": 2490,
        "y": 740,
        "wires": [
            [
                "084c443643309420"
            ]
        ]
    },
    {
        "id": "c7175cc969bd97d1",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln Schedule In Process",
        "func": "var topic = \"Kiln - Status\";\nvar payload = \"The Kiln has started a schedule\";\nvar msg_out = {\"payload\":payload, \"topic\":topic};\nreturn msg_out;",
        "outputs": 1,
        "noerr": 0,
        "x": 2510,
        "y": 760,
        "wires": [
            [
                "084c443643309420"
            ]
        ]
    },
    {
        "id": "f4ebb8c9282b0eed",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln Thermal Runaway",
        "func": "var topic = \"!!! Kiln - Status !!!\";\nvar payload = \"The Kiln has detected a thermal runaway condition!\";\nvar msg_out = {\"payload\":payload, \"topic\":topic};\nreturn msg_out;",
        "outputs": 1,
        "noerr": 0,
        "x": 2500,
        "y": 780,
        "wires": [
            [
                "084c443643309420"
            ]
        ]
    },
    {
        "id": "d85ad30e11bf7c81",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "c9a8f98e37d8cd58"
        ],
        "x": 2415,
        "y": 560,
        "wires": []
    },
    {
        "id": "b40405996b034ac2",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_RELEASE_REQ",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_RELEASE_REQ",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2170,
        "y": 580,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "1b628c2bb3b5d690",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SAFETY_OK",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SAFETY_OK",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2160,
        "y": 600,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "baaeb197f24940d2",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_IN_PROCESS",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_IN_PROCESS",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.4",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2170,
        "y": 620,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "a0335f39b7a74374",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_THERMAL_RUNAWAY",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_THERMAL_RUNAWAY",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.5",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2190,
        "y": 640,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "6ac37c4128e09344",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "c9a8f98e37d8cd58"
        ],
        "x": 2855,
        "y": 1840,
        "wires": []
    },
    {
        "id": "cbf122499d2c701b",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Schedule Name",
        "func": "// need to make a copy of the buffer here\n// node-red 1.0 doesnt clone the msg object like it used to\n// https://nodered.org/blog/2019/09/13/cloning-messages\nlet bufcopy = Buffer.alloc(64) //allocate 64 bytes\nmsg.responseBuffer.buffer.copy(bufcopy)\nvar swapped = bufcopy.swap16();\nmsg.converted = swapped.toString('utf8',46,61)\nmsg.stripped = msg.converted.replace(/[^0-9a-z ]/gi, '') // remove non alpha-numeric characters\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2060,
        "y": 1980,
        "wires": [
            [
                "99cff620516f120e"
            ]
        ]
    },
    {
        "id": "99cff620516f120e",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SCHEDULE_NAME",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SCHEDULE_NAME",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "stripped",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 1980,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "632d7555a917b664",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Segment Name",
        "func": "// need to make a copy of the buffer here\n// node-red 1.0 doesnt clone the msg object like it used to\n// https://nodered.org/blog/2019/09/13/cloning-messages\nlet bufcopy = Buffer.alloc(64) //allocate 64 bytes\nmsg.responseBuffer.buffer.copy(bufcopy)\nvar swapped = bufcopy.swap16();\nmsg.converted = swapped.toString('utf8',30,45)\nmsg.stripped = msg.converted.replace(/[^0-9a-z ]/gi, '') // remove non alpha-numeric characters\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2060,
        "y": 2000,
        "wires": [
            [
                "0d6243cf7d1037b1"
            ]
        ]
    },
    {
        "id": "0d6243cf7d1037b1",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SEGMENT_NAME",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SEGMENT_NAME",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "stripped",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 2000,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "3f1d90b525423184",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_EEPROM_WRITTEN",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_EEPROM_WRITTEN",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.6",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2190,
        "y": 660,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "c870591aeeb4f43d",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SSR_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SSR_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.0",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2150,
        "y": 540,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "015c6e455a343145",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SSR_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SSR_02",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.1",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2150,
        "y": 560,
        "wires": [
            [
                "d85ad30e11bf7c81"
            ]
        ]
    },
    {
        "id": "7df63e209e828cd7",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_SELECT_SCHEDULE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_SELECT_SCHEDULE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.0",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2130,
        "y": 100,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "5103dbb2ec350c68",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_START_PROFILE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_START_PROFILE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.1",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 120,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "5ccb425de6cd3043",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_STOP_PROFILE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_STOP_PROFILE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 140,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "226b23e84daa9e7f",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_HOLD_RELEASE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_HOLD_RELEASE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 160,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "f5b69bd318d4d12d",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_THERM_OVERRIDE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_THERM_OVERRIDE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.4",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2130,
        "y": 180,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "28f7f47ec4030e5c",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_WRITE_EEPROM",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_WRITE_EEPROM",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.5",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 200,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "df82c52621b2a2b2",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_ENABLED",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_ENABLED",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.6",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 220,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "fd45a7ec263909ff",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_HOLD_EN",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_HOLD_EN",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.7",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 240,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "90b30e739f7cef61",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_CAL_CH0_LOW",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_CAL_CH0_LOW",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.8",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 260,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "f367822b99343d89",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_CAL_CH1_LOW",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_CAL_CH1_LOW",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.9",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 280,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "51f0d2431d18a72e",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_CAL_CH0_HIGH",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_CAL_CH0_HIGH",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.10",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 300,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "d2a651c758075472",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_CAL_CH1_HIGH",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_CAL_CH1_HIGH",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.11",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2120,
        "y": 320,
        "wires": [
            [
                "dd50473f43c42c8d"
            ]
        ]
    },
    {
        "id": "dd50473f43c42c8d",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "c9a8f98e37d8cd58"
        ],
        "x": 2435,
        "y": 200,
        "wires": []
    },
    {
        "id": "a07e870b4eaf9c94",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_HEARTBEAT",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_HEARTBEAT",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data[0]",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2310,
        "y": 1700,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "25343c3b117928c0",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_REMAINING_TIME_H",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_REMAINING_TIME_H",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.1",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1720,
        "wires": [
            []
        ]
    },
    {
        "id": "51078bb3e9343385",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_REMAINING_TIME_M",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_REMAINING_TIME_M",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1740,
        "wires": [
            [
                "5e5526fcd1c7c685"
            ]
        ]
    },
    {
        "id": "7c2a073047dae242",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_REMAINING_TIME_S",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_REMAINING_TIME_S",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1760,
        "wires": [
            []
        ]
    },
    {
        "id": "81cfd08a8f30221a",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_PID_01_OUTPUT",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_PID_01_OUTPUT",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.6",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 1840,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "5ec89d0bdf2ff66f",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_PID_02_OUTPUT",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_PID_02_OUTPUT",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.7",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 1860,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "69baef954d3b19b2",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_NUMBER_OF_SCHEDULES",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_NUMBER_OF_SCHEDULES",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.12",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1900,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "f0e3d2d223c6e82a",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_NUMBER_OF_SEGMENTS",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_NUMBER_OF_SEGMENTS",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.13",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1920,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "5590e4743e5cde05",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_SEGMENT_STATE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_SEGMENT_STATE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.14",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 1940,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "8c7b4b8cc6a2a9c4",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMP_01_RAW",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_TEMP_01_RAW",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 2040,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "e403958956779767",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMP_02_RAW",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_TEMP_02_RAW",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 2060,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "66973e9639eb6f4f",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 8;\nvar start = 4;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2060,
        "y": 1840,
        "wires": [
            [
                "48c9213f42747ac4",
                "f663af8e72e4088c",
                "81cfd08a8f30221a",
                "5ec89d0bdf2ff66f",
                "59a5e2e830055729",
                "dd58dd7d102b7d2c"
            ]
        ]
    },
    {
        "id": "401f69780d9e6e48",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 4;\nvar start = 31;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2060,
        "y": 2060,
        "wires": [
            [
                "8c7b4b8cc6a2a9c4",
                "e403958956779767"
            ]
        ]
    },
    {
        "id": "19049100a5e686cb",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 14;\nvar start = 2;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2130,
        "y": 1160,
        "wires": [
            [
                "dceecfa71e978185",
                "772dfb70f7cd4222",
                "3dd4926b5bfac629",
                "7cced67b02c7c9ba",
                "7688ef754fc8234d",
                "70aaf527680f367b",
                "a7045f72c08f678c"
            ]
        ]
    },
    {
        "id": "f6b01bf28bfb0364",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_MODE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_MODE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.0",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1020,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "49978cdc9c752b63",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_SELECTED_SCHEDULE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_SELECTED_SCHEDULE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.1",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2420,
        "y": 1040,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "dceecfa71e978185",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CMD_SETPOINT",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CMD_SETPOINT",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.7",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2380,
        "y": 1080,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "772dfb70f7cd4222",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_P_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_P_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.8",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1100,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "3dd4926b5bfac629",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_I_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_I_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.9",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1120,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "7cced67b02c7c9ba",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_D_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_D_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.10",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1140,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "7688ef754fc8234d",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_P_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_P_02",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.11",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1160,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "70aaf527680f367b",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_I_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_I_02",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.12",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1180,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "a7045f72c08f678c",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_PID_D_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_PID_D_02",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.13",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2360,
        "y": 1200,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "5abe1f1dd3e16716",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "c9a8f98e37d8cd58"
        ],
        "x": 2755,
        "y": 1180,
        "wires": []
    },
    {
        "id": "93e56826b34e0533",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Schedule Name",
        "func": "// need to make a copy of the buffer here\n// node-red 1.0 doesnt clone the msg object like it used to\n// https://nodered.org/blog/2019/09/13/cloning-messages\nlet bufcopy = Buffer.alloc(64) //allocate 64 bytes\nmsg.responseBuffer.buffer.copy(bufcopy)\nvar swapped = bufcopy.swap16();\nmsg.converted = swapped.toString('utf8',32,47)\nmsg.stripped = msg.converted.replace(/[^0-9a-z ]/gi, '') // remove non alpha-numeric characters\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2140,
        "y": 1240,
        "wires": [
            [
                "bfa8a99dd5e42315"
            ]
        ]
    },
    {
        "id": "bfa8a99dd5e42315",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_NAME",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_NAME",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "stripped",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2370,
        "y": 1240,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "14dcfdf63a558763",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Segment Name",
        "func": "// need to make a copy of the buffer here\n// node-red 1.0 doesnt clone the msg object like it used to\n// https://nodered.org/blog/2019/09/13/cloning-messages\nlet bufcopy = Buffer.alloc(64) //allocate 64 bytes\nmsg.responseBuffer.buffer.copy(bufcopy)\nvar swapped = bufcopy.swap16();\nmsg.converted = swapped.toString('utf8',48,63)\nmsg.stripped = msg.converted.replace(/[^0-9a-z ]/gi, '') // remove non alpha-numeric characters\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 2140,
        "y": 1260,
        "wires": [
            [
                "2805b742a3c4045b"
            ]
        ]
    },
    {
        "id": "2805b742a3c4045b",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_NAME",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_NAME",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "stripped",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2380,
        "y": 1260,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "3678d9a7d8ed2318",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 2;\nvar start = 32;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2150,
        "y": 1300,
        "wires": [
            [
                "e567e98a3199e788"
            ]
        ]
    },
    {
        "id": "e567e98a3199e788",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_SETPOINT",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_SETPOINT",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.1",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1300,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "e3437a0ebc8beed6",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_RAMP_RATE",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_RAMP_RATE",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.34",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1340,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "1f16f8d311a29797",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_SOAK_TIME",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_SOAK_TIME",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.35",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1360,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "57bd853af69f0a6d",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SEG_SELECTED",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SEG_SELECTED",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.36",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1380,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "b42808da37c3de75",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_SCH_SELECTED",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_SCH_SELECTED",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "responseBuffer.data.37",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2380,
        "y": 1400,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "3e63b7e2bccfbce4",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 4;\nvar start = 38;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2150,
        "y": 1440,
        "wires": [
            [
                "600f97b0f98c4ff7",
                "3bdcb18ec5ab5284"
            ]
        ]
    },
    {
        "id": "600f97b0f98c4ff7",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CAL_TEMP_ACT_CH0",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CAL_TEMP_ACT_CH0",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1440,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "3bdcb18ec5ab5284",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_CAL_TEMP_ACT_CH1",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_CAL_TEMP_ACT_CH1",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2400,
        "y": 1460,
        "wires": [
            [
                "5abe1f1dd3e16716"
            ]
        ]
    },
    {
        "id": "bd607d715124bdb0",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Concat Time",
        "func": "\nvar hour = 0;\nif (msg.responseBuffer.data[1] < 10) {\n    hour = '0' + msg.responseBuffer.data[1];\n} else {\n    hour = msg.responseBuffer.data[1];\n}\n\nvar min = 0;\nif (msg.responseBuffer.data[2] < 10) {\n    min = '0' + msg.responseBuffer.data[2];\n} else {\n    min = msg.responseBuffer.data[2];\n}\n\nvar sec = 0;\nif (msg.responseBuffer.data[3] < 10) {\n    sec = '0' + msg.responseBuffer.data[3];\n} else {\n    sec = msg.responseBuffer.data[3];\n}\n\ndelete msg.payload;\nmsg.payload = {\"TagName\":\"MB_STS_REMAINING_TIME\",\"TagValue\": hour + ':' + min + ':' + sec};\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2730,
        "y": 1740,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "59a5e2e830055729",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMPERATURE_01",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_TEMPERATURE_01",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.4",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1780,
        "wires": [
            [
                "575745f1c105a8b3"
            ]
        ]
    },
    {
        "id": "dd58dd7d102b7d2c",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_TEMPERATURE_02",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_TEMPERATURE_02",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.5",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 1800,
        "wires": [
            [
                "a5b657b141245bb3"
            ]
        ]
    },
    {
        "id": "474707b82f72ecf7",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "Input - Kiln",
        "links": [
            "06f63b677e22c148"
        ],
        "x": 1680,
        "y": 2580,
        "wires": [
            [
                "cedf67bb8f49b107"
            ]
        ]
    },
    {
        "id": "cedf67bb8f49b107",
        "type": "switch",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "property": "payload.type",
        "propertyType": "msg",
        "rules": [
            {
                "t": "eq",
                "v": "Coil",
                "vt": "str"
            },
            {
                "t": "eq",
                "v": "Holding-Register",
                "vt": "str"
            }
        ],
        "checkall": "true",
        "repair": false,
        "outputs": 2,
        "x": 1880,
        "y": 2580,
        "wires": [
            [
                "54e6a50f05ca8781"
            ],
            [
                "48ed65c4be19e9d5"
            ]
        ]
    },
    {
        "id": "675f3ac92877860c",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "Set Message",
        "func": "\nmsg.payload = { value: msg.payload.Data.TagVal, 'fc': msg.payload.Data.fc, 'unitid': 1, 'address': msg.payload.Data.TagAddr , 'quantity': 1 };\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2290,
        "y": 2580,
        "wires": [
            [
                "f775f71255ded8d1"
            ]
        ]
    },
    {
        "id": "48ed65c4be19e9d5",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Holding Registers",
        "rules": [
            {
                "t": "set",
                "p": "payload.Data.fc",
                "pt": "msg",
                "to": "6",
                "tot": "num"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2050,
        "y": 2600,
        "wires": [
            [
                "675f3ac92877860c"
            ]
        ]
    },
    {
        "id": "54e6a50f05ca8781",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "Coils",
        "rules": [
            {
                "t": "set",
                "p": "payload.Data.fc",
                "pt": "msg",
                "to": "5",
                "tot": "num"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2010,
        "y": 2560,
        "wires": [
            [
                "675f3ac92877860c"
            ]
        ]
    },
    {
        "id": "5e5526fcd1c7c685",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "deadbandEq",
        "gap": "50%",
        "start": "",
        "inout": "out",
        "property": "payload.TagValue",
        "x": 2570,
        "y": 1740,
        "wires": [
            [
                "bd607d715124bdb0"
            ]
        ]
    },
    {
        "id": "575745f1c105a8b3",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "deadbandEq",
        "gap": "2",
        "start": "",
        "inout": "out",
        "property": "payload.TagValue",
        "x": 2570,
        "y": 1780,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "a5b657b141245bb3",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "deadbandEq",
        "gap": "2",
        "start": "",
        "inout": "out",
        "property": "payload.TagValue",
        "x": 2570,
        "y": 1800,
        "wires": [
            [
                "6ac37c4128e09344"
            ]
        ]
    },
    {
        "id": "6c99449773b22693",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_FLOAT",
        "func": "var arrSize = 4;\nvar start = 35;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\nvar fl32 = new Float32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Float32Array.BYTES_PER_ELEMENT);\nmsg.converted = fl32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2060,
        "y": 2180,
        "wires": [
            [
                "6254f8ed2c6a7b74",
                "4ce4f8a3d9fdfa25"
            ]
        ]
    },
    {
        "id": "6254f8ed2c6a7b74",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_MEAS_RATE_CH0",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_MEAS_RATE_CH0",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 2160,
        "wires": [
            []
        ]
    },
    {
        "id": "4ce4f8a3d9fdfa25",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_MEAS_RATE_CH1",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_MEAS_RATE_CH1",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2340,
        "y": 2180,
        "wires": [
            []
        ]
    },
    {
        "id": "a8c51d3dd60903fc",
        "type": "function",
        "z": "2df22b78ebc5ab05",
        "name": "UINT16_TO_UINT32",
        "func": "var arrSize = 4;\nvar start = 35;\nlet arr = new Array(arrSize);\nvar i;\nfor (i=0;i<arrSize;i++) {\n   arr.push(msg.payload[i+start]); \n}\n\nvar ui16 = new Uint16Array(arr);\n\nvar ui32 = new Uint32Array(ui16.buffer, ui16.byteOffset, ui16.byteLength / Uint32Array.BYTES_PER_ELEMENT)\n\nmsg.converted = ui32;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "x": 2060,
        "y": 2120,
        "wires": [
            [
                "ad87d49574f4d5ae",
                "861da6e8ad7becd9"
            ]
        ]
    },
    {
        "id": "ad87d49574f4d5ae",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_RUNAWAY_TEMP_T",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_RUNAWAY_TEMP_T",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.2",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 2100,
        "wires": [
            []
        ]
    },
    {
        "id": "861da6e8ad7becd9",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "MB_STS_RUNAWAY_RATE_T",
        "rules": [
            {
                "t": "delete",
                "p": "payload",
                "pt": "msg"
            },
            {
                "t": "set",
                "p": "payload.TagName",
                "pt": "msg",
                "to": "MB_STS_RUNAWAY_RATE_T",
                "tot": "str"
            },
            {
                "t": "set",
                "p": "payload.TagValue",
                "pt": "msg",
                "to": "converted.3",
                "tot": "msg"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2350,
        "y": 2120,
        "wires": [
            []
        ]
    },
    {
        "id": "a8b78d02e3c7cf45",
        "type": "rbe",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "func": "rbe",
        "gap": "",
        "start": "",
        "inout": "out",
        "property": "payload.2",
        "x": 1950,
        "y": 720,
        "wires": [
            [
                "28c214f601bacb08"
            ]
        ]
    },
    {
        "id": "084c443643309420",
        "type": "e-mail",
        "z": "2df22b78ebc5ab05",
        "server": "smtp.gmail.com",
        "port": "465",
        "secure": true,
        "tls": false,
        "name": "address@mail.com",
        "dname": "GMail",
        "x": 2810,
        "y": 740,
        "wires": []
    },
    {
        "id": "1994475d2c581258",
        "type": "modbus-getter",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln-Coil-Status",
        "showStatusActivities": true,
        "showErrors": true,
        "logIOActivities": false,
        "unitid": "",
        "dataType": "Coil",
        "adr": "1",
        "quantity": "12",
        "server": "730a18e6.6782b",
        "useIOFile": false,
        "ioFile": "",
        "useIOForPayload": false,
        "emptyMsgOnFail": false,
        "keepMsgProperties": false,
        "x": 1720,
        "y": 280,
        "wires": [
            [
                "8444bb7dd6cdef8a"
            ],
            []
        ]
    },
    {
        "id": "d107d9a3429a2ba7",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "8444bb7dd6cdef8a"
        ],
        "x": 1795,
        "y": 200,
        "wires": [
            [
                "7df63e209e828cd7",
                "5103dbb2ec350c68",
                "5ccb425de6cd3043",
                "226b23e84daa9e7f",
                "f5b69bd318d4d12d",
                "28f7f47ec4030e5c",
                "df82c52621b2a2b2",
                "fd45a7ec263909ff",
                "90b30e739f7cef61",
                "f367822b99343d89",
                "51f0d2431d18a72e",
                "d2a651c758075472",
                "8f160a4ee8250b77"
            ]
        ]
    },
    {
        "id": "8444bb7dd6cdef8a",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "d107d9a3429a2ba7"
        ],
        "x": 1735,
        "y": 200,
        "wires": []
    },
    {
        "id": "80c29a9161906f93",
        "type": "modbus-getter",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln-Input-Status",
        "showStatusActivities": true,
        "showErrors": true,
        "logIOActivities": false,
        "unitid": "",
        "dataType": "Input",
        "adr": "1",
        "quantity": "8",
        "server": "730a18e6.6782b",
        "useIOFile": false,
        "ioFile": "",
        "useIOForPayload": false,
        "emptyMsgOnFail": false,
        "keepMsgProperties": false,
        "x": 1730,
        "y": 540,
        "wires": [
            [
                "c522c89f6e35e5d5"
            ],
            []
        ]
    },
    {
        "id": "f61ee51608b9e2d4",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "c522c89f6e35e5d5"
        ],
        "x": 1775,
        "y": 660,
        "wires": [
            [
                "c870591aeeb4f43d",
                "015c6e455a343145",
                "b40405996b034ac2",
                "1b628c2bb3b5d690",
                "baaeb197f24940d2",
                "a0335f39b7a74374",
                "3f1d90b525423184",
                "a8b78d02e3c7cf45",
                "af8aa154d9cfef6b",
                "c8362016c3e6378f",
                "9a4903da55b6532e",
                "af016c9a43ee5c9e"
            ]
        ]
    },
    {
        "id": "c522c89f6e35e5d5",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "f61ee51608b9e2d4"
        ],
        "x": 1895,
        "y": 540,
        "wires": []
    },
    {
        "id": "b80bbf11300c18b0",
        "type": "modbus-getter",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln-Holding-Registers",
        "showStatusActivities": true,
        "showErrors": true,
        "logIOActivities": false,
        "unitid": "",
        "dataType": "HoldingRegister",
        "adr": "1",
        "quantity": "42",
        "server": "730a18e6.6782b",
        "useIOFile": false,
        "ioFile": "",
        "useIOForPayload": false,
        "emptyMsgOnFail": false,
        "keepMsgProperties": false,
        "x": 1740,
        "y": 1040,
        "wires": [
            [
                "0ffa535d1eaea620"
            ],
            []
        ]
    },
    {
        "id": "716fb9896864285e",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "0ffa535d1eaea620"
        ],
        "x": 1655,
        "y": 1280,
        "wires": [
            [
                "f6b01bf28bfb0364",
                "49978cdc9c752b63",
                "19049100a5e686cb",
                "93e56826b34e0533",
                "14dcfdf63a558763",
                "3678d9a7d8ed2318",
                "3e63b7e2bccfbce4",
                "e3437a0ebc8beed6",
                "1f16f8d311a29797",
                "57bd853af69f0a6d",
                "b42808da37c3de75",
                "5f4e5f0a24b0d7b6"
            ]
        ]
    },
    {
        "id": "0ffa535d1eaea620",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "716fb9896864285e"
        ],
        "x": 1915,
        "y": 1040,
        "wires": []
    },
    {
        "id": "1994a1ba2122a989",
        "type": "modbus-getter",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln-Input-Registers",
        "showStatusActivities": true,
        "showErrors": true,
        "logIOActivities": false,
        "unitid": "",
        "dataType": "InputRegister",
        "adr": "1",
        "quantity": "43",
        "server": "730a18e6.6782b",
        "useIOFile": false,
        "ioFile": "",
        "useIOForPayload": false,
        "emptyMsgOnFail": false,
        "keepMsgProperties": false,
        "x": 1760,
        "y": 1660,
        "wires": [
            [
                "9b2c3dea7594fe94"
            ],
            []
        ]
    },
    {
        "id": "9b2c3dea7594fe94",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "a0b6087a56733e32"
        ],
        "x": 1925,
        "y": 1660,
        "wires": []
    },
    {
        "id": "a0b6087a56733e32",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "9b2c3dea7594fe94"
        ],
        "x": 1725,
        "y": 1820,
        "wires": [
            [
                "a07e870b4eaf9c94",
                "25343c3b117928c0",
                "51078bb3e9343385",
                "7c2a073047dae242",
                "66973e9639eb6f4f",
                "69baef954d3b19b2",
                "f0e3d2d223c6e82a",
                "5590e4743e5cde05",
                "cbf122499d2c701b",
                "632d7555a917b664",
                "401f69780d9e6e48",
                "a8c51d3dd60903fc",
                "6c99449773b22693",
                "1a0d17101390aa98"
            ]
        ]
    },
    {
        "id": "f775f71255ded8d1",
        "type": "modbus-flex-write",
        "z": "2df22b78ebc5ab05",
        "name": "Kiln",
        "showStatusActivities": true,
        "showErrors": true,
        "server": "730a18e6.6782b",
        "emptyMsgOnFail": false,
        "keepMsgProperties": false,
        "x": 2470,
        "y": 2580,
        "wires": [
            [
                "b175aa811dacde30"
            ],
            [
                "b175aa811dacde30"
            ]
        ]
    },
    {
        "id": "5f4e5f0a24b0d7b6",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 2150,
        "y": 1500,
        "wires": []
    },
    {
        "id": "8f160a4ee8250b77",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 2100,
        "y": 380,
        "wires": []
    },
    {
        "id": "1a0d17101390aa98",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 2290,
        "y": 1660,
        "wires": []
    },
    {
        "id": "ae34f53a7bee8402",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "d3ed67dc6c8d3f99"
        ],
        "x": 1575,
        "y": 280,
        "wires": [
            [
                "1994475d2c581258"
            ]
        ]
    },
    {
        "id": "d518ac90e1b193a4",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "d3ed67dc6c8d3f99"
        ],
        "x": 1575,
        "y": 540,
        "wires": [
            [
                "80c29a9161906f93"
            ]
        ]
    },
    {
        "id": "4c1232f753350a69",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "d3ed67dc6c8d3f99"
        ],
        "x": 1575,
        "y": 1040,
        "wires": [
            [
                "b80bbf11300c18b0"
            ]
        ]
    },
    {
        "id": "cb1106dcb2ada1aa",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "d3ed67dc6c8d3f99"
        ],
        "x": 1575,
        "y": 1660,
        "wires": [
            [
                "1994a1ba2122a989"
            ]
        ]
    },
    {
        "id": "d3ed67dc6c8d3f99",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "4c1232f753350a69",
            "cb1106dcb2ada1aa",
            "d518ac90e1b193a4",
            "ae34f53a7bee8402"
        ],
        "x": 1455,
        "y": 880,
        "wires": []
    },
    {
        "id": "af016c9a43ee5c9e",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 2130,
        "y": 500,
        "wires": []
    },
    {
        "id": "b175aa811dacde30",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "false",
        "statusVal": "",
        "statusType": "auto",
        "x": 2680,
        "y": 2580,
        "wires": []
    },
    {
        "id": "043d67cef12b7030",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "true",
        "targetType": "full",
        "statusVal": "",
        "statusType": "auto",
        "x": 490,
        "y": 620,
        "wires": []
    },
    {
        "id": "4b5fa5220e6f11ce",
        "type": "influxdb out",
        "z": "2df22b78ebc5ab05",
        "influxdb": "1a7d214ba0ada216",
        "name": "",
        "measurement": "",
        "precision": "",
        "retentionPolicy": "",
        "database": "home",
        "precisionV18FluxV20": "ms",
        "retentionPolicyV18Flux": "52w",
        "org": "organisation",
        "bucket": "bucket",
        "x": 3000,
        "y": 2260,
        "wires": []
    },
    {
        "id": "d91270a882fd1f25",
        "type": "inject",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 2460,
        "y": 2820,
        "wires": [
            [
                "7c97d8bb2c18e93a"
            ]
        ]
    },
    {
        "id": "7c97d8bb2c18e93a",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "rules": [
            {
                "t": "set",
                "p": "payload",
                "pt": "msg",
                "to": "100.23",
                "tot": "num"
            },
            {
                "t": "set",
                "p": "measurement",
                "pt": "msg",
                "to": "test_measurement",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2660,
        "y": 2820,
        "wires": [
            [
                "a706a207c63e7192"
            ]
        ]
    },
    {
        "id": "f786dccfafaa285f",
        "type": "influxdb in",
        "z": "2df22b78ebc5ab05",
        "influxdb": "1a7d214ba0ada216",
        "name": "",
        "query": "",
        "rawOutput": false,
        "precision": "",
        "retentionPolicy": "",
        "org": "organisation",
        "x": 2920,
        "y": 2880,
        "wires": [
            [
                "394d30b59ab758a1"
            ]
        ]
    },
    {
        "id": "0ad2b757faf664b9",
        "type": "inject",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "props": [
            {
                "p": "payload"
            },
            {
                "p": "topic",
                "vt": "str"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 2460,
        "y": 2880,
        "wires": [
            [
                "69a306f72c16bcf5"
            ]
        ]
    },
    {
        "id": "394d30b59ab758a1",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "true",
        "targetType": "full",
        "statusVal": "",
        "statusType": "auto",
        "x": 3160,
        "y": 2880,
        "wires": []
    },
    {
        "id": "69a306f72c16bcf5",
        "type": "change",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "rules": [
            {
                "t": "set",
                "p": "query",
                "pt": "msg",
                "to": "select * from kiln_upper_temperature_01",
                "tot": "str"
            }
        ],
        "action": "",
        "property": "",
        "from": "",
        "to": "",
        "reg": false,
        "x": 2660,
        "y": 2880,
        "wires": [
            [
                "f786dccfafaa285f"
            ]
        ]
    },
    {
        "id": "31b130f940682e38",
        "type": "link in",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "301b42111e5f2eb2",
            "a706a207c63e7192"
        ],
        "x": 2785,
        "y": 2260,
        "wires": [
            [
                "4b5fa5220e6f11ce",
                "ef660b7f94cfaef5"
            ]
        ]
    },
    {
        "id": "a706a207c63e7192",
        "type": "link out",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "links": [
            "31b130f940682e38"
        ],
        "x": 2795,
        "y": 2820,
        "wires": []
    },
    {
        "id": "5269196694985f72",
        "type": "comment",
        "z": "2df22b78ebc5ab05",
        "name": "Influxdb test area",
        "info": "",
        "x": 2640,
        "y": 2760,
        "wires": []
    },
    {
        "id": "ef660b7f94cfaef5",
        "type": "debug",
        "z": "2df22b78ebc5ab05",
        "name": "",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "true",
        "targetType": "full",
        "statusVal": "",
        "statusType": "auto",
        "x": 2930,
        "y": 2220,
        "wires": []
    },
    {
        "id": "6b5cc424.028694",
        "type": "websocket-listener",
        "path": "/ws/Kiln",
        "wholemsg": "false"
    },
    {
        "id": "730a18e6.6782b",
        "type": "modbus-client",
        "name": "",
        "clienttype": "tcp",
        "bufferCommands": true,
        "stateLogEnabled": false,
        "queueLogEnabled": false,
        "tcpHost": "10.0.3.164",
        "tcpPort": "502",
        "tcpType": "DEFAULT",
        "serialPort": "/dev/ttyUSB",
        "serialType": "RTU-BUFFERD",
        "serialBaudrate": "9600",
        "serialDatabits": "8",
        "serialStopbits": "1",
        "serialParity": "none",
        "serialConnectionDelay": "100",
        "serialAsciiResponseStartDelimiter": "",
        "unit_id": "1",
        "commandDelay": "1",
        "clientTimeout": "1000",
        "reconnectOnTimeout": true,
        "reconnectTimeout": "2000",
        "parallelUnitIdsAllowed": true
    },
    {
        "id": "1a7d214ba0ada216",
        "type": "influxdb",
        "hostname": "127.0.0.1",
        "port": "8086",
        "protocol": "http",
        "database": "home",
        "name": "",
        "usetls": false,
        "tls": "",
        "influxdbVersion": "1.x",
        "url": "http://localhost:8086",
        "rejectUnauthorized": true
    }
]
```