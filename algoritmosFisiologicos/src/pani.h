#ifndef PANI_H
#define PANI_H

#include <Arduino.h>
#include "HX711.h"

void setupPANI();
void procesarPANI();

void iniciarPANI_Medicion();
void detenerPANI_Emergencia();

#endif