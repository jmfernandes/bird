#!/usr/bin/python
import os
import pygame, sys

from pygame.locals import *
import pygame.camera

from picamera import PiCamera
from time import sleep

width = 640
height = 480

################################################################
#USB Camera 1 
def TakeUSBPicture1():
    #initialise pygame   
    pygame.init()
    pygame.camera.init()
    cam = pygame.camera.Camera("/dev/video0",(width,height))
    cam.start()

    #setup window
    windowSurfaceObj = pygame.display.set_mode((width,height),1,16)
    pygame.display.set_caption('RFIDxxx_RightSideCamera')

    #take a picture
    image = cam.get_image()

    cam.stop()

    #display the picture
    catSurfaceObj = image

    windowSurfaceObj.blit(catSurfaceObj,(0,0))

    pygame.display.update()

    #save picture
    #pygame.image.save(img,'picture.jpg')
    pygame.image.save(windowSurfaceObj,'RFIDxxx_Camera1.jpg')


################################################################
#USB Camera 2 
def TakeUSBPicture2():
    #initialise pygame   
    pygame.init()
    pygame.camera.init()
    cam = pygame.camera.Camera("/dev/video1",(width,height))
    cam.start()


    #setup window
    windowSurfaceObj = pygame.display.set_mode((width,height),1,16)
    pygame.display.set_caption('Camera')

    #take a picture
    image = cam.get_image()

    cam.stop()


    #display the picture
    catSurfaceObj = image

    windowSurfaceObj.blit(catSurfaceObj,(0,0))

    pygame.display.update()

    #save picture
    #pygame.image.save(img,'picture.jpg')
    pygame.image.save(windowSurfaceObj,'RFIDxxx_Camera2.jpg')


################################################################
# Pi Camera 3 

def TakePiPicture():
    #from picamera import PiCamera
    #from time import sleep
    camera = PiCamera()

    camera.resolution = (2592, 1944)
    camera.framerate = 15
    camera.brightness = 50
    camera.contrast = 50
    camera.exposure_mode = 'auto'
    camera.start_preview()
    for i in range(1):
        camera.annotate_text = "Ala'la RFIDXXX_Camera3"
        camera.annotate_text_size = 60
        sleep(2)
        camera.capture('/home/pi/RFIDxxx_Camera3_image%s.jpg' % i)
    camera.stop_preview()


'''
################################################################

#Video Recording Code, 10 Seconds Long
#Vidoe will start after last picture is taken using Fisheye Birdseye Camera

#def TakeVideo()
    #camera.start_preview()
    #camera.start_recording('/home/pi/RFIDxxx_video.h264')
    #sleep(10)
    #camera.stop_recording()
    #camera.stop_preview()
'''