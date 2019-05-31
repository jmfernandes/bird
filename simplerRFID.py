import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time

reader = SimpleMFRC522()


def checkRFID(id,text):
    id,text = reader.read()
    
    


##try:
##    id, text = reader.read()
##    print(id)
##    print(text)
##finally:
##    GPIO.cleanup()