#Used to store all custom defined functions for Alala Carte Diner
from picamera import PiCamera
from time import sleep
import serial
import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import os
import time
import sys
from hx711 import HX711

def checkRFID():
    #https://pimylifeup.com/raspberry-pi-rfid-rc522/
    reader = SimpleMFRC522()
    try:
        id, text = reader.read()
        if id:
            takePicture(id)
            useServo()
    finally:
        GPIO.cleanup()
        
#Create the exit function ahead of time.
def cleanAndExit():
    print("Cleaning...") #so you know the exit function ran
    GPIO.cleanup() #prevents an error occuring when you run GPIO again or in another program
    print("Bye!")
    sys.exit()
    
def lowPassFilter(weightdata):
    X = weightdata[0:-1] #Create Array C, that is all the values of A except for the last one because the last value is zero sometimes
    B=len(X) #Create List B, that is equal to the length of X
    filterweight=[200] #Seed our y function with an estimate of the bird weight (200 was used because of calibration weight)
    dt=0.1 # 10 hz is how fast we are taking time, every 0.1 seconds
    freq=1 #frequency of transients we want to reject (ones faster than 1 hz or 1 second)
    omega=freq*2*3.1415 #omega for low pass filter, converting to radians
    if B > 2:
        for n in range(1,B):
            yi=(filterweight[n-1]+X[n]*dt*omega)/(1+omega*dt)
            filterweight.append(yi)
        print(filterweight[B-1])
        return filterweight[B-1]
    
def averageWeight(weightdata1):
    X = weightdata1[0:-1] #Create Array C, that is all the values of A except for the last one because the last value is zero sometimes
    B=len(X) #Create List B, that is equal to the length of X
    if B > 2:
        weight = sum(X)/B
        return weight
