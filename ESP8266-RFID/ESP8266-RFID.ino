/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read data from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
   So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
   details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
   keep the PICCs at reading distance until complete.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <SPI.h>
#include <MFRC522.h>
#include <RTClib.h>
RTC_DS1307 RTC;

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4);

#include <ESP8266WiFi.h>


#define RST_PIN         0         // Configurable, see typical pin layout above
#define SS_PIN          2         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

const int buzzer = 10;

// Wi-Fi Settings
const char* ssid = "RGNET"; // your wireless network name (SSID)
const char* password = "apocrifa11manana"; // your Wi-Fi network password

WiFiClient client;

// ThingSpeak Settings
String APIKey = "vABED66E5DE876F6"; // write API key for your ThingSpeak Channel
const char* server = "api.pushingbox.com";

// Variable Setup
long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;

DateTime now;

void setup() {
  Serial.begin(115200);		      // Initialize serial communications with the PC
  while (!Serial);		          // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

  // Configura el pin para el buzzer
  pinMode(buzzer, OUTPUT);

  // Inicialilza el display LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.home();

  // Inicio la conexión a la red Wifi
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Conectando a " + String(ssid) + ": ");
  lcd.print("Conectando a " + String(ssid) + ": ");
  lcd.setCursor(0, 1);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Conectado");

  lcd.setCursor(0, 2);
  lcd.print("Conectado!");
  lcd.setCursor(0, 3);
  lcd.print("Intensidad: " + String(WiFi.RSSI()) + "db");
  tone(buzzer, 500);
  delay(250);
  tone(buzzer, 750);
  delay(250);
  noTone(buzzer);
  delay(2000);
  lcd.clear();
  // Fin de conexión a Wifi

  // Inicializa el protocolo SPI para la conexión al lector RFID
  SPI.begin();			          // Init SPI bus
  mfrc522.PCD_Init();		      // Init MFRC522

  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // Si no hay nuevas tarjetas despliega un mensaje de espera en el LCD
  lcd.home();
  lcd.print("Colegio Copilli");
  lcd.setCursor(0, 1);
  lcd.print("Pase la tarjeta:");
  /*lcd.setCursor(0, 2);
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);
    lcd.print(' ');
    lcd.setCursor(0, 3);
    if (now.hour() < 10)
    lcd.print('0');
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute() < 10)
    lcd.print('0');
    lcd.print(now.minute(), DEC);
  */

}

void loop() {

  //-------------------------------------------
  // Imprime la respuesta del servidor en el monitor serial
  // Este bloque debe ir aquí para que se procese en el loop
  // Mientras la conexión siga viva
  // ------------------------------------------
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
    return;
  }

  // Se desconecta del servidor
  if (!client.connected() && lastConnected)
  {
    Serial.println("...desconectado");
    Serial.println();

    client.stop();
  }

  //--------------------------------------------
  // Revisa si se debe reiniciar la conexión a WiFi
  if (failedCounter > 3 ) {
    WiFi.status();
  }
  lastConnected = client.connected();

  // Se actualiza la hora para desplegarla
  //now = RTC.now();

  // Busca nuevas tarjetas
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Si se lee una nueva tarjeta
  Serial.println("-------");
  String uid = getID();
  if (uid != "") {
    // Imprime info en el puerto serial
    Serial.print("Card detected, UID: "); Serial.println(uid);

    // Imprime info en el LCD
    lcd.clear();
    lcd.home();
    lcd.print("Tarjeta detectada!");
    lcd.setCursor(0, 1);
    lcd.print("ID de tarjeta: ");
    lcd.setCursor(0, 2);
    lcd.print(uid);
    lcd.setCursor(0, 3);
    lcd.print("Desayuno descontado");
    tone(buzzer, 1000);
    delay(500);
    noTone(buzzer);

    // Guarda el dato en la hoja de google
    updateData(uid);

    lcd.clear();
  }

  // Si no hay nuevas tarjetas despliega un mensaje de espera en el LCD
  lcd.home();
  lcd.print("Colegio Copilli");
  lcd.setCursor(0, 1);
  lcd.print("Pase la tarjeta:");
  /*lcd.setCursor(0, 2);
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);
    lcd.print(' ');
    lcd.setCursor(0, 3);
    if (now.hour() < 10)
    lcd.print('0');
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    if (now.minute() < 10)
    lcd.print('0');
    lcd.print(now.minute(), DEC);
  */

}

/**
   mfrc522.PICC_IsNewCardPresent() should be checked before
   @return the card UID
*/
String getID() {
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return "";
  }
  String strUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      strUID += "0";
    }
    else {
      strUID += "";
    }
    strUID += String(mfrc522.uid.uidByte[i], HEX);
    if (i + 1 != mfrc522.uid.size) strUID += ":";
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return strUID;
}

void updateData(String uid)
{

  //Start or API service using our WiFi Client through PushingBox
  if (client.connect(server, 80))
  {
    client.print("GET /pushingbox?devid=" + APIKey
                 + "&UID=" + uid );
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: ESP8266/1.0");
    client.println("Connection: close");
    client.println();


    lastConnectionTime = millis();

    if (client.connected())
    {
      Serial.println("Connecting to Punishbox...");
      Serial.println();

      failedCounter = 0;
    }
    else
    {
      failedCounter++;

      Serial.println("Connection to Punishbox failed (" + String(failedCounter, DEC) + ")");
      Serial.println();
    }

  }
  else
  {
    failedCounter++;

    Serial.println("Connection to ThingSpeak Failed (" + String(failedCounter, DEC) + ")");
    Serial.println();

    lastConnectionTime = millis();
  }

}
