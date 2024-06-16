// remote-asiatech.runflare.com:32373
// myiotproject
// sUL9z8rFnzdoxdIQ5ZHi

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <base64custom.h>
#include <Hash.h>
#include <Ticker.h>

// Configuration
#define BAUD_RATE 9600
char *uniqueID = "D1";
char *device2UniqueID = "D2";
byte currentState = 0x00; // 0x00=>NotPair || 0x01=>Pair

// Network side
#define defaultSSID "ESP-WiFi"
#define defaultPASSWORD "12345678"
String userName, userPassword, userWiFiSSID, userWiFiPassword;
ESP8266WebServer server(80);
const char adminForm_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Admin Form</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
        }
        h1 {
            text-align: center;
        }
        form {
            max-width: 500px;
            margin: 0 auto;
        }
        label {
            display: block;
            margin-top: 10px;
        }
        input {
            width: 100%;
            padding: 8px;
            box-sizing: border-box;
            margin-top: 5px;
        }
        input[type="submit"] {
            background-color: #4CAF50;
            color: white;
            cursor: pointer;
        }
        input[type="submit"]:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <h1>Enter the details</h1>
    <form id="authForm" action="/auth" method="POST">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" required>
        <label for="userPassword">Password:</label>
        <input type="password" id="userPassword" name="userPassword" required>
        <label for="ssid">WiFi SSID:</label>
        <input type="text" id="ssid" name="ssid" required>
        <label for="ssidPassword">WiFi Password:</label>
        <input type="password" id="ssidPassword" name="ssidPassword" required>
        <input type="submit" value="Submit">
    </form>
    <script>
        document.getElementById('authForm').addEventListener('submit', function (event) {
            event.preventDefault();
            var formData = {};
            var formElements = this.elements;
            for (var i = 0; i < formElements.length; i++) {
                if (formElements[i].type !== "submit") {
                    formData[formElements[i].name] = encodeURIComponent(formElements[i].value);
                }
            }
            var queryString = Object.keys(formData).map(key => key + '=' + formData[key]).join('&');
            window.location.href = '/auth?' + queryString;
        });
    </script>
</body>
</html>
)rawliteral";
const char dashboard1_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100vh;
            background-color: #f4f4f4;
        }
        #formContainer {
            text-align: center;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            background-color: #fff;
        }
        h1 {
            color: #333;
        }
        p {
            color: #555;
        }
        input {
            padding: 10px;
            margin-bottom: 15px;
            width: 100%;
            box-sizing: border-box;
        }
        input::placeholder {
            color: #999;
        }
        button {
            padding: 12px;
            cursor: pointer;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
            transition: background-color 0.3s ease;
        }
        button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div id="formContainer">
        <h1>Dashboard</h1>
)rawliteral";
void initWiFi();
void initWebServer();
void handleRoot();
void handleAuth();
void handleDashboard();
void handleSend();
void handleReset();
void handleError();
void handleNotFound();

// MQTT Broker side
#define MQTT_TIMEOUT 5000
uint32_t lastReconnectAttempt = 0;
const char *mqttServer = "remote-asiatech.runflare.com";
const int mqttPort = 32373;
const char *MQTT_Username = "myiotproject";
const char *MQTT_Password = "sUL9z8rFnzdoxdIQ5ZHi";
const char *registerTopic = "Register";
// bool flagConnected = false;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
void initMQTT();
bool mqttConnect(const String &);
void mqttPublish(const String &, const String &);
void subscribeReceive(char *topic, byte *payload, unsigned int length);

// Implementation of work with EEPROM
#define SIZE_EEPROM 400
#define MAGIC_IDX 0
#define MAGIC_NUM 13
#define CSidx_EEPROM 1 // index of CurrentState in EEPROM
#define DATA_IDX 2
#define IsConfig 0x01
#define IsNotConfig 0x00
#define separator char(3)
String getStrWithSeparatorFromEEPROM(byte &_idx);
void initEEPROM();
void resetFactoryFunc(); // Reset and set default
String createStrForEEPROM(const String &, const String &, const String &, const String &);
void saveToEEPROM(const byte &_idx, const String &_str);

// other Implementation
String data = "", token = "";
uint32_t decode_time = 0;
String getSHAToken(const String &, const String &);
void generateToken();
String encryption(const String &);
String decoding(const String &, const String &);
Ticker timer;
void timerHandler()
{
  WiFi.begin(userWiFiSSID, userWiFiPassword);
}

