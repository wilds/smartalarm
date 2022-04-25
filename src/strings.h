#ifndef _STRINGS_H_
#define _STRINGS_H_

const char ALARM_PAGE_STYLE[]       PROGMEM = "<style> #keypad {display: flex;justify-content: center;flex-wrap: wrap;margin: auto;width: 100%;max-width: 300px;}.numberkey {flex: 1 1 auto;overflow: hidden;padding: 8px;width: 30%;margin: 4px;transition-delay: 0.2s;}.actions {margin: 0px;display: flex;flex-wrap: wrap;justify-content: center;}.action {flex: 1 1 auto;overflow: hidden;padding: 8px;width: 20%;margin: 4px;transition-delay: 0.2s;}.state {float: right;background-color: #df4c1e;border-radius: 20px;padding: 5px;font-size: medium;}</style>";
const char ALARM_PAGE_SCRIPT[]      PROGMEM = "<script> function sendCommand(c) {if (c) {const xhttp = new XMLHttpRequest();xhttp.open('POST', 'command', false);xhttp.setRequestHeader('Content-type', 'text/plain');xhttp.send(c);document.getElementById('resp').innerHTML = xhttp.responseText;}}function add(k) {document.getElementById('alarmCode').value = document.getElementById('alarmCode').value + k;}function remove() {document.getElementById('alarmCode').value = document.getElementById('alarmCode').value.slice(0, -1);}</script>";
const char ALARM_PAGE_BODY[]        PROGMEM = "<h3>"
"Alarm Control"
"<span id='state' class='state'>"
"{c}"
"</span>"
"</h3>"
""
"<div id='armActions' class='actions'>"
"<button class='action' onclick='sendCommand(\"A\")'>Arm Away</button><br/>"
"<button class='action' onclick='sendCommand(\"H\")'>Arm Home</button><br/>"
"<button class='action' onclick='sendCommand(\"C\")'>Call</button><br/>"
"<button class='action' onclick='sendCommand(\"S\")'>SOS</button><br/>"
"</div>"
"<div id='resp'></div>"
"<div id='keypad'>"
"<input id='alarmCode' name='alarmCode' value=''>"
"<button class='numberkey' onclick='add(\"1\")'>1</button>"
"<button class='numberkey' onclick='add(\"2\")'>2</button>"
"<button class='numberkey' onclick='add(\"3\")'>3</button>"
"<button class='numberkey' onclick='add(\"4\")'>4</button>"
"<button class='numberkey' onclick='add(\"5\")'>5</button>"
"<button class='numberkey' onclick='add(\"6\")'>6</button>"
"<button class='numberkey' onclick='add(\"7\")'>7</button>"
"<button class='numberkey' onclick='add(\"8\")'>8</button>"
"<button class='numberkey' onclick='add(\"9\")'>9</button>"
"<button class='numberkey' onclick='add(\"*\")'>*</button>"
"<button class='numberkey' onclick='add(\"0\")'>0</button>"
"<button class='numberkey' onclick='add(\"#\")'>#</button>"
"<button class='numberkey' onclick='sendCommand(document.getElementById(\"alarmCode\").value);document.getElementById(\"alarmCode\").value=\"\"'>Send</button>"
"<button class='numberkey' onclick='remove()'>Canc</button>"
"</div>";

#endif
