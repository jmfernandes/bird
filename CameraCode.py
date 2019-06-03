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
def TakeUSBPicture1(RFID,datetime,name):
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
        path = '/home/pi/bird2/media/{0}/{1}'.format(RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString = '/home/pi/bird2/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        pygame.image.save(image,fileString)
        print('done')
        return(fileString)
    except Exception as e:
        print(e)
        return("None")


################################################################
#USB Camera 2
def TakeUSBPicture2(RFID,datetime,name):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video2",(width,height))
        cam.start()
        pygame.display.set_caption('{0}'.format(name))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        path = '/home/pi/bird2/media/{0}/{1}'.format(RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString = '/home/pi/bird2/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except Exception as e:
        print(e)
        return("None")


################################################################
# Pi Camera 3
def TakePiPicture(RFID,datetime,name):
    try:
        camera = PiCamera()
        camera.annotate_text = "{0}".format(name)
        camera.annotate_text_size = 60
        path = '/home/pi/bird2/media/{0}/{1}'.format(RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString='/home/pi/bird2/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        camera.capture(fileString)
        return(fileString)
    except Exception as e:
        print(e)
        return("None")
