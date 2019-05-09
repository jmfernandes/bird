import sqlite3
import datetime as dt
import time
import random
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
#cursor.execute("CREATE TABLE birds(RFID text, datetime text, GPS text," +
#               "birdWeight real, foodWeight real, temperature real," +
#               "humidity real, windSpeed real, airQuality real, rain real,"+
#               "video text, picture1 text, picture2 text, picture3 text, picture4 text)")

#delete everything
cursor.execute("DELETE FROM birds")

birds = ["1111","2222","3333","4444","5555"]
currentTime=dt.datetime.now()
timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
#use this format and not the other ones when actually building the string
for i in range(10):
    time.sleep(1)
    currentTime=dt.datetime.now()
    timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
    bird = random.choice(birds)
    birdWeight = random.randint(18,23)
    foodWeight = random.randint(1,6)
    temperature = random.randint(72,78)
    humidity = random.randint(0,10)
    windSpeed = random.randint(1,6)
    airQuality = random.randint(95,99)
    rain= random.randint(0,5)
    cursor.execute("INSERT INTO birds VALUES('{0}','{1}','{2}',{3},{4},{5},{6},{7},{8},{9},'{10}','{11}','{12}','{13}','{14}')".format(
                             bird,timeString,"Redondo",birdWeight,foodWeight,temperature,humidity,windSpeed,airQuality,rain,
                             bird+"/vid.mp4",bird+"/pic1.png",bird+"/pic2.png",bird+"/pic3.png",bird+"/pic4.png"))
    


#cursor.execute("INSERT INTO bird VALUES(45,'text')")
#cursor.execute("DROP TABLE bird")
connect.commit()
for row in cursor.execute("SELECT rowid, * FROM birds"):
    print(row)

connect.close()