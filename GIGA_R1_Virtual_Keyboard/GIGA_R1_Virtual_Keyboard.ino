#include <WiFi.h>
#include "PluggableUSBHID.h"
#include "USBKeyboard.h"
#include "Arduino_H7_Video.h"
#include "ArduinoGraphics.h"
#include "Arduino_GigaDisplayTouch.h"

// ================================================================
//  CONFIG — modifiez ici vos identifiants WiFi
// ================================================================
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";

// ================================================================
//  MODIFICATEURS & CODES HID (norme USB HID Usage Tables)
// ================================================================
#define MOD_NONE    0x00
#define MOD_SHIFT   0x02
#define MOD_ALTGR   0x40
#define MOD_CTRL    0x01

#define HID_KEY_UP_ARROW    0x52
#define HID_KEY_DOWN_ARROW  0x51
#define HID_KEY_LEFT_ARROW  0x50
#define HID_KEY_RIGHT_ARROW 0x4F
#define HID_KEY_ESCAPE      0x29
#define HID_KEY_ENTER       0x28
#define HID_KEY_TAB         0x2B
#define HID_KEY_BACKSPACE   0x2A
#define HID_KEY_SPACE       0x2C
#define HID_KEY_DELETE      0x4C

// ================================================================
//  OBJETS GLOBAUX
// ================================================================
WiFiServer               server(80);
USBKeyboard              Keyboard;
Arduino_H7_Video         Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch touchDetector;

bool          azertyMode    = true;   // true = AZERTY FR, false = QWERTY EN
String        cachedIP;
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_THRESHOLD = 300; // ms anti-rebond

// Zones des boutons tactiles (x, y, largeur, hauteur)
#define BTN_AZERTY_X  80
#define BTN_AZERTY_Y  370
#define BTN_AZERTY_W  260
#define BTN_AZERTY_H  70

#define BTN_QWERTY_X  460
#define BTN_QWERTY_Y  370
#define BTN_QWERTY_W  260
#define BTN_QWERTY_H  70

// ================================================================
//  TABLE DE CONVERSION AZERTY FR
//  Les keycodes HID representent des POSITIONS physiques de touches.
//  Ex: keycode 0x04 = position "A" sur un QWERTY = touche "Q" sur AZERTY
//  L'appareil cible configure en FR interprete correctement chaque position.
// ================================================================
struct KeyMapping {
  char    c;
  uint8_t keycode;
  uint8_t modifier;
};

