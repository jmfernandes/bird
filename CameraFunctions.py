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

###############################################################
def TakeVideo(cameraPath,RFID,datetime,name):
    camera = None
    try: 
        camera = PiCamera()
        camera.start_preview()
        path = '{0}/{1}/{2}'.format(cameraPath,RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString='{0}/{1}/{2}/{3}.h264'.format(cameraPath,RFID,datetime,name)
        camera.start_recording(fileString)
        sleep(12)
        camera.stop_recording()
        camera.stop_preview()
        camera.close()
        return(fileString)
    except Exception as e:
        if camera:
            camera.stop_recording()
            camera.stop_preview()
            camera.close()
        print("TakeVideo error: {0}".format(e))
        return("None")

################################################################
#USB Camera 1
    
def TakeUSBPicture1(cameraPath,RFID,datetime,name,videoName):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        if not os.path.exists("/dev/{0}".format(videoName)):
            raise Exception("/dev/{0} does not exist".format(videoName))
        cam = pygame.camera.Camera("/dev/{0}".format(videoName),(width,height))
        cam.start()
        #pygame.display.set_caption('{0}'.format(name))
        #take a picture
        cam.get_image()
        sleep(2)
        cam.query_image()
        image = cam.get_image()
        cam.stop()
        cam = None
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
def TakeUSBPicture2(cameraPath,RFID,datetime,name,videoName):
    try:
        #initialise pygame
        pygame.init()
        pygame.camera.init()
        if not os.path.exists("/dev/{0}".format(videoName)):
            raise Exception("/dev/{0} does not exist".format(videoName))
        cam = pygame.camera.Camera("/dev/{0}".format(videoName),(width,height))
        cam.start()
        #pygame.display.set_caption('{0}'.format(name))
        #take a picture
        sleep(2)
        cam.query_image()
        image = cam.get_image()
        cam.stop()
        cam = None
        #save picture
        path = '{0}/{1}/{2}'.format(cameraPath,RFID,datetime)
        if not os.path.exists(path):
            os.makedirs(path)
        fileString = '{0}/{1}/{2}/{3}.jpg'.format(cameraPath,RFID,datetime,name)
        pygame.image.save(image,fileString)
        return(fileString)
    except Exception as e:
        print("TakeUSBPicture2 error: {0}".format(e))
        return("None")


################################################################
# Pi Camera 3
def TakePiPicture(cameraPath,RFID,datetime,name):
    camera = None
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
        if camera:
            camera.close()
        print("TakePiPicture error: {0}".format(e))
        return("None")
    
#TakeUSBPicture1('/home/pi/',"test","test","right","video0") #video0          
#TakeUSBPicture2('/home/pi/',"test","test","left","video2") #video1         

#TakePiPicture('/home/pi',"test","test","pi")