#include <Arduino.h>
#include "Ampel.hpp"
#include "Driver.hpp"
#include "main.h"

const uint8_t redLed = 21;  
const uint8_t yellowLed = 22; 
const uint8_t greenLed = 23;   
const uint8_t bttn = 4;
const TickType_t xTimeInTicks = pdMS_TO_TICKS(1);

QueueHandle_t xQueue = nullptr;

unsigned long ampelTime = 0;
Ampel Sm;


void setup() {
  Serial.begin(9600);
  initBttn();
  Ampel_ctor(&Sm);
  Ampel_start(&Sm);
  xQueue = xQueueCreate(15, sizeof(int32_t));
  if(xQueue == NULL) {
    Serial.println("Message-Queue could not be created");
  }
  xTaskCreate(vSenderAmpel, "Ampel Tick", 2048, NULL, 1, NULL);    
  xTaskCreate(vSenderSerialCom, "Serial Communication", 4096, NULL, 1, NULL);
  xTaskCreate(vReceiverTask, "Receiver", 4096, NULL, 3, NULL);
  attachInterrupt(digitalPinToInterrupt(bttn),onButtonChange, CHANGE);
  infoAusgabe();
}

void loop() {}

static void tick(Ampel* sm) { 
  Ampel_dispatch_event(sm, Ampel_EventId_TICK);
}

static void autoRequestGreen(Ampel* sm) {
  constexpr unsigned long MIN_RED_TICKS = 10000; // ~1s bei 1ms-Ticks -> Hier 10 Sekunden
  if (sm->state_id == Ampel_StateId_TRAFFICLIGHTSTATERED && ampelTime >= MIN_RED_TICKS) {
    Ampel_dispatch_event(sm, Ampel_EventId_REQUESTGREEN);
  }
}

void initRedLight() { pinMode(redLed,OUTPUT);}
void initYellowLight() { pinMode(yellowLed,OUTPUT);}
void initGreenLight() { pinMode(greenLed,OUTPUT);}
void initBttn() {pinMode(bttn, INPUT_PULLUP);}

void turnOnRedLight() { digitalWrite(redLed,HIGH);}
void turnOffRedLight() { digitalWrite(redLed,LOW);}
void turnOnYellowLight() { digitalWrite(yellowLed,HIGH);}
void turnOffYellowLight() { digitalWrite(yellowLed,LOW);}
void turnOnGreenLight() { digitalWrite(greenLed,HIGH);}
void turnOffGreenLight() { digitalWrite(greenLed,LOW);}

void infoAusgabe() {
  Serial.println("\nWählen Sie einen Eintrag per Eingabe der Nummer: ");
  Serial.println("1. Auslesen des Leucht-Zustands einer Ampel");
  Serial.println("2. Setzen des Requests bei einer Ampel");
  Serial.println("\nAuswahl: __");
}

static void handleSerial() {    // regelt die Ausgabe auf dem Serial-Monitor
  while(Serial.available() > 0) {
    int input = Serial.read();  // speichert Eingabe

    if(input == '\r' || input == '\n') continue;  //wenn keine Eingabe, fahre fort 

    switch(input) {
      case '1': { //gibt aktuellen State aus
        Serial.println(Ampel_state_id_to_string(Sm.state_id));  
        break;
      }
      case '2': { //fordert Grün an
        Serial.println(F("Grün wird angefordert..."));
        Ampel_dispatch_event(&Sm, Ampel_EventId_REQUESTGREEN);
        break;
      }
      default:  //falsche Eingabe behandeln
        Serial.println(F("Ungültige Auswahl."));
        infoAusgabe();
        break;
    }
  }
}

static void vSenderAmpel(void *pvParameters) {  //Sender Task für automatisches Durchlaufen der Ampel
  (void) pvParameters;
  const int32_t lValueToSend = 1; // Sendet 1 in Message-Queue
  for(;;) {
    BaseType_t xStatus = xQueueSendToBack(xQueue, &lValueToSend,portMAX_DELAY);
    if(xStatus != pdPASS) {
      Serial.println("Could not send WorkingMessage to MessageQueue");
    }
    vTaskDelay(xTimeInTicks); //jede ms ein Delay für korrekten Tick
  }
}

static void vSenderSerialCom(void *pvParameters) {
  (void) pvParameters;
  for(;;) {
    if(Serial.available() > 0) {
      const int32_t lValueToSend = 3; //Sendet 3 in Message-Queue
      BaseType_t xStatus = xQueueSendToBack(xQueue, &lValueToSend, portMAX_DELAY);
      if(xStatus != pdPASS) {
        Serial.println("Could not send SerialMessage to MessageQueue");
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    } else {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

static void vReceiverTask(void *pvParameters) {
  (void) pvParameters;
  int32_t lReceivedValue;
  for(;;) {
    BaseType_t xStatus = xQueueReceive(xQueue, &lReceivedValue,portMAX_DELAY);
    if(xStatus == pdPASS) {
      switch(lReceivedValue) {
        case 1: {
          tick(&Sm);
          autoRequestGreen(&Sm);
          break;
        }
        case 2: {
          if (Sm.state_id == Ampel_StateId_TRAFFICLIGHTSTATERED) {
            Serial.println("Button: Grün wird angefordert");
            Ampel_dispatch_event(&Sm, Ampel_EventId_REQUESTGREEN);
          }
          break;
        }
        case 3: {
          handleSerial();
          break;
        }
        default: {
          Serial.println("Unknown Message");
          break;
        }
      }
    } else {
      Serial.println("Keine Task erhalten");
    }
  }
}


void onButtonChange() {
  if(xQueue == NULL) {  //Verhindern von Fehlern, wenn Button während des Boot-Vorgangs gedrückt wird.
    return;
  }
  const int32_t lValueToSend = 2; //Sendet 2 in Message-Queue
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendToBackFromISR(xQueue, &lValueToSend, &xHigherPriorityTaskWoken);
}