const KeyMapping azertyMap[] = {
  // Chiffres et symboles (rangee du haut)
  // Positions physiques AZERTY FR : &/1  é/2/~  "/3/#  '/4/{  (/5/[  -/6/|  è/7/`  _/8/\  ç/9/^  à/0/@
  {'&',        0x1E, MOD_NONE},  {'1',        0x1E, MOD_SHIFT},
  {(char)0xE9, 0x1F, MOD_NONE},  {'2',        0x1F, MOD_SHIFT},  {'~', 0x1F, MOD_ALTGR},
  {'3',        0x20, MOD_SHIFT},  {'#',        0x20, MOD_ALTGR},
  {'4',        0x21, MOD_SHIFT},  {'{',        0x21, MOD_ALTGR},
  {'5',        0x22, MOD_SHIFT},  {'[',        0x22, MOD_ALTGR},
  {'-',        0x23, MOD_NONE},   {'6',        0x23, MOD_SHIFT},  {'|', 0x23, MOD_ALTGR},
  {(char)0xE8, 0x24, MOD_NONE},  {'7',        0x24, MOD_SHIFT},  {'`', 0x24, MOD_ALTGR},
  {'_',        0x25, MOD_NONE},   {'8',        0x25, MOD_SHIFT},  {'\\',0x25, MOD_ALTGR},
  {(char)0xE7, 0x26, MOD_NONE},  {'9',        0x26, MOD_SHIFT},  {'^', 0x26, MOD_ALTGR},
  {(char)0xE0, 0x27, MOD_NONE},  {'0',        0x27, MOD_SHIFT},  {'@', 0x27, MOD_ALTGR},

  // Rangee AZERTY
  {'a', 0x14, MOD_NONE},  {'A', 0x14, MOD_SHIFT},
  {'z', 0x1A, MOD_NONE},  {'Z', 0x1A, MOD_SHIFT},
  {'e', 0x08, MOD_NONE},  {'E', 0x08, MOD_SHIFT},
  {'r', 0x15, MOD_NONE},  {'R', 0x15, MOD_SHIFT},
  {'t', 0x17, MOD_NONE},  {'T', 0x17, MOD_SHIFT},
  {'y', 0x1C, MOD_NONE},  {'Y', 0x1C, MOD_SHIFT},
  {'u', 0x18, MOD_NONE},  {'U', 0x18, MOD_SHIFT},
  {'i', 0x0C, MOD_NONE},  {'I', 0x0C, MOD_SHIFT},
  {'o', 0x12, MOD_NONE},  {'O', 0x12, MOD_SHIFT},
  {'p', 0x13, MOD_NONE},  {'P', 0x13, MOD_SHIFT},

  // Rangee QSDF
  {'q', 0x04, MOD_NONE},  {'Q', 0x04, MOD_SHIFT},
  {'s', 0x16, MOD_NONE},  {'S', 0x16, MOD_SHIFT},
  {'d', 0x07, MOD_NONE},  {'D', 0x07, MOD_SHIFT},
  {'f', 0x09, MOD_NONE},  {'F', 0x09, MOD_SHIFT},
  {'g', 0x0A, MOD_NONE},  {'G', 0x0A, MOD_SHIFT},
  {'h', 0x0B, MOD_NONE},  {'H', 0x0B, MOD_SHIFT},
  {'j', 0x0D, MOD_NONE},  {'J', 0x0D, MOD_SHIFT},
  {'k', 0x0E, MOD_NONE},  {'K', 0x0E, MOD_SHIFT},
  {'l', 0x0F, MOD_NONE},  {'L', 0x0F, MOD_SHIFT},
  {'m', 0x33, MOD_NONE},  {'M', 0x33, MOD_SHIFT},

  // Rangee WXCV
  {'w', 0x1D, MOD_NONE},  {'W', 0x1D, MOD_SHIFT},
  {'x', 0x1B, MOD_NONE},  {'X', 0x1B, MOD_SHIFT},
  {'c', 0x06, MOD_NONE},  {'C', 0x06, MOD_SHIFT},
  {'v', 0x19, MOD_NONE},  {'V', 0x19, MOD_SHIFT},
  {'b', 0x05, MOD_NONE},  {'B', 0x05, MOD_SHIFT},
  {'n', 0x11, MOD_NONE},  {'N', 0x11, MOD_SHIFT},

  // Ponctuation AZERTY FR
  {' ', HID_KEY_SPACE, MOD_NONE},
  {'.', 0x36, MOD_SHIFT},
  {',', 0x10, MOD_NONE},
  {';', 0x36, MOD_NONE},
  {':', 0x37, MOD_SHIFT},
  {'!', 0x38, MOD_SHIFT},
  {'?', 0x38, MOD_NONE},
  {'/', 0x37, MOD_NONE},
  {'=',        0x2E, MOD_NONE},
  {'+',        0x2E, MOD_SHIFT},
  {'(',        0x22, MOD_NONE},
  {')',        0x2D, MOD_NONE},         // ) : touche - QWERTY = ) sur AZERTY FR
  {'\'',       0x21, MOD_NONE},         // ' : touche 4 sur AZERTY FR
  {'"',        0x20, MOD_NONE},         // " : touche 3 sur AZERTY FR (sans modif)
  {(char)0xF9, 0x34, MOD_NONE},         // u avec accent grave : touche apostrophe QWERTY
  {'<',        0x64, MOD_NONE},
  {'>',        0x64, MOD_SHIFT},
};
const int AZERTY_MAP_SIZE = sizeof(azertyMap) / sizeof(azertyMap[0]);

// ================================================================
//  FONCTIONS HID
// ================================================================
void sendChar(char c) {
  for (int i = 0; i < AZERTY_MAP_SIZE; i++) {
    if (azertyMap[i].c == c) {
      Keyboard.key_code(azertyMap[i].keycode, azertyMap[i].modifier);
      delay(10);
      return;
    }
  }
  Keyboard.putc(c);
  delay(10);
}

void sendString(String str) {
  for (int i = 0; i < (int)str.length(); ) {
    uint8_t b = (uint8_t)str.charAt(i);
    if (azertyMode) {
      // Convertit les sequences UTF-8 sur 2 octets (Latin-1 etendu U+00C0..U+00FF)
      // en leur valeur Latin-1 pour la recherche dans azertyMap.
      if ((b == 0xC2 || b == 0xC3) && i + 1 < (int)str.length()) {
        uint8_t b2 = (uint8_t)str.charAt(i + 1);
        sendChar((char)((b == 0xC3 ? 0xC0 : 0x80) | (b2 & 0x3F)));
        i += 2;
      } else {
        sendChar((char)b);
        i++;
      }
    } else {
      Keyboard.putc((char)b);
      delay(10);
      i++;
    }
  }
}

