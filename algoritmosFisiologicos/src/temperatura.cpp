#include "temperatura.h"

float tempGlobal = 0.0;

const int PIN_TEMP = 35;//33; 
const float RESISTENCIA_FIJA = 10000.0; 
const float RESISTENCIA_NOMINAL = 2252.0; 
const float TEMP_NOMINAL = 25.0; 
const float BETA = 4950.0; 
const float alpha = 1.34; 

unsigned long tiempoAnteriorTemp = 0;
const unsigned long intervaloTemp = 1000; 

unsigned long tiempoActual = 0;
int valorADC = 0;
float Rt = 0.0;
float tempKelvin = 0.0;
float tempCelsius = 0.0;

void setupTemperatura() {
  
}

void procesarTemperatura() {
  tiempoActual = millis();

  if (tiempoActual - tiempoAnteriorTemp >= intervaloTemp) {
    tiempoAnteriorTemp = tiempoActual;

    valorADC = analogRead(PIN_TEMP) * alpha;

    if(valorADC == 0 || valorADC >= 4095) { 
        return; 
    }

    Rt = RESISTENCIA_FIJA * (valorADC / (4095.0 - valorADC));

    tempKelvin = Rt / RESISTENCIA_NOMINAL;          
    tempKelvin = log(tempKelvin);                   
    tempKelvin = (1.0 / BETA) * tempKelvin;         
    tempKelvin = tempKelvin + (1.0 / (TEMP_NOMINAL + 273.15)); 
    tempKelvin = 1.0 / tempKelvin;                  

    tempCelsius = tempKelvin - 273.15;

    tempGlobal = tempCelsius;
  }
}