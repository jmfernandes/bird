import RPi.GPIO as GPIO
from mfrc522 import SimpleMFRC522
import time

reader = SimpleMFRC522()


def checkRFID(id,text):
    t_end=time.time() + 3
    while time.time() < t_end:
        id,text = reader.read()
        print t_end
        print('it ended')
    print id
    
    


##try:
##    id, text = reader.read()
##    print(id)
##    print(text)
##finally:
##    GPIO.cleanup()