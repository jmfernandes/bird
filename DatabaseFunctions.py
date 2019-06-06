import sqlite3
import os
import datetime as dt
import time
import csv
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
def generate_rows():
    connect = sqlite3.connect(r"test.db")
    cursor = connect.cursor()
    #cursor.execute("drop table birds")
    #cursor.execute("CREATE TABLE birds(RFID text, datetime text UNIQUE, GPS text," +
    #               "hopperName text, hopperWeight real, birdWeight real," +
    #               "feedingDuration real, feedingAmount real, temperature real, rainAmount real,"+
    #               "filepath text)")

    #delete everything
    cursor.execute("DELETE FROM birds")

    # birds = ["1111","2222","3333","4444","5555"]
    # currentTime=dt.datetime.now()
    # timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
    #use this format and not the other ones when actually building the string
    # for i in range(10):
    #     time.sleep(1)
    #     currentTime=dt.datetime.now()
    #     timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
    #     bird = random.choice(birds)
    #     birdWeight = random.randint(18,23)
    #     foodWeight = random.randint(1,6)
    #     temperature = random.randint(72,78)
    #     humidity = random.randint(0,10)
    #     windSpeed = random.randint(1,6)
    #     airQuality = random.randint(95,99)
    #     rain= random.randint(0,5)
    #     cursor.execute("INSERT INTO birds VALUES('{0}','{1}','{2}',{3},{4},{5},{6},{7},{8},{9},'{10}','{11}','{12}','{13}','{14}')".format(
    #                              bird,timeString,"Redondo",birdWeight,foodWeight,temperature,humidity,windSpeed,airQuality,rain,
    #                              bird+"/vid.mp4",bird+"/pic1.png",bird+"/pic2.png",bird+"/pic3.png",bird+"/pic4.png"))



    #cursor.execute("INSERT INTO bird VALUES(45,'text')")
    #cursor.execute("DROP TABLE bird")
    connect.commit()
    # for row in cursor.execute("SELECT rowid, * FROM birds"):
    #     print(row)

    connect.close()

def view_rows():
    connect = sqlite3.connect(r"test.db")
    cursor = connect.cursor()

    for row in cursor.execute("SELECT rowid, * FROM birds"):
        print(row)

    connect.close()
    

def convert_database_to_csv(path,databaseLocation):
    # Raise exception if it does not exist
    if not os.path.exists(path):
        raise Exception('Filepath does not exist')

    #connect to the database
    connect = sqlite3.connect(databaseLocation)
    cursor = connect.cursor()
    #put everything in a try block so if something fails, the connection is still closed.
    try:
        #create header row
        headerRow = [description[0] for description in cursor.execute("SELECT rowid, * FROM birds LIMIT 1").description]
        #create a list of unique RFID values
        ListRFID = []
        for item in cursor.execute("SELECT DISTINCT RFID FROM birds").fetchall():
            ListRFID.append(item[0])
        #write to the different files
        for name in ListRFID:
            with open("{0}{1}.csv".format(path,name), 'w+') as file:
                writer = csv.writer(file)
                writer.writerow(headerRow)
                for row in cursor.execute("SELECT rowid, * FROM birds WHERE RFID = '{}'".format(name)):
                    writer.writerow(row)
    except Exception as e:
        print("Error in convert_database_to_csv: {}".format(e))

    #close SQL connection
    connect.close()

def write_to_website():
    connect = sqlite3.connect(r"test.db")
    cursor = connect.cursor()

    birds = ["josh","erica","krystal","meena"]
    currentTime=dt.datetime.now()
    # timeString = currentTime.strftime('%d/%m/%y %I:%M %S %p')
    #use this format and not the other ones when actually building the string
    timeString = currentTime.strftime('%Y-%m-%d %H:%M:%S')
    bird = random.choice(birds)
    birdWeight = random.randint(300,400)
    foodWeight = random.randint(10,30)
    temperature = random.randint(72,90)
    feedingDuration = random.randint(10,25)
    feedingAmount = random.randint(5,20)
    airQuality = random.randint(95,99)
    rain= random.randint(0,15)
    cursor.execute("INSERT INTO birds VALUES('{0}','{1}','{2}','{3}',{4},{5},{6},{7},{8},{9},'{10}')".format(
                              bird,
                              timeString,
                              "Los Angeles",
                              "the hopper",
                              foodWeight,
                              birdWeight,
                              feedingDuration,
                              feedingAmount,
                              temperature,
                              rain,
                              "None"))

    
    connect.commit()
    connect.close()

    exit()
    #write to the website
    mystring = "https://alalacrow.herokuapp.com/enter/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}/{11}/{12}/{13}/{14}".format(
    bird,timeString,"Los Angeles",birdWeight,foodWeight,temperature,humidity,windSpeed,airQuality,rain,bird+"vid.mp4",
    bird+"pic1.png",bird+"pic2.png",bird+"pic3.png",bird+"pic4.png")

    r = requests.get(mystring)
    print(len(r.content))
    
        