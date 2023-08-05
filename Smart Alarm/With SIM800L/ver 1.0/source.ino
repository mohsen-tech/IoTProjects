#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Ticker.h>

// Configuration
#define BAUD_RATE 9600
byte currentState = 0x00; // 0=>NotPair || 1=>Pair
byte checkSensors = 0x00; // 0=>Without Check Sensors || 1=>With Check Sensors
byte flagNewPass = 0x00;  // 0=>With Default Pass || 1=>With New Pass
String phoneNumber, password;

// Implementation of work with Timer
Ticker timer, makeCallTimer;
#define TIMER_S 30 // 600
#define CALL_TIMER_S 40
void resetTimer();
void setTimer(bool &_flag);
void timerHandler();
void makeCallTimerHandler();

// Network side, Server side
const char *ESP_WIFI_SSID = "SecurityModule";
const char *ESP_WIFI_DEFAULT_PASSWORD = "12345678";
#define ADMIN_USER "admin"
#define ADMIN_PASSWORD "admin"
String validationCode;
ESP8266WebServer server(80); // Create a webserver object that listens for HTTP request on port 80
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Login Form</title>
</head>
<body>
    <h1>Login</h1>
    <form id="loginForm" action="/authenticate" method="POST">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Login">
    </form>
    <script>
        document.getElementById('loginForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var username = document.getElementById('username').value;
            var password = document.getElementById('password').value;
            var formData = new FormData();
            formData.append('username', username);
            formData.append('password', password);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/authenticate', true);
            xhr.onreadystatechange = function () {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        window.location.href = '/setPhoneNumber';
                    }
                    else {
                        window.location.href = '/error';
                    }
                }
            };
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";
const char setPhoneNumber_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Phone Number Form</title>
</head>
<body>
    <h1>Phone Number</h1>
    <form id="phoneForm" action="/authenticatePhoneNumber" method="POST">
        <label for="phoneNumber">Phone Number:</label>
        <input type="tel" id="phoneNumber" name="phoneNumber" pattern="[0-9]+" required placeholder="+98 . . ."><br><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" required><br><br>
        <input type="submit" value="Submit">
    </form>
    <script>
        document.getElementById('phoneForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var phoneNumber = document.getElementById('phoneNumber').value;
            var password = document.getElementById('password').value;
            window.location.href = '/authenticatePhoneNumber?phoneNumber=' + encodeURIComponent(phoneNumber) + '&password=' + encodeURIComponent(password);
        });
    </script>
</body>
</html>
)rawliteral";
const char codeForm_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Code Form</title>
</head>
<body>
    <h1>Code</h1>
    <form id="codeForm" action="/process-code" method="POST">
        <label for="code">Code:</label>
        <input type="text" id="code" name="code" pattern="[0-9]{5}" maxlength="5" required><br><br>
        <input type="submit" value="Submit">
    </form>
    <script>
        document.getElementById('codeForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var code = document.getElementById('code').value;
            var formData = new FormData();
            formData.append('code', code);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/process-code', true);
            xhr.onreadystatechange = function () {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        window.location.href = '/dashboard';
                    } else {
                        window.location.href = '/error';
                    }
                }
            };
            xhr.send(formData);
        });
    </script>
</body>
</html>
)rawliteral";
void initAP();
void initWebServer();
void handleRoot();
void handleAuthenticate();
void handleSetPhoneNumber();
String generateCode(String &_str, const byte &_codeSize);
void handleAuthenticatePhoneNumber();
void handleProcessCode();
void handleDashboard();
void handleUpdateSensor();
void handleError();
void handleNotFound();
String phoneNumber_temp, password_temp;

// Implementations that depend on pins
#define LONG_PRESS_TIME 2700
#define PIR_PIN D4         // Active HIGH
#define DOOR_SENSOR_PIN D7 // Active HIGH
#define PUSH_BTN_PIN D5    // Active HIGH
struct Key
{
  byte pin;
  unsigned long buttonTimer = 0;
  bool buttonActive = false, longPressActive = false;
} progKey, doorSensor;
void readButtonState(Key &_key, const byte &_relayPin);
void checkButtons(Key &_key, const byte &_relayPin);

// Implementation of work with EEPROM
#define SIZE_EEPROM 50
#define MAGIC_IDX 0
#define MAGIC_NUM 13
#define CSidx_EEPROM 1 // index of CurrentState in EEPROM
#define IsConfig 0x01
#define IsNotConfig 0x00
#define ChS_EEPROM 2 // index of CheckSensors in EEPROM
#define flagNewPass_EEPROM 3
#define separator '#'
String getStrWithSeparatorFromEEPROM(byte &_idx);
void initEEPROM();
void ResetFactoryFunc(); // Reset and set default
String createStrForEEPROM(const String &_phNum, const String &_pass);
void saveToEEPROM(const String &_str);
void updateSensors(const byte &_checkSensors);

