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
def TakeUSBPicture1(RFID,timeStamp):
    try:
        #initialise pygame   
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video0",(width,height))
        cam.start()
        pygame.display.set_caption('RFIDxxx_RightSideCamera')
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        fileString = 'RFID{0}_Camera1.jpg'.format(RFID,timeStamp)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
#USB Camera 2 
def TakeUSBPicture2(RFID):
    try:
        #initialise pygame   
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video1",(width,height))
        cam.start()
        pygame.display.set_caption('RFID{}_LeftSideCamera'.format(RFID))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        fileString = 'RFID{}_Camera2.jpg'.format(RFID)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
# Pi Camera 3 
def TakePiPicture(RFID):
    try:
        camera = PiCamera()
        camera.annotate_text = "Ala'la RFID{}_Camera3".format(RFID)
        camera.annotate_text_size = 60
        fileString='/home/pi/RFID{}_Camera3_images.jpg'.format(RFID)
        camera.capture(fileString)
        return(fileString)
    except:
        return("None")