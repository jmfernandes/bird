import sqlite3
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


connect = sqlite3.connect(r"test.db")
cursor = connect.cursor()

for row in cursor.execute("SELECT rowid, * FROM birds"):
    print(row)

connect.close()
