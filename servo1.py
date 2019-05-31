# Servo Control
from time import sleep
import RPi.GPIO as GPIO
 
servoPin = 18 #which pin is the servo connected to (besides power and gnd)
GPIO.setmode(GPIO.BCM)
GPIO.setup(servoPin, GPIO.OUT)

#servoPin2 = XX #For second servo
p1 = GPIO.PWM(servoPin, 50) #sets 50hz frequency for servoPin

def actuateServo(angle):
#    GPIO.output(servoPin,True)
    p1.start(7.5) #7.5% duty cycle means 1.5ms out of 20ms means 90 deg or neutral for servo
    duty = 2.5 + angle / 18 #2.5 = 0 deg, 12.5 = 180
    p1.ChangeDutyCycle(duty)
    sleep(1)
    p1.ChangeDutyCycle(0)
#    GPIO.output(servoPin,False)
    print('servo actuated')

