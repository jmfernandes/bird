from picamera import PiCamera
from time import sleep
import serial
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
#Import Alala-specific functions
from AlalaFunctions import cleanAndExit
from AlalaFunctions import checkRFID
from servo1 import actuateServo
from simplerRFID import checkRFID
from CameraCode import TakeUSBPicture1
from CameraCode import TakeUSBPicture2
from CameraCode import TakePiPicture
from BirdVideo import TakeVideo
import os
import time
import sys
from hx711 import HX711
#Important, must run "git clone https://github.com/tatobari/hx711py in command window and place the HX711.py file
#in the same folder as this file in order for it to run.  It will not work without the HX711.py file.

import RPi.GPIO as GPIO
import time
import sys

#Compression Loadcell = hxcomp1 & hxcomp2, bar load cells are hxbar1 & hxbar2
hxcomp1= HX711(5, 6)
#hxcomp2= HX711(13, 26)
#hxbar1= HX711(4, 17)
#hxbar2= HX711(27, 22)
# this value is different for every load cell, must be calculated by setting to 1, running the program, placing a known weight
#(continued) on the scale and dividing the reading by the known weight.  I.e., if the reading is 90,000 when 200g is on the scale, the set_reference_unit should be 90,000/200=450
hxcomp1.set_reference_unit(440.3) #222.55892255 
hxcomp1.reset()
hxcomp1.tare()
#hxcomp2.set_reference_unit(1)
#hxcomp2.reset()
#hxcomp2.tare()
print "Tare done! Add weight now..."
A=[]
id=[]
#val = hxcomp1.get_weight(5)
#        val2 = int(hxcomp2.get_weight(5))
#print val

while True:
    try:
        val= int(hxcomp1.get_weight(5))
        
        if val < 101:
            print val
            time.sleep(0.1)
        if val > 100:
            checkRFID(id)
            if id > 0:
                actuateServo(100)
                print ('loopstart')
                for n in range(1,30):
                    val = hxcomp1.get_weight(1)
                    A.append(val)
                    sleep(.1)
                print ('loopend')
                actuateServo(20)
                sys.exit()

    except (KeyboardInterrupt,SystemExit):
        cleanAndExit(A)









