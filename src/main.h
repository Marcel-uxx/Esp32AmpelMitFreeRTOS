#ifndef MAIN_H
#define MAIN_H

void initRedLight();
void initYellowLight();
void initGreenLight();
void initBttn();
void turnOnRedLight();
void turnOffRedLight();
void turnOnYellowLight();
void turnOffYellowLight();
void turnOnGreenLight();
void turnOffGreenLight();

static void tick(Ampel *sm);
static void autoRequestGreen(Ampel *sm);

void infoAusgabe();
static void handleSerial();

static void vSenderAmpel(void *pvParameters);
static void vSenderSerialCom(void *pvParameters);
static void vReceiverTask(void *pvParameters);

void onButtonChange();


#endif
