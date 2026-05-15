#include "spo2.h"

float spo2OndaGlobal = 0.0;
int pctGlobal = 0;

const int pinRojo = 26;//19; 
const int pinIR = 27;//18;   
const int pinSensor = 33;//35; 

const unsigned long tiempoEncendido = 2000; 
const unsigned long tiempoApagado = 2000;   

enum FaseSpO2 { ROJO_ON, OSCURIDAD_1, IR_ON, OSCURIDAD_2 };
FaseSpO2 faseActual = ROJO_ON;

unsigned long tiempoUltimaFaseMicro = 0;

int lecturaRojoCrudo = 0;
int lecturaOscuro1 = 0;
int lecturaIRCrudo = 0;
int lecturaOscuro2 = 0;

float rojoDC = 0.0;
float irDC = 0.0;
float rojoAC_Filtrado = 0.0;
float irAC_Filtrado = 0.0;
const float alpha = 0.10; 

const int VENTANA_MUESTRAS = 250; 
int contadorMuestras = 0;

float maxRojo = -9999.0;
float minRojo = 9999.0;
float maxIR = -9999.0;
float minIR = 9999.0;

unsigned long tiempoActualMicro = 0;

int rojoLimpio = 0;
int irLimpio = 0;
float rojoAC = 0.0;
float irAC = 0.0;

float pleth_mV = 0.0;
float acRojo = 0.0;
float acIR = 0.0;

float ratioRojo = 0.0;
float ratioIR = 0.0;
float R = 0.0;
float spo2Final = 0.0;

void setupSpO2() {
  pinMode(pinRojo, OUTPUT);
  pinMode(pinIR, OUTPUT);
  digitalWrite(pinRojo, LOW);
  digitalWrite(pinIR, LOW);
  
  delay(2000); 
  tiempoUltimaFaseMicro = micros();
}

void procesarSpO2() {
  tiempoActualMicro = micros();

  switch (faseActual) {
    
    case ROJO_ON:
      digitalWrite(pinRojo, HIGH); 
      digitalWrite(pinIR, LOW);   
      if (tiempoActualMicro - tiempoUltimaFaseMicro >= (tiempoEncendido - 300)) {
        lecturaRojoCrudo = analogRead(pinSensor); 
        tiempoUltimaFaseMicro = micros();
        faseActual = OSCURIDAD_1;
      }
      break;

    case OSCURIDAD_1:
      digitalWrite(pinRojo, LOW);
      digitalWrite(pinIR, LOW);  
      if (tiempoActualMicro - tiempoUltimaFaseMicro >= (tiempoApagado - 300)) {
        lecturaOscuro1 = analogRead(pinSensor);
        tiempoUltimaFaseMicro = micros();
        faseActual = IR_ON;
      }
      break;

    case IR_ON:
      digitalWrite(pinRojo, LOW); 
      digitalWrite(pinIR, HIGH);  
      if (tiempoActualMicro - tiempoUltimaFaseMicro >= (tiempoEncendido - 300)) {
        lecturaIRCrudo = analogRead(pinSensor);
        tiempoUltimaFaseMicro = micros();
        faseActual = OSCURIDAD_2;
      }
      break;

    case OSCURIDAD_2:
      digitalWrite(pinRojo, LOW); 
      digitalWrite(pinIR, LOW);
      if (tiempoActualMicro - tiempoUltimaFaseMicro >= (tiempoApagado - 300)) {
        lecturaOscuro2 = analogRead(pinSensor);
        tiempoUltimaFaseMicro = micros();
        faseActual = ROJO_ON;

        rojoLimpio = lecturaRojoCrudo - lecturaOscuro1;
        irLimpio = lecturaIRCrudo - lecturaOscuro2;

        if (rojoLimpio < 0) rojoLimpio = 0;
        if (irLimpio < 0) irLimpio = 0;

        rojoDC = (0.01 * rojoLimpio) + (0.99 * rojoDC);
        irDC = (0.01 * irLimpio) + (0.99 * irDC);

        rojoAC = rojoLimpio - rojoDC;
        irAC = irLimpio - irDC;

        rojoAC_Filtrado = (alpha * rojoAC) + ((1.0 - alpha) * rojoAC_Filtrado);
        irAC_Filtrado = (alpha * irAC) + ((1.0 - alpha) * irAC_Filtrado);

        if (rojoAC_Filtrado > maxRojo) maxRojo = rojoAC_Filtrado;
        if (rojoAC_Filtrado < minRojo) minRojo = rojoAC_Filtrado;
        if (irAC_Filtrado > maxIR) maxIR = irAC_Filtrado;
        if (irAC_Filtrado < minIR) minIR = irAC_Filtrado;

        pleth_mV = (irAC_Filtrado * 3300.0) / 4095.0;
        spo2OndaGlobal = pleth_mV;

        contadorMuestras++;

        if (contadorMuestras >= VENTANA_MUESTRAS) {
          acRojo = maxRojo - minRojo;
          acIR = maxIR - minIR;

          if (rojoDC > 50.0 && irDC > 50.0 && acRojo > 1.0 && acIR > 1.0) {
            ratioRojo = acRojo / rojoDC;
            ratioIR = acIR / irDC;
            R = ratioRojo / ratioIR;

            spo2Final = 110.0 - (25.0 * R);

            if (spo2Final > 100.0) spo2Final = 100.0;
            if (spo2Final < 0.0) spo2Final = 0.0;

            pctGlobal = (int)spo2Final;
          }
          maxRojo = -9999.0; minRojo = 9999.0;
          maxIR = -9999.0; minIR = 9999.0;
          contadorMuestras = 0;
        }
      }
      break;
  }
}