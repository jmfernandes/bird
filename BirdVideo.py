
#Video Recording Code, 10 Seconds Long
#Vidoe will start after last picture is taken using Fisheye Birdseye Camera

from picamera import PiCamera
from time import sleep

camera = PiCamera()

def TakeVideo():
    camera.start_preview()
    camera.start_recording('/home/pi/RFIDxxx_video.h264')
    sleep(10)
    camera.stop_recording()
    camera.stop_preview()