// Implementations that depend on SIM800
SoftwareSerial sim800(D1, D2);
String mySMS = "";
void initSIM800();
void sendSMS(const String &_phoneNumber, const String &_text);
String receiveFromSim();
void makeCall(const String &_phoneNumber);
void hangUpCall();
void updateSerial();
void runCommand(const String &command);
bool checkSenderValid(const String &command);

// Other functions

//--------------------------------------------------------------------------------------------------------------------------------
#define minimumDangerThreshold 500
int thresholdCounter = 0;
bool flagcheckPIR = true;
void checkPIR()
{
  if (digitalRead(PIR_PIN) == HIGH)
  {
    delay(20);
    if (digitalRead(PIR_PIN) == HIGH)
    {
      thresholdCounter++;
      if (thresholdCounter >= minimumDangerThreshold)
      {
        ///
        Serial.println("khatar!");
        sendSMS(phoneNumber, "khatar!");
        setTimer(flagcheckPIR);
        thresholdCounter = 0;
      }
    }
  }
}
//--------------------------------------------------------------------------------------------------------------------------------

void setup()
{
  pinMode(PIR_PIN, INPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT);
  pinMode(PUSH_BTN_PIN, INPUT);
  Serial.begin(BAUD_RATE);
  Serial.println();
  doorSensor.pin = DOOR_SENSOR_PIN;
  progKey.pin = PUSH_BTN_PIN;
  initEEPROM();
  initAP();
  initWebServer();
  initSIM800();
}
void loop()
{
  checkButtons(progKey, PUSH_BTN_PIN);
  if (checkSensors == 1)
  {
    checkButtons(doorSensor, DOOR_SENSOR_PIN);
    if (flagcheckPIR == true)
      checkPIR();
  }
  if (currentState == IsConfig && sim800.available())
  {
    mySMS = receiveFromSim();
    if (mySMS != "" && checkSenderValid(mySMS) == true)
    {
      runCommand(mySMS);
    }
  }
  server.handleClient();
}
//--------------------------------------------------------------------------------------------------------------------------------
void resetTimer()
{
  timer.detach();
}
void setTimer(bool &_flag)
{
  _flag = false;
  if (timer.active() == true)
    resetTimer();

  timer.attach(TIMER_S, timerHandler);
}
void timerHandler()
{
  resetTimer();
  flagcheckPIR = true;
}
void makeCallTimerHandler()
{
  hangUpCall();
  makeCallTimer.detach();
}
//--------------------------------------------------------------------------------------------------------------------------------
void initAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ESP_WIFI_SSID, ESP_WIFI_DEFAULT_PASSWORD);
  Serial.println(WiFi.softAPIP());
}
void initWebServer()
{
  server.on("/", handleRoot);
  server.on("/authenticate", handleAuthenticate);
  server.on("/setPhoneNumber", handleSetPhoneNumber);
  server.on("/authenticatePhoneNumber", handleAuthenticatePhoneNumber);
  server.on("/process-code", handleProcessCode);
  server.on("/dashboard", handleDashboard);
  server.on("/updateSensor", handleUpdateSensor);
  server.on("/error", handleError);
  server.onNotFound(handleNotFound);
  server.begin();
}
void handleRoot()
{
  server.send(200, "text/html", login_html);
}
void handleAuthenticate()
{
  String username_temp = server.arg("username"), pass_temp = server.arg("password");
  if (username_temp == phoneNumber && pass_temp == password)
  {
    server.send(200);
  }
  else
    server.send(401);
}
void handleSetPhoneNumber()
{
  server.send(200, "text/html", setPhoneNumber_html);
}
String generateCode(String &_str, const byte &_codeSize = 5)
{
  _str = "";
  for (byte i = 0; i < _codeSize; i++)
    _str += random(9);
  ///
  Serial.print("code: ");
  Serial.println(_str);
  return _str;
}
void handleAuthenticatePhoneNumber()
{
  phoneNumber_temp = server.arg("phoneNumber"), password_temp = server.arg("password");
  generateCode(validationCode);

  Serial.print("phoneNumber_temp: ");
  Serial.println(phoneNumber_temp);
  Serial.print("password_temp: ");
  Serial.println(password_temp);

  sendSMS(phoneNumber_temp, "code: " + validationCode);
  server.send(200, "text/html", codeForm_html);
}
void handleProcessCode()
{
  String code = server.arg("code");
  if (code == validationCode)
  {
    phoneNumber = phoneNumber_temp;
    password = password_temp;
    saveToEEPROM(createStrForEEPROM(phoneNumber, password));
    currentState = IsConfig;
    flagNewPass = 1;
    checkSensors = 0;
    EEPROM.write(CSidx_EEPROM, currentState);
    delay(1);
    EEPROM.write(ChS_EEPROM, checkSensors);
    delay(1);
    EEPROM.write(flagNewPass_EEPROM, flagNewPass);
    delay(1);
    EEPROM.commit();
    delay(1);

    ///
    Serial.println("if (code == validationCode)");
    Serial.print("phoneNumber: ");
    Serial.println(phoneNumber);
    Serial.print("password: ");
    Serial.println(password);
    Serial.print("currentState: ");
    Serial.println(currentState);
    Serial.print("checkSensors: ");
    Serial.println(checkSensors);
    Serial.print("flagNewPass: ");
    Serial.println(flagNewPass);
    ///
    server.send(200);
  }
  else
  {
    ///
    Serial.println("if (code != validationCode)");
    server.send(401);
  }
}
void handleDashboard()
{
  if (!server.authenticate(phoneNumber.c_str(), password.c_str()))
  {
    server.requestAuthentication();
    return;
  }
  String html = "<!DOCTYPE html><html><head><title>Dashboard</title><script>function changeValue() {";
  html += "var currentValue = document.getElementById('sensorValue').innerText;";
  html += "var newValue = currentValue === 'ON' ? 'OFF' : 'ON';";
  html += "if (postData(newValue) == true) { document.getElementById('sensorValue').innerText = newValue;}}";
  html += "function postData(value) {";
  html += "var xhr = new XMLHttpRequest();";
  html += "xhr.open('POST', '/updateSensor', true);";
  html += "xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
  html += "xhr.onload = function () {if (xhr.status === 200) {location.reload();return true;}return false;};";
  html += "xhr.send('sensorValue=' + encodeURIComponent(value));}</script></head><body>";
  String temp = "OFF";
  if (checkSensors == 1)
    temp = "ON";
  html += "<h1>Sensor Activation Status: <span id=\"sensorValue\">" + temp + "</span></h1>";
  html += "<button onclick=\"changeValue()\">Toggle Sensor</button></body></html>";
  server.send(200, "text/html", html);
}
void handleUpdateSensor()
{
  String temp = server.arg("sensorValue");
  if (temp == "ON")
  {
    updateSensors(1);
    server.send(200);
  }
  else if (temp == "OFF")
  {
    updateSensors(0);
    server.send(200);
  }
  server.send(401);
}
void handleError()
{
  server.send(401, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
}
void handleNotFound()
{
  server.send(404, "text/plain", "Error 404 - Not Found!");
}
//--------------------------------------------------------------------------------------------------------------------------------
void readButtonState(Key &_key, const byte &_relayPin)
{
  if (digitalRead(_key.pin) == HIGH)
  {
    if (_key.buttonActive == false)
    {
      _key.buttonActive = true;
      _key.buttonTimer = millis();
    }
    if ((millis() - _key.buttonTimer >= LONG_PRESS_TIME) && (_key.longPressActive == false))
    {
      _key.longPressActive = true;
      // Serial.println("Hold");
      if (_key.pin == PUSH_BTN_PIN)
      {
        ResetFactoryFunc();
      }
      else if (_key.pin == DOOR_SENSOR_PIN)
      {
        ///
        Serial.println("DOOR_SENSOR_PIN");
        Serial.print("makeCall ");
        Serial.println(phoneNumber);
        makeCall(phoneNumber);
        makeCallTimer.attach(CALL_TIMER_S, makeCallTimerHandler);
      }
    }
  }
  else
  {
    if (_key.buttonActive == true)
    {
      if (_key.longPressActive == true)
      {
        _key.longPressActive = false;
      }
      else
      {
        if (_key.pin == PUSH_BTN_PIN)
        {
          checkSensors = !checkSensors;
          updateSensors(checkSensors);
        }
      }
      _key.buttonActive = false;
    }
  }
}
void checkButtons(Key &_key, const byte &_relayPin)
{
  if (digitalRead(_key.pin) == HIGH)
  {
    delay(20);
    readButtonState(_key, _relayPin);
  }
  else if (_key.buttonActive == true)
    readButtonState(_key, _relayPin);
}
//--------------------------------------------------------------------------------------------------------------------------------
String getStrWithSeparatorFromEEPROM(byte &_idx)
{
  String str = "";
  while (1)
  {
    char ch = EEPROM.read(_idx++);
    delay(1);
    if (ch == separator)
      return str;
    str += ch;
  }
}
void initEEPROM()
{
  EEPROM.begin(SIZE_EEPROM);
  delay(1);

  if (EEPROM.read(MAGIC_IDX) != MAGIC_NUM)
  {
    EEPROM.write(MAGIC_IDX, MAGIC_NUM);
    delay(1);
    currentState = IsNotConfig;
    EEPROM.write(CSidx_EEPROM, currentState);
    delay(1);
    EEPROM.commit();
    delay(1);
  }
  else
  {
    EEPROM.get(CSidx_EEPROM, currentState);
    delay(1);
    if (currentState == IsConfig)
    {
      EEPROM.get(ChS_EEPROM, checkSensors);
      delay(1);
      EEPROM.get(flagNewPass_EEPROM, flagNewPass);
      delay(1);
      byte i = flagNewPass_EEPROM + 1;
      phoneNumber = getStrWithSeparatorFromEEPROM(i);
      if (flagNewPass == 1)
      {
        password = getStrWithSeparatorFromEEPROM(i);
      }
      else
        password = ADMIN_PASSWORD;
    }
    else
    {
      phoneNumber = ADMIN_USER;
      password = ADMIN_PASSWORD;
      checkSensors = 0;
    }

    ///
    Serial.println("else");
    Serial.print("currentState: ");
    Serial.println(currentState);
    Serial.print("checkSensors: ");
    Serial.println(checkSensors);
    Serial.print("flagNewPass: ");
    Serial.println(flagNewPass);
    Serial.print("phoneNumber: ");
    Serial.println(phoneNumber);
    Serial.print("password: ");
    Serial.println(password);
    ///
  }
}
void ResetFactoryFunc() // Reset and set default
{
  EEPROM.write(CSidx_EEPROM, IsNotConfig);
  delay(1);
  EEPROM.write(ChS_EEPROM, 0);
  delay(1);
  EEPROM.write(flagNewPass_EEPROM, 0);
  delay(1);
  EEPROM.commit();
  delay(1);
  ESP.restart();
}
String createStrForEEPROM(const String &_phNum, const String &_pass)
{
  return _phNum + separator + _pass + separator;
}
void saveToEEPROM(const String &_str)
{
  byte j = flagNewPass_EEPROM + 1;
  for (byte i = 0; i < _str.length(); i++)
  {
    EEPROM.write(j++, _str[i]);
    delay(1);
  }
  EEPROM.commit();
  delay(1);
}
void updateSensors(const byte &_checkSensors)
{
  checkSensors = _checkSensors;
  EEPROM.write(ChS_EEPROM, checkSensors);
  delay(1);
  EEPROM.commit();
  delay(1);
  ///
  Serial.print("checkSensors: ");
  Serial.println(checkSensors);
}
//--------------------------------------------------------------------------------------------------------------------------------
void initSIM800()
{
  sim800.begin(BAUD_RATE);
  delay(1000);
  sim800.setTimeout(10000);
  delay(1000);
  sim800.println("AT"); // Once the handshake test is successful, it will back to OK
  updateSerial();
  sim800.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  sim800.println("AT+CNMI=2,2,0,0,0"); // Forward received SMS to serial port using interrupt
  updateSerial();
  sim800.print("AT+CMGDA=\"DEL ALL\"\r\n"); // delete sms
  updateSerial();
}
void sendSMS(const String &_phoneNumber, const String &_text)
{
  sim800.print("AT+CMGS=\""); // command to send sms
  sim800.print(_phoneNumber);
  sim800.print("\"\r");
  updateSerial();
  sim800.print(_text); // text content
  updateSerial();
  sim800.write(26);
}
String receiveFromSim()
{
  String temp = "";
  while (sim800.available())
    temp += sim800.readString();
  return temp;
}
void makeCall(const String &_phoneNumber)
{
  sim800.print("ATD");
  sim800.print(_phoneNumber);
  sim800.println(";");
  delay(1000);
}
void hangUpCall()
{
  sim800.println("ATH");
  delay(1000);
}
void updateSerial()
{
  delay(500);
  while (Serial.available())
  {
    sim800.write(Serial.read()); // Forward what Serial received to Software Serial Port
  }
  while (sim800.available())
  {
    Serial.write(sim800.read()); // Forward what Software Serial received to Serial Port
  }
}
void runCommand(const String &command)
{
  if (command.indexOf("ON") > 0)
  {
    updateSensors(1);
    Serial.println("\n*** checkSensors = 1 ***\n");
    sendSMS(phoneNumber, "Your Command is Run");
    ///
  }
  else if (command.indexOf("OFF") > 0)
  {
    updateSensors(0);
    Serial.println("\n*** checkSensors = 0 ***\n");
    sendSMS(phoneNumber, "Your Command is Run");
    ///
  }
  else if (command.indexOf("status") > 0)
  {
    if (checkSensors == 1)
      sendSMS(phoneNumber, "check Sensors is ON");
    else
      sendSMS(phoneNumber, "check Sensors is OFF");
  }
  else
  {
    sendSMS(phoneNumber, "Your Command is Wrong");
  }
}
bool checkSenderValid(const String &command)
{
  if (command.indexOf(phoneNumber) > 0)
    return true;
  return false;
}
//--------------------------------------------------------------------------------------------------------------------------------