// ================================================================
//  DECODAGE URL
// ================================================================
int hexToDec(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

String urldecode(String str) {
  String result = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == '+') {
      result += ' ';
    } else if (c == '%' && i + 2 < str.length()) {
      char code0 = str.charAt(++i);
      char code1 = str.charAt(++i);
      result += (char)((hexToDec(code0) << 4) | hexToDec(code1));
    } else {
      result += c;
    }
  }
  return result;
}

// ================================================================
//  AFFICHAGE ECRAN GIGA DISPLAY
// ================================================================
void drawScreen(String ip) {
  Display.beginDraw();
  Display.background(0, 0, 0);
  Display.clear();

  Display.stroke(0, 200, 255);
  Display.textFont(Font_5x7);
  Display.textSize(3);
  Display.text("Clavier Virtuel", 60, 40);

  Display.stroke(180, 180, 180);
  Display.textSize(2);
  String modeLabel = azertyMode ? "Mode actif : AZERTY FR" : "Mode actif : QWERTY EN";
  Display.text(modeLabel.c_str(), 55, 110);

  Display.stroke(0, 200, 255);
  Display.line(40, 150, 760, 150);

  Display.stroke(150, 150, 150);
  Display.textSize(2);
  Display.text("Connectez-vous sur :", 55, 185);

  Display.stroke(0, 255, 120);
  Display.textSize(4);
  int xPos = max(40, (int)(400 - (int)ip.length() * 12));
  Display.text(ip.c_str(), xPos, 245);

  Display.stroke(120, 120, 120);
  Display.textSize(2);
  Display.text("USB-C  ->  port USB appareil cible", 55, 318);

  // Bouton AZERTY
  if (azertyMode) { Display.fill(0, 150, 65); Display.stroke(0, 255, 120); }
  else            { Display.fill(20, 20, 50);  Display.stroke(60, 60, 100); }
  Display.rect(BTN_AZERTY_X, BTN_AZERTY_Y, BTN_AZERTY_W, BTN_AZERTY_H);
  Display.stroke(255, 255, 255);
  Display.textSize(3);
  Display.text("AZERTY", BTN_AZERTY_X + 38, BTN_AZERTY_Y + 22);

  // Bouton QWERTY
  if (!azertyMode) { Display.fill(0, 150, 65); Display.stroke(0, 255, 120); }
  else             { Display.fill(20, 20, 50);  Display.stroke(60, 60, 100); }
  Display.rect(BTN_QWERTY_X, BTN_QWERTY_Y, BTN_QWERTY_W, BTN_QWERTY_H);
  Display.stroke(255, 255, 255);
  Display.textSize(3);
  Display.text("QWERTY", BTN_QWERTY_X + 32, BTN_QWERTY_Y + 22);

  Display.endDraw();
}

