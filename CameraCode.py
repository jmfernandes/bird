#!/usr/bin/python
import os
import pygame, sys
import datetime as dt

from pygame.locals import *
import pygame.camera

from picamera import PiCamera
from time import sleep


width = 640
height = 480

################################################################
#USB Camera 1
def TakeUSBPicture1(cameraPath,RFID,datetime,name):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video0",(width,height))
        cam.start()
        pygame.display.set_caption('{0}'.format(name))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        path = '{0}/{1}/{2}'.format(cameraPath,RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString = '{0}/{1}/{2}/{3}.jpg'.format(cameraPath,RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except Exception as e:
        print("TakeUSBPicture1 error: {0}".format(e))
        return("None")


################################################################
#USB Camera 2
def TakeUSBPicture2(cameraPath, RFID,datetime,name):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video1",(width,height))
        cam.start()
        pygame.display.set_caption('{0}'.format(name))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        path = '{0}/{1}/{2}'.format(cameraPath,RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString = '{0}/{1}/{2}/{3}.jpg'.format(cameraPath,RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except Exception as e:
        print("TakeUSBPicture3 error: {0}".format(e))
        return("None")


################################################################
# Pi Camera 3
def TakePiPicture(cameraPath,RFID,datetime,name):
    try:
        camera = PiCamera()
        camera.annotate_text = "{0}".format(name)
        camera.annotate_text_size = 60
        path = '{0}/{1}/{2}'.format(cameraPath,RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString='{0}/{1}/{2}/{3}.jpg'.format(cameraPath,RFID,datetime,name)
        camera.capture(fileString)
        camera.close()
        return(fileString)
    except Exception as e:
        print("TakePiPicture error: {0}".format(e))
        return("None")
