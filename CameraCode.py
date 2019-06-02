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
        fileString = '/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
#USB Camera 2
def TakeUSBPicture2(RFID,datetime,name):
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
        fileString = '/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
# Pi Camera 3
def TakePiPicture(RFID,datetime,name):
    try:
        camera = PiCamera()
        camera.annotate_text = "{0}".format(name)
        camera.annotate_text_size = 60
        fileString='/home/bird/media/{0}/{1}/{2}.jpg'.format(RFID,datetime,name)
        camera.capture(fileString)
        return(fileString)
    except:
        return("None")
