#include "ecg.h"

const int PIN_ECG_Crudo = 34;//32;
const int PIN_LO_positivo = 18;//16;
const int PIN_LO_negativo = 5;//17;

float ecg_DC = 2000.0; 
float ecg_AC = 0;
const float UMBRAL_BPM = 1000;//90.0; //60.0; 
const unsigned long PERIODO_REFRACTARIO = 300; 
unsigned long tiempoUltimoLatido = 0;
const int TAMANO_FILTRO = 12;

int historialBPM[TAMANO_FILTRO]; 
int indiceBPM = 0;
int bpmUltimoValido = 70;

float ecgGlobal = 0.0; 
int bpmGlobal = 0;
bool electrodosConectados = false;

int lecturaCruda_ecg = 0;
unsigned long tiempoActual_ecg = 0;

unsigned long deltaT = 0;
int bpmInstantaneo = 0;

long suma = 0;
int i_ecg = 0;

void setupECG() {
  pinMode(PIN_LO_positivo, INPUT);
  pinMode(PIN_LO_negativo, INPUT);
}

void procesarECG() {
  lecturaCruda_ecg = analogRead(PIN_ECG_Crudo);
  
  if ((digitalRead(PIN_LO_positivo) == 0) || (digitalRead(PIN_LO_negativo) == 0)) {  
    electrodosConectados = true;

    ecg_DC = (0.001 * lecturaCruda_ecg) + (0.999 * ecg_DC);
    ecg_AC = lecturaCruda_ecg - ecg_DC;
    
    ecgGlobal = (ecg_AC * 3300.0) / 4095.0; 

    tiempoActual_ecg = millis();
    
    if (ecg_AC > UMBRAL_BPM && (tiempoActual_ecg - tiempoUltimoLatido) > PERIODO_REFRACTARIO) {
      deltaT = tiempoActual_ecg - tiempoUltimoLatido;
      tiempoUltimoLatido = tiempoActual_ecg;

      if (deltaT > 300 && deltaT < 2000) {
        bpmInstantaneo = 60000 / deltaT; 

        if (abs(bpmInstantaneo - bpmUltimoValido) < 30) {
            historialBPM[indiceBPM] = bpmInstantaneo;
            indiceBPM = (indiceBPM + 1) % TAMANO_FILTRO;
            bpmUltimoValido = bpmInstantaneo;
        }

        suma = 0;
        for (i_ecg = 0; i_ecg < TAMANO_FILTRO; i_ecg++) {
          suma += historialBPM[i_ecg];
        }
        bpmGlobal = suma / TAMANO_FILTRO;
      }
    }
  } else {
    electrodosConectados = false;
    ecgGlobal = 0.0;
    bpmGlobal = 0;
    for(i_ecg=0; i_ecg<TAMANO_FILTRO; i_ecg++) historialBPM[i_ecg] = 0;
  }
}