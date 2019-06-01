from picamera import PiCamera
from time import sleep
import serial
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
#Import Alala-specific functions
from AlalaFunctions import cleanAndExit
from AlalaFunctions import checkRFID
from AlalaFunctions import lowPassFilter
from servo1 import actuateServo
from CameraCode import TakeUSBPicture1,TakeUSBPicture2,TakePiPicture
from BirdVideo import TakeVideo
import os
import time
import random
import datetime as dt
import sys
from WriteAllToWebsite import write_to_databases
from hx711 import HX711
#Important, must run "git clone https://github.com/tatobari/hx711py in command window and place the HX711.py file
#in the same folder as this file in order for it to run.  It will not work without the HX711.py file.

import RPi.GPIO as GPIO
import time
import sys

dataList = []

dataDict = {}
dataDict['RFID'] = random.choice(["steve", "bob", "josh"])
dataDict['datetime'] = dt.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
dataDict['GPS'] = random.choice(["hawaii","Hilo Hawaii", "Kona Hawaii"])
dataDict['temperature'] = random.randint(65,100)
dataDict['hopperWeight'] = random.randint(10,1000)
dataDict['birdWeight'] = random.randint(350,750)
dataDict['consumedDuration'] = random.randint(10,1000)
dataDict['consumedWeight'] = -1
dataDict['rain'] = random.randint(0,2)
dataDict['filePath'] = "None"

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
print("Tare done! Add weight now...")
A=[]
id=None
#val = hxcomp1.get_weight(5)
#        val2 = int(hxcomp2.get_weight(5))
#print val

reader = SimpleMFRC522()

while True:
    try:
        val= int(hxcomp1.get_weight(5))
        currentTime=dt.datetime.now()
        timeString = currentTime.strftime('%Y-%m-%d %H:%M:%S')
        if val < 101:
            print(val)
            time.sleep(0.1)
        if val > 100:
            id,text = reader.read()
            if id:
                dataDict['RFID']=id
                actuateServo(100)
                print ('loopstart')
                for n in range(1,30):
                    val = hxcomp1.get_weight(1)
                    A.append(val)
                    sleep(.1)
                print ('loopend')
                dataDict['datetime']= timeString
                dataDict['birdWeight']=lowPassFilter(A)
                dataDict['filePath'] = TakeUSBPicture1(id,timeString)
                actuateServo(20)
                #DO LOTS OF STUFF
                #call the averaging function to determine when bird leaves and then continue
                dataDict['consumedDuration'] = (dt.datetime.now()-currentTime).total_seconds()
                print(dataDict)
                #APPEND TO LIST
                dataList.append(dataDict)
                sys.exit()

    except (KeyboardInterrupt,SystemExit):
        #write_to_databases(dataList)
        cleanAndExit()









