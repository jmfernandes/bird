import sqlite3
import requests
"""
table name : birds
table values:
automatically generated rowid
RFID         text
datetime     text
GPS          text
birdWeight   real
hopperWeight   real
consumedWeight real
temperature  real
rain         real
video        text
overhead1     text
overhead2     text
right1        text
right2        text
left1         text
left2         text
"""

def write_to_databases(websiteList):
    connect = sqlite3.connect(r"./DatabaseWork/test.db")
    cursor = connect.cursor()
    
    for websiteData in websiteList:
        # Write to local
        cursor.execute("INSERT INTO birds VALUES('{0}','{1}','{2}',{3},{4},{5},{6},{7},{8},{9},'{10}','{11}','{12}','{13}','{14}')".format(
        websiteData["RFID"],
        websiteData["datetime"],
        websiteData["GPS"],
        websiteData["birdWeight"],
        websiteData["hopperWeight"],
        websiteData["temperature"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"]))

        #write to the website
        myString = "https://alalacrow.herokuapp.com/enter/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}/{11}/{12}/{13}/{14}".format(
        websiteData["RFID"],
        websiteData["datetime"],
        websiteData["GPS"],
        websiteData["birdWeight"],
        websiteData["hopperWeight"],
        websiteData["temperature"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["rain"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"],
        websiteData["filePath"])
        r = requests.get(myString)
    
    connect.commit()
    connect.close()
    
    return None