//---------------------------------------------------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.println();
  initEEPROM();
  initWiFi();
  initWebServer();
  if (currentState == IsConfig)
  {
    initMQTT();
    MDNS.begin(uniqueID);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
}
void loop()
{
  server.handleClient();
  if (currentState == IsConfig)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      ///
      if (timer.active())
      {
        Serial.println("if (timer.active())");
        timer.detach();
      }
      if (!mqttClient.connected())
      {
        // uint32_t t = millis();
        // if (t - lastReconnectAttempt > MQTT_TIMEOUT)
        // {
        //   lastReconnectAttempt = t;
        //   if (mqttConnect(clientID))
        //   {
        //     lastReconnectAttempt = 0;
        //   }
        // }
        if (lastReconnectAttempt >= MQTT_TIMEOUT)
        {
          mqttConnect(uniqueID);
          lastReconnectAttempt = 0;
        }
        else
        {
          lastReconnectAttempt += 5;
        }
      }
      mqttClient.loop();
    }
    else
    {
      timer.attach(10, timerHandler);
    }
  }
  delay(5);
}
//---------------------------------------------------------------------------------------------------------------------------------------
void initWiFi()
{
  if (currentState == IsNotConfig)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(userWiFiSSID, userWiFiPassword);
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(userWiFiSSID, userWiFiPassword);
    if (WiFi.waitForConnectResult() == WL_CONNECTED)
      Serial.println(WiFi.localIP());
    else
      Serial.println("Cannot connect to WiFi!");
  }
}
void initWebServer()
{
  server.on("/", handleRoot);
  server.on("/auth", handleAuth);
  server.on("/dashboard", handleDashboard);
  server.on("/send", handleSend);
  server.on("/reset", handleReset);
  server.on("/error", handleError);
  server.onNotFound(handleNotFound);
  server.begin();
}
void handleRoot()
{
  if (currentState == IsNotConfig)
  {
    server.send(200, "text/html", adminForm_html);
  }
  else if (currentState == IsConfig)
  {
    String str = "<br><a href=\"/dashboard\">Click to See the Dashboard</a>";
    str += "<br><br><a href=\"/reset\">Click to Reset Admin</a>";
    server.send(200, "text/html", str);
  }
}
void handleAuth()
{
  String userName_temp = server.arg("username"), userPassword_temp = server.arg("userPassword"),
         wifiSSID_temp = server.arg("ssid"), wifiPassword_temp = server.arg("ssidPassword");

  if (userName_temp != "" && userPassword_temp != "" && wifiSSID_temp != "" && wifiPassword_temp != "")
  {
    userName = userName_temp;
    userPassword = userPassword_temp;
    userWiFiSSID = wifiSSID_temp;
    userWiFiPassword = wifiPassword_temp;

    saveToEEPROM(DATA_IDX, createStrForEEPROM(userName, userPassword, userWiFiSSID, userWiFiPassword));
    delay(1);
    currentState = IsConfig;
    EEPROM.write(CSidx_EEPROM, currentState);
    delay(1);
    EEPROM.commit();
    delay(1);
    server.send(200, "text/html", "<a>The entered information has been saved!</a><br><br><br><a>After 10 seconds, the device will restart</a>");
    delay(10000);
    ESP.restart();
  }
  else
  {
    server.send(401, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
  }
}
void handleDashboard()
{
  if (currentState == IsNotConfig)
  {
    server.send(404, "text/plain", "Error 404 - Not Found!");
    return;
  }
  if (!server.authenticate(userName.c_str(), userPassword.c_str()))
  {
    server.requestAuthentication();
    return;
  }
  String html = dashboard1_html;
  html += "<p>Last data received: " + data + "</p>";
  html += "<p>Decoding time: " + String(decode_time) + " ms</p>";
  html += "<form id=\"sendForm\" action=\"/send\" method=\"GET\">";
  html += "<input type=\"text\" id=\"textInput\" name=\"data\" placeholder=\"Please enter your text\" required>";
  html += "<br>";
  html += "<button type=\"submit\">Send</button>";
  html += "</form>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  server.send(200, "text/html", html);
}
void handleSend()
{
  String temp = server.arg("data");
  mqttPublish(device2UniqueID, temp);
  delay(500);
  server.sendHeader("Location", "/dashboard", true);
  server.send(302, "text/plane", "");
}
void handleReset()
{
  if (currentState == IsNotConfig)
  {
    server.send(404, "text/plain", "Error 404 - Not Found!");
    return;
  }
  if (!server.authenticate(userName.c_str(), userPassword.c_str()))
  {
    server.requestAuthentication();
    return;
  }
  server.send(200, "text/html", "After 10 seconds, the settings return to the initial state");
  delay(10000);
  resetFactoryFunc();
}
void handleError()
{
  server.send(401, "text/html", "The entered values are not correct!<br><a href=\"/\">Return to Home Page</a>");
}
void handleNotFound()
{
  server.send(404, "text/plain", "Error 404 - Not Found!");
}
//---------------------------------------------------------------------------------------------------------------------------------------
void initMQTT()
{
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(subscribeReceive);
  mqttConnect(uniqueID);
}
bool mqttConnect(const String &_clientId)
{
  if (mqttClient.connect(_clientId.c_str(), MQTT_Username, MQTT_Password))
  {
    mqttClient.subscribe(uniqueID);
    mqttClient.publish(registerTopic, uniqueID);
    return true;
  }
  else
  {
    Serial.println("not connected!\n");
    return false;
  }
}
void mqttPublish(const String &_topic, const String &_data)
{
  if (mqttClient.connected())
  {
    Serial.print("publish: ");
    Serial.println(_data);
    String json;
    JsonDocument doc;
    doc["data"] = encryption(_data);
    doc["token"] = token;
    serializeJson(doc, json);
    mqttClient.publish(_topic.c_str(), json.c_str());
  }
  else
  {
    Serial.println("can't publish");
  }
}
void subscribeReceive(char *topic, byte *payload, unsigned int length)
{
  String temp;
  for (int i = 0; i < length; i++)
    temp += (char)payload[i];

  JsonDocument doc;
  deserializeJson(doc, temp);
  const char *temp_data = doc["data"];
  const char *token_temp = doc["token"];
  data = decoding(temp_data, token_temp);

  Serial.println("-----------------------------------------------------{");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("payload: ");
  Serial.println(data);
  Serial.println("-----------------------------------------------------}");
}
//---------------------------------------------------------------------------------------------------------------------------------------
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
      byte i = DATA_IDX + 1;
      userName = getStrWithSeparatorFromEEPROM(i);
      userPassword = getStrWithSeparatorFromEEPROM(i);
      userWiFiSSID = getStrWithSeparatorFromEEPROM(i);
      userWiFiPassword = getStrWithSeparatorFromEEPROM(i);

      Serial.println(userName);
      Serial.println(userPassword);
      Serial.println(userWiFiSSID);
      Serial.println(userWiFiPassword);
    }
    else
    {
      userWiFiSSID = defaultSSID;
      userWiFiPassword = defaultPASSWORD;
    }
  }
}
void resetFactoryFunc() // Reset and set default
{
  EEPROM.write(CSidx_EEPROM, IsNotConfig);
  delay(1);
  EEPROM.commit();
  delay(1);
  ESP.restart();
}
String createStrForEEPROM(const String &a, const String &b, const String &c, const String &d)
{
  return a + separator + b + separator + c + separator + d + separator;
}
void saveToEEPROM(const byte &_idx, const String &_str)
{
  byte j = _idx + 1;
  for (byte i = 0; i < _str.length(); i++)
  {
    EEPROM.write(j++, _str[i]);
    delay(1);
  }
  EEPROM.commit();
  delay(1);
}
//---------------------------------------------------------------------------------------------------------------------------------------
String getSHAToken(const String &_token, const String &_id) // get SHA of current token to compare with the revieved token from the user
{
  return sha1(_token + _id + _token);
}
void generateToken() // Generates a new token with 3 random integers.
{
  int *res = new int[3];
  res[0] = rand() % 10;
  res[1] = rand() % 10;
  res[2] = rand() % 10;
  token = String(res[0]) + String(res[1]) + String(res[2]);
  delete[] res;
}
String encryption(const String &_data)
{
  generateToken();
  String temp = _data + getSHAToken(token, device2UniqueID);
  char b64data[256];
  b64_encode(b64data, (char *)temp.c_str(), temp.length());
  return b64data;
}
String decoding(const String &_entrance, const String &_token)
{
  uint32_t t = millis();
  String ss = _entrance;
  char decodedString[300];
  b64_decode(decodedString, (char *)ss.c_str(), ss.length());
  int decodedLength = 0;
  for (int i = 0; i < 300; i++)
  {
    if (decodedString[i] == '\0')
    {
      break;
    }
    decodedLength++;
  }
  String tokenHash = "";
  for (int i = decodedLength - 40; i < decodedLength; i++) // decodedLength - 40 => 40 is size of any string encoded in sha1
  {
    tokenHash += decodedString[i];
  }
  String currentTokenHash = getSHAToken(_token, uniqueID); // get SHA of current token generated before
  currentTokenHash.toLowerCase();
  tokenHash.toLowerCase();
  if (currentTokenHash != tokenHash)
  {
    decode_time = millis() - t;
    return "not valied";
  }
  String res = "";
  for (int i = 0; i < decodedLength - 40; i++)
  {
    res += decodedString[i];
  }
  decode_time = millis() - t;
  return res;
}