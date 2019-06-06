#=================================== Headers ===================================#
# Import python functions
import os
import time
import random
import datetime as dt
import sys
import serial
import RPi.GPIO as GPIO
# Import third-party functions
from mfrc522 import SimpleMFRC522
from hx711 import HX711
# Import local funcions
from DatabaseFunctions import convert_database_to_csv
from AlalaFunctions import cleanAndExit, checkRFID, lowPassFilter, actuateServo
from CameraFunctions import TakeUSBPicture1,TakeUSBPicture2,TakePiPicture, TakeVideo
from UploadFunctions import upload_data_to_database, upload_images_to_dropbox, upload_data_to_website
#===============================================================================#

#======================= Set up data list and dictionary =======================#
dataList = []
dataDict = {}
dataDict['RFID'] = random.choice(["steve", "bob", "josh"])
dataDict['datetime'] = dt.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
dataDict['GPS'] = random.choice(["hawaii","Hilo Hawaii", "Kona Hawaii"])
dataDict['hopperName'] = "Ala'la Carte Diner"
dataDict['hopperWeight'] = random.randint(10,1000)
dataDict['birdWeight'] = random.randint(350,750)
dataDict['feedingDuration'] = random.randint(10,1000)
dataDict['feedingAmount'] = random.randint(1,20)
dataDict['temperature'] = random.randint(65,100)
dataDict['rainAmount'] = random.randint(0,2) 
dataDict['filePath'] = "None"
dataDict['video'] = "None"
dataDict['RightSideCamera1'] = "None"
dataDict['RightSideCamera2'] = "None"
dataDict['LeftSideCamera1'] = "None"
dataDict['LeftSideCamera2'] = "None"
dataDict['OverheadCamera1'] = "None"
dataDict['OverheadCamera2'] = "None"
#===============================================================================#

#============================ Initialize Parameters ============================#

#Compression Loadcell = hxcomp1 & hxcomp2, bar load cells are hxbar1 & hxbar2
hxcomp1= HX711(17, 27)
hxcomp2= HX711(16, 20)
hxcomp1.set_reference_unit(440.3) #222.55892255
hxcomp1.reset()
hxcomp1.tare()
hxcomp2.set_reference_unit(1)
hxcomp2.reset()
hxcomp2.tare()
print("Tare done! Add weight now...")


birdWeightList = []
hopperWeightList = []
id = None
reader = SimpleMFRC522()
running = True
baseDir = os.path.dirname(os.path.realpath(__file__))
cameraPath = "{0}/media".format(baseDir)
databasePath = "{0}/test.db".format(baseDir)
usbPath = "/media/pi/1842-ED03/csv/"

#===============================================================================#

#=============================== Set up GPIO Pins ==============================#
GPIO.setmode(GPIO.BCM)
ledPin = 21 
buttonPin = 7 
servoPin = 24 #which pin is the servo connected to (besides power and gnd)
GPIO.setup(buttonPin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(ledPin,GPIO.OUT, initial=0)
GPIO.setup(servoPin, GPIO.OUT) #servo pin
p1 = GPIO.PWM(servoPin, 50) #sets 50hz frequency for servoPin

#===============================================================================#

def my_callback(channel):
    GPIO.output(21,1)
    print('Callback')
    try:
        upload_data_to_database(dataList)
        convert_database_to_csv(usbPath,databasePath)
        time.sleep(1)
    except:
        print('Error writing data to USB stick')
        for i in range(3):
            time.sleep(0.25)
            GPIO.output(21,0)
            time.sleep(0.25)
            GPIO.output(21,1)
        
    GPIO.output(21,0)

GPIO.add_event_detect(7, GPIO.RISING, callback=my_callback, bouncetime=300)


while running:
    try:
        val = int(hxcomp1.get_weight(5))
        currentTime = dt.datetime.now()
        timeString = currentTime.strftime('%Y-%m-%d %H:%M:%S')
        if (currentTime.hour >= 21):
            print("it's too late to feed the birds")
            if not os.path.exists(usbPath):
                sys.exit()
        if (val < 0):
            print("value of {} - improper tare".format(val))
            time.sleep(0.1)
        elif (val <= 100):
            print("value of {}".format(val))
            time.sleep(0.1)
        else:
            id,text = reader.read()
            id = "josh"
            if id:
                # Get initial data
                dataDict['RFID']= id
                dataDict['datetime']= timeString
                dataDict['filePath'] = 'https://www.dropbox.com/home/media/{}/{}'.format(id,timeString)
                # Open hood
                actuateServo(p1,50)
                # Take pictures
                dataDict['RightSideCamera1'] = TakeUSBPicture1(cameraPath,id,timeString,"RightSideCamera1") #video0          
                dataDict['LeftSideCamera1'] = TakeUSBPicture2(cameraPath,id,timeString,"LeftSideCamera1") #video1              
                dataDict['OverheadCamera1'] = TakePiPicture(cameraPath,id,timeString,"OverheadCamera1")
                # Get the weight of the bird
                for n in range(1,30):
                    val = hxcomp1.get_weight(1)
                    birdWeightList.append(val)
                    time.sleep(.1)
                dataDict['birdWeight']=lowPassFilter(birdWeightList)
                birdWeightList.clear()
                # Take video
                dataDict['video'] = TakeVideo(cameraPath,id,timeString,"video")
                # Get the hopper weight
                for n in range(1,30):
                    val = hxcomp2.get_weight(1)
                    hopperWeightList.append(val)
                    time.sleep(.1)
                dataDict['hopperWeight']=lowPassFilter(hopperWeightList)
                hopperWeightList.clear()
                # Take second round of photos
                dataDict['RightSideCamera2'] = TakeUSBPicture1(cameraPath,id,timeString,"RightSideCamera2") #video0
                dataDict['LeftSideCamera2'] = TakeUSBPicture2(cameraPath,id,timeString,"LeftSideCamera2") #video1
                dataDict['OverheadCamera2'] = TakePiPicture(cameraPath,id,timeString,"OverheadCamera2")
                print(dataDict)
                # Append to List
                dataList.append(dataDict)
                # Close the lid
                while id:
                    id,text = reader.read()
                actuateServo(p1,-45)
                # Get feeding duration
                dataDict['feedingDuration'] = (dt.datetime.now()-currentTime).total_seconds()
                sys.exit()

    except (KeyboardInterrupt,SystemExit):
        #upload_images_to_dropbox(dataList)
        #upload_data_to_database(dataList)
        #upload_data_to_website(dataList)
        #cleanAndExit()
        GPIO.cleanup()
        running = False

print("program finished")
