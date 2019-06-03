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
        print(e)

    #close SQL connection
    connect.close()

        