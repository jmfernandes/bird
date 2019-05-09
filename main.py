from picamera import PiCamera
from time import sleep
import serial
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import os
import time
import sys
from hx711 import HX711
import wiringpi

def cleanAndExit():
    print("Cleaning...")
    GPIO.cleanup()
    print("Bye!")
    sys.exit()

def takePicture(rfid):
    RFID = rfid

    camera = PiCamera()

    #Code for single Camera taking single image
    camera.rotation = 0
    camera.resolution = (1000,1000)#2592, 1944)
    camera.framerate = 15 
    camera.brightness = 50
    camera.exposure_mode = 'auto'
    camera.start_preview()
    camera.annotate_text = "Ala'la RFID:{} Camera 3 Back View".format(RFID)
    camera.annotate_text_size = 50
    sleep(5) 
    camera.capture('/home/pi/Desktop/{}_1.jpg'.format(rfid))
    camera.stop_preview()
    camera.close()

    os.system("fswebcam /home/pi/Desktop/{}_2.jpg".format(rfid))
    #https://www.raspberrypi.org/documentation/usage/webcams/

    #Code for multiple frames with single camera, update range(#)
    """camera.start_preview()
    for i in range(5):
        sleep(5)
        camera.capture('/home/pi/Desktop/image%s.jpg' % i)
    camera.stop_preview()

    #Code for recording video, to change length modify sleep(#) 
    camera.start_preview()
    camera.start_recording('/home/pi/video.h264')
    sleep(10)
    camera.stop_recording()
    camera.stop_preview()
    """
"""
def checkRFID():
    ser = serial.Serial('/dev/ttyACM0',9600)
    s = [0]
    read_serial=ser.readline()
    RFID = read_serial
    RFID = RFID[:12]
    takePicture(RFID.decode("utf-8"))
"""

def useServo():
     #use 'GPIO naming'
     wiringpi.wiringPiSetupGpio()
 
     # set #18 to be a PWM output
     wiringpi.pinMode(18, wiringpi.GPIO.PWM_OUTPUT)
 
     # set the PWM mode to milliseconds stype
     wiringpi.pwmSetMode(wiringpi.GPIO.PWM_MODE_MS)
 
     # divide down clock
     wiringpi.pwmSetClock(192)
     wiringpi.pwmSetRange(2000)
 
     delay_period = 0.01
 
     while True:
        for pulse in range(50, 250, 1):
                wiringpi.pwmWrite(18, pulse)
                time.sleep(delay_period)
                print('works')
        for pulse in range(250, 50, -1):
                wiringpi.pwmWrite(18, pulse)
                time.sleep(delay_period)

def checkRFID():
    #https://pimylifeup.com/raspberry-pi-rfid-rc522/
    reader = SimpleMFRC522()
    try:
        id, text = reader.read()
        takePicture(id)
        useServo()
    finally:
        GPIO.cleanup()
"""hx = HX711(12,6)
hx.set_reading_format("MSB","MSB")
hx.set_reference_unit(113)
hx.reset()
hx.tare()
print("Tare done! Add weight now...")
"""

while True:
    """try:
        val = hx.get_weight(5)
        print(val)
        hx.power_down()
        hx.power_up()
        time.sleep(0.1)
    ###if weight changes, checkRFID()
    except (KeyboardInterrupt,SystemExit):
        cleanAndExit()"""
    checkRFID()



