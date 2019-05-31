import sqlite3
import datetime as dt
import time
import csv
import random
import urllib.request
import requests
"""
table name : birds

table values:

automatically generated rowid
RFID         text
datetime     text
GPS          text
birdWeight   real
foodWeight   real
temperature  real
humidity     real
windSpeed    real
airQuality   real
rain         real
video        text
picture1     text
picture2     text
picture3     text
picture4     text
"""


connect = sqlite3.connect(r"test.db")
cursor = connect.cursor()

birds = ["1111","2222","3333","4444","5555"]
currentTime=dt.datetime.now()
# timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
#use this format and not the other ones when actually building the string
timeString = currentTime.strftime('%Y-%m-%d %H:%M:%S')
bird = random.choice(birds)
birdWeight = random.randint(18,23)
foodWeight = random.randint(1,6)
temperature = random.randint(72,90)
humidity = random.randint(0,10)
windSpeed = random.randint(1,6)
airQuality = random.randint(95,99)
rain= random.randint(0,15)
cursor.execute("INSERT INTO birds VALUES('{0}','{1}','{2}',{3},{4},{5},{6},{7},{8},{9},'{10}','{11}','{12}','{13}','{14}')".format(
                              bird,timeString,"Los Angeles",birdWeight,foodWeight,temperature,humidity,windSpeed,airQuality,rain,
                              bird+"/vid.mp4",bird+"/pic1.png",bird+"/pic2.png",bird+"/pic3.png",bird+"/pic4.png"))


connect.commit()
connect.close()

#write to the website
mystring = "https://alalacrow.herokuapp.com/enter/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}/{11}/{12}/{13}/{14}".format(
bird,timeString,"Los Angeles",birdWeight,foodWeight,temperature,humidity,windSpeed,airQuality,rain,bird+"vid.mp4",
bird+"pic1.png",bird+"pic2.png",bird+"pic3.png",bird+"pic4.png")

r = requests.get(mystring)
print(len(r.content))
