#include "pani.h"

int sysGlobal = 0;
int diaGlobal = 0;

const int PIN_DATOS = 21;
const int PIN_RELOJ = 22;
const int PIN_BOMBA = 16;//4;//26;   
const int PIN_VALVULA = 4;//16;//27; 

HX711 sensorPresion;

const float FACTOR_ESCALA = 44687.63; 
const float PRESION_OBJETIVO = 160;
const float PRESION_MINIMA = 40.0; //40.0;    

enum EstadoPANI { 
  REPOSO, 
  PREPARANDO_INFLADO, 
  INFLANDO, 
  ESTABILIZANDO_INFLADO, 
  MIDIENDO, 
  ESCALON_VALVULA, 
  CALCULANDO 
};

EstadoPANI estadoActual = REPOSO;

float presionDC = 0.0; 
float presionAC = 0.0; 

const int MAX_MUESTRAS = 250; 
float historialDC[MAX_MUESTRAS];
float historialAC[MAX_MUESTRAS];
float historialAC_Suavizado[MAX_MUESTRAS];
int indiceMuestra = 0;

unsigned long tiempoUltimoPaso = 0;
unsigned long cronometroTransicion = 0; 
int tiempoAperturaDinamico = 0;

const int INTERVALO_PASO_MS = 1200; 
bool nuevoPaso = true;
float maxPresionPaso = 0.0;
float minPresionPaso = 999.0;

char cmd = 'r';
float presionActual = 0.0;
float lecturaCruda = 0.0;
float acCrudo = 0.0;
float centroPAM = 90.0;
float anchoCampana = 28.0;
float pesoGaussiano = 0.0;

int i = 0;
float maxOscilacion = 0.0;
int indicePAM = 0;
float PAM = 0.0;
float sistole = 0.0;
float umbralSistolico = 0.0;
float diastole = 0.0;
float umbralDiastolico = 0.0;

void setupPANI() {
  pinMode(PIN_BOMBA, OUTPUT);
  pinMode(PIN_VALVULA, OUTPUT);
  
  digitalWrite(PIN_BOMBA, LOW);
  digitalWrite(PIN_VALVULA, LOW);
  
  sensorPresion.begin(PIN_DATOS, PIN_RELOJ);
  sensorPresion.set_gain(64); 
  
  delay(3000); 
  
  sensorPresion.tare(20);
  sensorPresion.set_scale(FACTOR_ESCALA);

}

void iniciarPANI_Medicion() {
  if (estadoActual == REPOSO) {
    digitalWrite(PIN_VALVULA, HIGH);
    cronometroTransicion = millis(); 
    estadoActual = PREPARANDO_INFLADO;
    indiceMuestra = 0; 
  }
}

void detenerPANI_Emergencia() {
  if (estadoActual != REPOSO) {
    digitalWrite(PIN_VALVULA, LOW); 
    digitalWrite(PIN_BOMBA, LOW);
    estadoActual = REPOSO;
  }
}

