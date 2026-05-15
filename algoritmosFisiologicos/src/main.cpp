#include <Arduino.h>

#include "temperatura.h"
#include "pani.h"
#include "spo2.h"
#include "ecg.h"

extern float ecgGlobal;
extern int bpmGlobal;
extern float spo2OndaGlobal;
extern int pctGlobal;
extern float tempGlobal;
extern int sysGlobal;
extern int diaGlobal;

unsigned long tiempoAnterior = 0;
unsigned long tiempoActual = 0;

void setup() {
  Serial.begin(230400);
  delay(500); 

  setupTemperatura();
  setupPANI();
  setupSpO2();
  setupECG();
}

void loop() {
  tiempoActual = millis();

  if (tiempoActual - tiempoAnterior >= 10) {

    tiempoAnterior = tiempoActual;
  
    procesarECG();         
    procesarSpO2();        
    procesarPANI();        
    procesarTemperatura(); 

    Serial.printf("{\"ecg\":%.3f,\"spo2\":%.3f,\"temp\":%.1f,\"sys\":%d,\"dia\":%d,\"pct\":%d,\"bpm\":%d}\n", 
                  ecgGlobal, spo2OndaGlobal, tempGlobal, sysGlobal, diaGlobal, pctGlobal, bpmGlobal);
  }
}