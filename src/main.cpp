#include <Arduino.h>
#include <PN532.h>
#include <PN532_HSU.h>
#include <Adafruit_Fingerprint.h>
#define mySerial Serial1

PN532_HSU pn532hsu(Serial2);
PN532 nfc(pn532hsu);


Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
String tagId = "None", dispTag = "None";
byte nuidPICC[4];
 
void setup(void)
{
  SerialUSB.begin(115200);
  delay(100);
  SerialUSB.println("Hello Maker!");
  
  //  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    SerialUSB.print("Didn't Find PN53x Module");
    //while (1); // Halt
  }
  // Got valid data, print it out!
  SerialUSB.print("Found chip PN5");
  SerialUSB.println((versiondata >> 24) & 0xFF, HEX);
  SerialUSB.print("Firmware ver. ");
  SerialUSB.print((versiondata >> 16) & 0xFF, DEC);
  SerialUSB.print('.'); 
  SerialUSB.println((versiondata >> 8) & 0xFF, DEC);
  // Configure board to read RFID tags
  nfc.SAMConfig();
  //Serial.println("Waiting for an ISO14443A Card ...");
  SerialUSB.println("\n\nAdafruit finger detect test");
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    //while (1) { delay(1); }
  }

}
 
 int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
 
String tagToString(byte id[4])
{
  String tagId = "";
  for (byte i = 0; i < 4; i++)
  {
    if (i < 3) tagId += String(id[i]) + ".";
    else tagId += String(id[i]);
  }
  return tagId;
}
 
 
void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success)
  {
    SerialUSB.print("UID Length: ");
    SerialUSB.print(uidLength, DEC);
    SerialUSB.println(" bytes");
    SerialUSB.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      nuidPICC[i] = uid[i];
      SerialUSB.print(" "); SerialUSB.print(uid[i], DEC);
    }
    SerialUSB.println();
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    SerialUSB.print(F("tagId is : "));
    SerialUSB.println(tagId);
    SerialUSB.println("");
    delay(1000);  // 1 second halt
  }
  /*else
  {
    // PN532 probably timed out waiting for a card
    SerialUSB.println("Timed out! Waiting for a card...");
  }*/
}

void loop()
{
  readNFC();
  getFingerprintIDez();
  delay(100);
}

