from gpiozero import Button
import os
import time

btn_arriba = Button(17)
btn_abajo = Button(22)
btn_izq = Button(27)
btn_der = Button(23)
btn_enter = Button(24)

velocidad = 7 
refresco = 0.02 

try:
    while True:

        if btn_arriba.is_active:
            os.system(f"wlrctl pointer move 0 -{velocidad}")
        if btn_abajo.is_active:
            os.system(f"wlrctl pointer move 0 {velocidad}")
        if btn_izq.is_active:
            os.system(f"wlrctl pointer move -{velocidad} 0")
        if btn_der.is_active:
            os.system(f"wlrctl pointer move {velocidad} 0")
            
        if btn_enter.is_active:
            os.system("wlrctl pointer click left")
            time.sleep(0.3) 
            
        time.sleep(refresco)

except KeyboardInterrupt:
    print("\nDriver Apagado.")