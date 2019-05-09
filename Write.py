#!/usr/bin/env python

import RPi.GPIO as GPIO 
from mfrc522 import SimpleMRFC522

reader = SimpleMRFC522()

try:
    text = input('New data:')
    print("Now place your tag to write")
    reader.wirite(text)
    print("Written")
    
finally:
    GPIO.cleanup()