#include <Arduino.h>
#include <PN532.h>
#include <PN532_HSU.h>
#include <Adafruit_Fingerprint.h>
#include <FreeRTOS.h> 
#include <semphr.h> 
#include <ArduinoJson.h>
#define mySerial Serial1
SemaphoreHandle_t mutex_v; 
PN532_HSU pn532hsu(Serial2);
PN532 nfc(pn532hsu);
int touchID;
char* nfcID;
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
  Serial.begin(115200);
  while(!Serial){
    delay(1);
  }
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't Find PN53x Module");
  }
  // Got valid data, print it out!
  Serial.print("Found chip PN5");
  // Configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("\n\nAdafruit finger detect test");
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
  }
  sem_v = xSemaphoreCreateBinary();
  if (sem_v == NULL) { 
        Serial.println("Mutex can not be created"); 
    } 
    xTaskCreate(bookGive, "Task1", 128, NULL, 1, NULL); 
    xTaskCreate(bookWrite, "Task2", 128, NULL, 2, NULL);
    Serial.println("tasks created");
}
 
 bool getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return true;
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
 
 
bool readNFC()
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
  return true;
} 


StaticJsonDocument<96> bookDoc;
StaticJsonDocument<96> fingerDoc;


JsonObject book = bookDoc.createNestedObject("book");
book["161.162.95.29"] = "Три товарища";
book["3.222.220.182"] = "Понедельник начинается в субботу";

JsonObject person = fingerDoc.createNestedObject("person");
person["1"] = "Джон Смит";
person["3"] = "Алекс Уайт";

DynamicJsonDocument listDoc;

void bookWrite(void *pvParameters){
  Serial.println("bookWrite");
  while(1){
  if(xSemaphoreTake(mutex_v, portMAX_DELAY)){
    ind id = listDoc.capacity();
    Serial.println("Semaphore Taken");
    for(int i; i < id; i++){
        if(listDoc[to_string(i)]["book"] == bookDoc["book"]["161.162.95.29"] || listDoc[to_string(i)]["book"] == bookDoc["book"]["3.222.220.182"]){
            listDoc.remove(i);
        }
    }
    JsonObject id = listDoc.createNestedObject(to_string(id));
        id["book"] = bookDoc["book"][nfcID];
        id["person"] = fingerDoc["person"][touchID];
    }
  }
}


void bookGive(void *pvParameters){
  Serial.println("accessCheck");
  while(1){
    Serial.println("Provide book tag\n");
    while(!readNFC()){
        Serial.print(".");
    }
    Serial.println("Provide fingerprint\n");
    while(!getFingerprintIDez()){
        Serial.print(".");
    }
    for(int i = 0; i < 2; i++){
        if(finger.fingerID == fingerDoc["person"][to_string(i)]){
            touchID = finger.fingerID;
        }
        if(nfcID == bookDoc["book"]["161.162.95.29"] || nfcID == bookDoc["book"]["3.222.220.182"]){
            nfcID = tagId;
        }
    }
    Serial.println("Semaphore Released");
    xSemaphoreGive(mutex_v);
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

void loop()
{
    
}



