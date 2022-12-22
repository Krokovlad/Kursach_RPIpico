#include <Arduino.h>
#include <PN532.h>
#include <PN532_HSU.h>
#include <Adafruit_Fingerprint.h>
#include <FreeRTOS.h> 
#include <semphr.h> 
#define mySerial Serial1
SemaphoreHandle_t mutex_v; 
PN532_HSU pn532hsu(Serial2);
PN532 nfc(pn532hsu);
TaskHandle_t xHandle;
TaskHandle_t xHandle1;
/*
ПИНЫ:
  Датчик Отпечатков:
    -Чёрный - RX:GPIO0
    -Жёлтый - TX:GPIO1
  RFID:
    -Серый - TX:GPIO8
    -Белый - RX:GPIO9
*/

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
String tagId = "None", dispTag = "None";
byte nuidPICC[4];
 
void setup(void)
{
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  while(!Serial){
    delay(1);
  }
  Serial.println("Hello Maker!");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't Find PN53x Module");
  }
  // Got valid data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  // Configure board to read RFID tags
  nfc.SAMConfig();
  //Serial.println("Waiting for an ISO14443A Card ...");
  Serial.println("\n\nAdafruit finger detect test");
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    //while (1) { delay(1); }
  }
  mutex_v = xSemaphoreCreateBinary();
  if (mutex_v == NULL) { 
        Serial.println("Mutex can not be created"); 
    } 
    
  xTaskCreate(accessCheck, "Task1", 128, NULL, 1, &xHandle1); 
  xTaskCreate(grantAccess, "Task2", 128, NULL, 2, &xHandle);

  Serial.println("tasks created");
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
 
 
String readNFC()
{
  tagId = "None";
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success)
  {
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      nuidPICC[i] = uid[i];
      Serial.print(" "); Serial.print(uid[i], DEC);
    }
    Serial.println();
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    Serial.print(F("tagId is : "));
    Serial.println(tagId);
    Serial.println("");
    delay(100);  // 1 second halt
  }
  return tagId;
} 

void grantAccess(void *pvParameters){
  Serial.println("grantAccess");
  while(1){
  if(xSemaphoreTake(mutex_v, portMAX_DELAY)){
    Serial.println("Semaphore Taken");
    digitalWrite(2, HIGH);
    vTaskDelay(5000/portTICK_PERIOD_MS);
    digitalWrite(2, LOW);
    //xSemaphoreGive(mutex_v); 
    }
  }
}

void accessCheck(void *pvParameters){
  Serial.println("accessCheck");
  //xSemaphoreTake(mutex_v, portMAX_DELAY);
  while(1){
    Serial.print(".");
    int fingerID = getFingerprintIDez();
    String nfcID = readNFC();
    if(fingerID == 1 || nfcID == "161.162.95.29"){
        Serial.println("Semaphore Released");
        xSemaphoreGive(mutex_v);
    }
    vTaskDelay(100/portTICK_PERIOD_MS); //заменить на yield
  }
}

void loop()
{
  //accessCheck();
}