void setMode(bool azerty, const char* source) {
  if (azertyMode == azerty) return;
  azertyMode = azerty;
  drawScreen(cachedIP);
  Serial.print(azerty ? "-> AZERTY (" : "-> QWERTY (");
  Serial.print(source);
  Serial.println(")");
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);

  Display.begin();
  Display.beginDraw();
  Display.background(0, 0, 0);
  Display.clear();
  Display.stroke(255, 165, 0);
  Display.textFont(Font_5x7);
  Display.textSize(3);
  Display.text("Connexion WiFi...", 60, 200);
  Display.endDraw();

  if (!touchDetector.begin()) { Serial.println("WARN: Touch init echouee"); }
  else                        { Serial.println("Touch OK"); }

  Serial.print("Connexion a : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  {
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - wifiStart > 30000) {
        Display.beginDraw();
        Display.background(0, 0, 0); Display.clear();
        Display.stroke(255, 60, 60); Display.textFont(Font_5x7); Display.textSize(2);
        Display.text("WiFi : connexion impossible", 40, 190);
        Display.text("Verifiez SSID/mot de passe", 40, 230);
        Display.endDraw();
        Serial.println("\nErreur : WiFi non disponible");
        while (true) { delay(1000); }
      }
      delay(500);
      Serial.print(".");
    }
  }

  cachedIP = WiFi.localIP().toString();
  Serial.println("\nIP : " + cachedIP);
  drawScreen(cachedIP);
  server.begin();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {

  // Lecture du touch (polling)
  uint8_t    contacts;
  GDTpoint_t points[5];
  contacts = touchDetector.getTouchPoints(points);

  if (contacts > 0 && (millis() - lastTouchTime > TOUCH_THRESHOLD)) {
    lastTouchTime = millis();
    int tx = points[0].x;
    int ty = points[0].y;

    if (tx >= BTN_AZERTY_X && tx <= BTN_AZERTY_X + BTN_AZERTY_W &&
        ty >= BTN_AZERTY_Y && ty <= BTN_AZERTY_Y + BTN_AZERTY_H) {
      setMode(true, "touch");
    }
    if (tx >= BTN_QWERTY_X && tx <= BTN_QWERTY_X + BTN_QWERTY_W &&
        ty >= BTN_QWERTY_Y && ty <= BTN_QWERTY_Y + BTN_QWERTY_H) {
      setMode(false, "touch");
    }
  }

  // Serveur web
  WiFiClient client = server.available();
  if (!client) return;

  String currentLine = "";
  String requestLine  = "";
  bool   firstLine    = true;

  while (client.connected()) {
    if (!client.available()) continue;
    char c = client.read();

    if (c == '\n') {
      if (firstLine) { requestLine = currentLine; firstLine = false; }

      if (currentLine.length() == 0) {

        if (requestLine.startsWith("GET /text?val=")) {
          int start = requestLine.indexOf("val=") + 4;
          int end   = requestLine.indexOf(" HTTP");
          if (end < 0) end = requestLine.length();
          sendString(urldecode(requestLine.substring(start, end)));
        } else if (requestLine.startsWith("GET /key?val=ENTER"))    { Keyboard.key_code(HID_KEY_ENTER, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=TAB"))       { Keyboard.key_code(HID_KEY_TAB, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=BACKSPACE"))  { Keyboard.key_code(HID_KEY_BACKSPACE, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=DEL"))        { Keyboard.key_code(HID_KEY_DELETE, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=ESC"))        { Keyboard.key_code(HID_KEY_ESCAPE, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=UP"))         { Keyboard.key_code(HID_KEY_UP_ARROW, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=DOWN"))       { Keyboard.key_code(HID_KEY_DOWN_ARROW, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=LEFT"))       { Keyboard.key_code(HID_KEY_LEFT_ARROW, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=RIGHT"))      { Keyboard.key_code(HID_KEY_RIGHT_ARROW, MOD_NONE); }
        else if (requestLine.startsWith("GET /key?val=CTRLC"))      { Keyboard.key_code(0x06, MOD_CTRL); }
        else if (requestLine.startsWith("GET /key?val=CTRLZ"))      { Keyboard.key_code(0x1D, MOD_CTRL); }
        else if (requestLine.startsWith("GET /key?val=CTRLD"))      { Keyboard.key_code(0x07, MOD_CTRL); }
        else if (requestLine.startsWith("GET /mode?val=azerty")) {
          setMode(true, "web");
        } else if (requestLine.startsWith("GET /mode?val=qwerty")) {
          setMode(false, "web");
        }

        String modeStr = azertyMode ? "AZERTY FR" : "QWERTY EN";
        String clAZ    = azertyMode ? "btn-active" : "btn-inactive";
        String clQW    = !azertyMode ? "btn-active" : "btn-inactive";

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html; charset=UTF-8");
        client.println("Connection: close");
        client.println();
        client.print("<!DOCTYPE html><html><head>");
        client.print("<meta charset='UTF-8'>");
        client.print("<meta name='viewport' content='width=device-width,initial-scale=1'>");
        client.print("<title>Clavier Virtuel HID</title>");
        client.print("<style>");
        client.print("*{box-sizing:border-box;margin:0;padding:0}");
        client.print("body{font-family:sans-serif;padding:20px;max-width:640px;margin:auto;background:#0d0d1a;color:#eee;}");
        client.print("h2{color:#4fc3f7;margin-bottom:6px;font-size:1.3rem;}");
        client.print("#mode-badge{display:inline-block;padding:4px 14px;border-radius:20px;font-size:.85rem;font-weight:bold;margin-bottom:14px;background:#0f3460;color:#4fc3f7;border:1px solid #4fc3f7;}");
        client.print("input[type=text]{width:100%;padding:12px;font-size:16px;border-radius:8px;border:none;margin-bottom:10px;background:#16213e;color:#fff;}");
        client.print("button{padding:10px 14px;font-size:14px;margin:4px;border:none;border-radius:8px;cursor:pointer;transition:.2s;}");
        client.print(".btn-send{background:#0f3460;color:#fff;}.btn-send:hover{background:#4fc3f7;color:#000;}");
        client.print(".btn-sys{background:#1a2a50;color:#ccc;}.btn-sys:hover{background:#4fc3f7;color:#000;}");
        client.print(".btn-ctrl{background:#2a1040;color:#ccc;}.btn-ctrl:hover{background:#a855f7;color:#fff;}");
        client.print(".btn-active{background:#0a6640;color:#fff;border:2px solid #0f0;font-weight:bold;}");
        client.print(".btn-inactive{background:#1a1a3a;color:#777;border:2px solid #333;}");
        client.print(".arrows{display:inline-grid;grid-template-columns:repeat(3,52px);gap:4px;margin:8px 0;}");
        client.print(".arrows button{padding:10px;font-size:18px;width:52px;background:#1a2a50;color:#fff;}");
        client.print(".arrows button:hover{background:#4fc3f7;color:#000;}");
        client.print("hr{border-color:#1a1a3a;margin:12px 0;}");
        client.print("#status{margin-top:10px;font-size:13px;color:#4fc3f7;min-height:18px;}");
        client.print(".lbl{font-size:11px;color:#555;text-transform:uppercase;letter-spacing:1px;margin:8px 0 4px;}");
        client.print("</style></head><body>");
        client.print("<h2>&#9000; Clavier Virtuel USB HID</h2>");
        client.print("<div id='mode-badge'>Mode : "); client.print(modeStr.c_str()); client.print("</div><br>");
        client.print("<div class='lbl'>Disposition clavier</div>");
        client.print("<button class='"); client.print(clAZ.c_str()); client.print("' onclick='setMode(\"azerty\")'>AZERTY</button>");
        client.print("<button class='"); client.print(clQW.c_str()); client.print("' onclick='setMode(\"qwerty\")'>QWERTY</button>");
        client.print("<hr>");
        client.print("<input type='text' id='txt' placeholder='Tapez votre texte ici...' maxlength='512' autofocus>");
        client.print("<button class='btn-send' onclick='sendText()'>&#9654; Envoyer</button>");
        client.print("<hr>");
        client.print("<div class='lbl'>Touches systeme</div>");
        client.print("<button class='btn-sys' onclick='sendKey(\"ENTER\")'>&#8629; Enter</button>");
        client.print("<button class='btn-sys' onclick='sendKey(\"TAB\")'>&#8677; Tab</button>");
        client.print("<button class='btn-sys' onclick='sendKey(\"BACKSPACE\")'>&#9003; Backspace</button>");
        client.print("<button class='btn-sys' onclick='sendKey(\"DEL\")'>Del</button>");
        client.print("<button class='btn-sys' onclick='sendKey(\"ESC\")'>Esc</button><br>");
        client.print("<div class='lbl'>Raccourcis terminal Linux</div>");
        client.print("<button class='btn-ctrl' onclick='sendKey(\"CTRLC\")'>Ctrl+C</button>");
        client.print("<button class='btn-ctrl' onclick='sendKey(\"CTRLZ\")'>Ctrl+Z</button>");
        client.print("<button class='btn-ctrl' onclick='sendKey(\"CTRLD\")'>Ctrl+D</button>");
        client.print("<hr>");
        client.print("<div class='lbl'>Navigation</div>");
        client.print("<div class='arrows'>");
        client.print("<div></div><button onclick='sendKey(\"UP\")'>&#8593;</button><div></div>");
        client.print("<button onclick='sendKey(\"LEFT\")'>&#8592;</button>");
        client.print("<button onclick='sendKey(\"DOWN\")'>&#8595;</button>");
        client.print("<button onclick='sendKey(\"RIGHT\")'>&#8594;</button>");
        client.print("</div>");
        client.print("<div id='status'></div>");
        client.print("<script>");
        client.print("var s=document.getElementById('status');");
        client.print("function sendText(){var t=document.getElementById('txt').value;if(!t)return;fetch('/text?val='+encodeURIComponent(t)).then(()=>{s.textContent='Envoye: '+t;document.getElementById('txt').value='';});}");
        client.print("function sendKey(k){fetch('/key?val='+k).then(()=>{s.textContent='Touche: '+k;});}");
        client.print("function setMode(m){fetch('/mode?val='+m).then(()=>location.reload());}");
        client.print("document.getElementById('txt').addEventListener('keydown',function(e){if(e.key==='Enter'){e.preventDefault();sendText();}});");
        client.print("</script></body></html>");
        client.println();
        break;
      }
      currentLine = "";
    } else if (c != '\r') {
      if (currentLine.length() < 512) currentLine += c;
    }
  }
  delay(5);
  client.stop();
}
