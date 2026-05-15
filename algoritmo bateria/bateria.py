import smbus2
import struct

def leer_bateria():
    try:
        bus = smbus2.SMBus(1)
        address = 0x36 
        read = bus.read_word_data(address, 0x04)
        swapped = struct.unpack("<H", struct.pack(">H", read))[0]
        capacidad = swapped / 256.0
        print(round(capacidad, 1))
    except Exception as e:
        print(0)
        
leer_bateria()