import serial

ser = serial.Serial('/dev/ttyACM0',9600)
s = [0]
while True:
    read_serial=ser.readline()
    print(read_serial)
