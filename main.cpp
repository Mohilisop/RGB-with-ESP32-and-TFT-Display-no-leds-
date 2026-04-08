#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// ================= WIFI =================
const char* ssid = "A1 602";
const char* password = "Mohilisop@1979";

WebServer server(80);

// ================= TFT PINS =================
#define TFT_CS    15
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_MISO  19
#define TFT_BL    5

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ================= CONFIG =================
#define TRANSITION_SPEED 3
#define COLOR_DWELL_TIME 3000
#define DISPLAY_UPDATE_RATE 30

// ================= COLOR =================
struct Color {
  uint8_t r, g, b;
  const char* name;
};

Color colors[] = {
  {255, 20, 20, "Red"},
  {255, 140, 0, "Orange"},
  {255, 255, 50, "Yellow"},
  {50, 255, 50, "Green"},
  {50, 255, 255, "Cyan"},
  {50, 50, 255, "Blue"},
  {255, 50, 255, "Magenta"},
  {255, 220, 230, "Pink"}
};

#define NUM_COLORS (sizeof(colors) / sizeof(colors[0]))

Color currentColor = colors[0];
Color targetColor = colors[1];

int currentIndex = 0;
bool autoMode = true;

unsigned long lastTransition = 0;
unsigned long lastDisplay = 0;
unsigned long colorStart = 0;

// ================= RGB =================
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  r = min(255, r + 30);
  g = min(255, g + 30);
  b = min(255, b + 30);
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ================= WEB UI =================
String webpage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align: center; background: #111; color: white; }
button {
  width: 120px; height: 50px; margin: 8px;
  font-size: 18px; border: none; border-radius: 10px;
}
.red{background:red;} .green{background:green;}
.blue{background:blue;} .yellow{background:yellow;color:black;}
.cyan{background:cyan;color:black;} .magenta{background:magenta;}
.auto{background:#444;}
</style>
</head>
<body>
<h2>ESP32 RGB Control</h2>

<button class="red" onclick="send('red')">Red</button>
<button class="green" onclick="send('green')">Green</button>
<button class="blue" onclick="send('blue')">Blue</button><br>

<button class="yellow" onclick="send('yellow')">Yellow</button>
<button class="cyan" onclick="send('cyan')">Cyan</button>
<button class="magenta" onclick="send('magenta')">Magenta</button><br>

<button class="auto" onclick="send('auto')">Auto Mode</button>

<script>
function send(color){
  fetch("/set?color=" + color);
}
</script>

</body>
</html>
)rawliteral";
}

// ================= HANDLE REQUEST =================
void handleRoot() {
  server.send(200, "text/html", webpage());
}

void handleColor() {
  String color = server.arg("color");
  autoMode = false;

  if (color == "red") targetColor = {255,0,0,"Red"};
  else if (color == "green") targetColor = {0,255,0,"Green"};
  else if (color == "blue") targetColor = {0,0,255,"Blue"};
  else if (color == "yellow") targetColor = {255,255,0,"Yellow"};
  else if (color == "cyan") targetColor = {0,255,255,"Cyan"};
  else if (color == "magenta") targetColor = {255,0,255,"Magenta"};
  else if (color == "auto") {
    autoMode = true;
  }

  server.send(200, "text/plain", "OK");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(1);

  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 255);

  // WiFi Connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  // Server routes
  server.on("/", handleRoot);
  server.on("/set", handleColor);
  server.begin();

  colorStart = millis();
}

// ================= TRANSITION =================
void smoothTransition() {
  if (millis() - lastTransition < TRANSITION_SPEED) return;
  lastTransition = millis();

  if (currentColor.r < targetColor.r) currentColor.r++;
  else if (currentColor.r > targetColor.r) currentColor.r--;

  if (currentColor.g < targetColor.g) currentColor.g++;
  else if (currentColor.g > targetColor.g) currentColor.g--;

  if (currentColor.b < targetColor.b) currentColor.b++;
  else if (currentColor.b > targetColor.b) currentColor.b--;
}

// ================= DRAW =================
void drawScreen() {
  uint16_t color = rgb565(currentColor.r, currentColor.g, currentColor.b);
  tft.fillScreen(color);
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  smoothTransition();

  // Auto color cycle
  if (autoMode && millis() - colorStart > COLOR_DWELL_TIME) {
    colorStart = millis();
    currentIndex = (currentIndex + 1) % NUM_COLORS;
    targetColor = colors[currentIndex];
  }

  if (millis() - lastDisplay > DISPLAY_UPDATE_RATE) {
    lastDisplay = millis();
    drawScreen();
  }
}