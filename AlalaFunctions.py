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
        takePicture(id)
        useServo()
    finally:
        GPIO.cleanup()
        
#Create the exit function ahead of time.
def cleanAndExit(A):
    print "Cleaning..." #so you know the exit function ran
    GPIO.cleanup() #prevents an error occuring when you run GPIO again or in another program
    X = A[0:-1] #Create Array C, that is all the values of A except for the last one
    B=len(X) #Create List B, that is equal to the length of C
    Y=[200]
    dt=0.2 # 5 hz is how fast we are taking time, every 0.2 seconds
    freq=0.5 #frequency of transients we want to reject (ones faster than .5 hz or 2 second)
    omega=freq*2*3.1415 #omega for low pass filter
    if B > 2:
        for n in range(1,B):
            yi=(Y[n-1]+X[n]*dt*omega)/(1+omega*dt)
            Y.append(yi)
        print (Y[B-1])
        birdWeight = sum(X)/B #Find the average weight by dividing the sum of C by the length of C (which is B)
        print birdWeight 
        #print Y    
    print "Bye!"
    sys.exit()
    