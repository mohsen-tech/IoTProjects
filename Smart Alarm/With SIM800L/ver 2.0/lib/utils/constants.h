#include <Arduino.h>

const static char htmlmsg[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Smart Door Input Form</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
</head>

<body>
    <form action="/get">
        Your Phone Number: <input type="text" name="phoneNumber">
        <br />
        <br />
        <br /> Your SSID: <input type="text" name="ssid">
        <br />
        <br />
        <br />
        Your PASS: <input type="text" name="pass">
        <br />
        <br />
        <br /> <input type="submit" value="Submit">
    </form>
    <br />
</body>

</html>)rawliteral";

const static char home_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>Status</title>
    <script>

        function togglePirValue() {
            var currentURL = window.location.href;
            var url = new URL(currentURL);

            var statusElement = document.getElementById("status");
            var statusValue = statusElement.innerText;

            // Toggle the value of "pir" parameter
            if (statusValue === "Active") {
                url.searchParams.set("pir", "false");
            } else if (statusValue === "Inactive") {
                url.searchParams.set("pir", "true");
            }

            // Refresh the page with the updated URL
            window.location.href = url.toString();
        }
    </script>
</head>
)rawliteral";

const static char valid_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Smart Door Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/valid">
    A code has been sent to you to validate your phone number, please enter it: <input type="text" name="code">
    <input type="submit" value="Submit">
  </form><br>
</body></html>)rawliteral";