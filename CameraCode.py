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
def TakeUSBPicture1(RFID,number):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video0",(width,height))
        cam.start()
        pygame.display.set_caption('RFID{}_RightSideCamera{number}'.format(RFID,number))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        fileString = 'RFID{0}_RightSideCamera{number}.jpg'.format(RFID,number)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
#USB Camera 2
def TakeUSBPicture2(RFID,number):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        cam = pygame.camera.Camera("/dev/video1",(width,height))
        cam.start()
        pygame.display.set_caption('RFID{0}_LeftSideCamera{1}'.format(RFID,number))
        #take a picture
        image = cam.get_image()
        cam.stop()
        #save picture
        fileString = 'RFID{0}_LeftSideCamera{1}.jpg'.format(RFID,number)
        pygame.image.save(image,fileString)
        return(fileString)
    except:
        return("None")


################################################################
# Pi Camera 3
def TakePiPicture(RFID,number):
    try:
        camera = PiCamera()
        camera.annotate_text = "RFID{0}_OverheadCamera{1}".format(RFID,number)
        camera.annotate_text_size = 60
        fileString='/home/pi/RFID{0}_OverheadCamera{1}.jpg'.format(RFID,number)
        camera.capture(fileString)
        return(fileString)
    except:
        return("None")