void procesarPANI() {
  if (Serial.available() > 0) {
    cmd = Serial.read();
    if (cmd == 'I') iniciarPANI_Medicion();
    else if (cmd == 'P') detenerPANI_Emergencia();
  }

  switch (estadoActual) {
    case REPOSO:
      // Todo se mantiene apagado
      break;

    case PREPARANDO_INFLADO:
      if (millis() - cronometroTransicion >= 2000) {
        estadoActual = INFLANDO;
      }
      break;

    case INFLANDO:
      digitalWrite(PIN_BOMBA, HIGH);
      
      if (sensorPresion.is_ready()) {
        presionActual = sensorPresion.get_units(1);
        if(presionActual < 0.0) presionActual = 0.0; 
        
        if (presionActual >= PRESION_OBJETIVO) {
          digitalWrite(PIN_BOMBA, LOW); 
          
          cronometroTransicion = millis(); 
          estadoActual = ESTABILIZANDO_INFLADO;
        }
      }
      break;

    case ESTABILIZANDO_INFLADO:
      if (millis() - cronometroTransicion >= 800) {
        tiempoUltimoPaso = millis();
        nuevoPaso = true;
        estadoActual = MIDIENDO;
      }
      break;

    case MIDIENDO:
      digitalWrite(PIN_VALVULA, HIGH); 

      if (nuevoPaso) {
        maxPresionPaso = 0.0;
        minPresionPaso = 999.0;
        nuevoPaso = false;
      }

      if (sensorPresion.is_ready()) {
        lecturaCruda = sensorPresion.get_units(1);
        if(lecturaCruda < 1.0) lecturaCruda = 0.0;

        if (millis() - tiempoUltimoPaso > 400) {
            if (lecturaCruda > maxPresionPaso) maxPresionPaso = lecturaCruda;
            if (lecturaCruda < minPresionPaso && lecturaCruda > 0) minPresionPaso = lecturaCruda;
        }

        if (millis() - tiempoUltimoPaso >= INTERVALO_PASO_MS) {
          presionDC = minPresionPaso; 
          acCrudo = maxPresionPaso - minPresionPaso; 

          pesoGaussiano = exp(-pow((presionDC - centroPAM) / anchoCampana, 2));
          presionAC = acCrudo * pesoGaussiano;

          if (presionAC < 0.05 || presionAC > 15.0) {
            presionAC = 0.0; 
          }

          if (indiceMuestra < MAX_MUESTRAS && presionDC <= 130.0) {
            historialDC[indiceMuestra] = presionDC;
            historialAC[indiceMuestra] = presionAC; 
            indiceMuestra++;
          }

          if (presionDC < PRESION_MINIMA) {
            digitalWrite(PIN_VALVULA, LOW); 
            estadoActual = CALCULANDO;
            nuevoPaso = true;
            break; 
          }

          if (presionDC > 140.0)      tiempoAperturaDinamico = 15;
          else if (presionDC > 110.0) tiempoAperturaDinamico = 30;
          else if (presionDC > 80.0)  tiempoAperturaDinamico = 60;
          else if (presionDC > 60.0)  tiempoAperturaDinamico = 120;
          else                        tiempoAperturaDinamico = 200;

          digitalWrite(PIN_VALVULA, LOW); 
          cronometroTransicion = millis();
          estadoActual = ESCALON_VALVULA;
        }
      }
      break;

    case ESCALON_VALVULA:
      if (millis() - cronometroTransicion >= tiempoAperturaDinamico) {
        digitalWrite(PIN_VALVULA, HIGH); 
        tiempoUltimoPaso = millis();
        nuevoPaso = true;
        estadoActual = MIDIENDO; 
      }
      break;

    case CALCULANDO:
      historialAC_Suavizado[MAX_MUESTRAS];
      
      for (i = 0; i < indiceMuestra; i++) {
        if (i < 2 || i > indiceMuestra - 3) {
          historialAC_Suavizado[i] = historialAC[i]; 
        } else {
          historialAC_Suavizado[i] = (historialAC[i-2] + historialAC[i-1] + historialAC[i] + historialAC[i+1] + historialAC[i+2]) / 5.0;
        }
      }

      maxOscilacion = 0.0;
      indicePAM = 0;

      for (i = 0; i < indiceMuestra; i++) {
        if (historialAC_Suavizado[i] > maxOscilacion) {
          maxOscilacion = historialAC_Suavizado[i];
          indicePAM = i;
        }
      }
      
      PAM = historialDC[indicePAM];

      sistole = 0.0;
      umbralSistolico = maxOscilacion * 0.55; 
      for (i = 0; i <= indicePAM; i++) {
        if (historialAC_Suavizado[i] >= umbralSistolico) {
          sistole = historialDC[i] * 1.8;
          break; 
        }
      }

      diastole = 0.0;
      umbralDiastolico = maxOscilacion * 0.75; 
      for (i = indicePAM; i < indiceMuestra; i++) {
        if (historialAC_Suavizado[i] <= umbralDiastolico) {
          diastole = historialDC[i] * 1.6;
          break;
        }
      }
      
      sysGlobal = (int)sistole;
      diaGlobal = (int)diastole;
      
      digitalWrite(PIN_VALVULA, LOW); 
      estadoActual = REPOSO;
      break;
  }
}