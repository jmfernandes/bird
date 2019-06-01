import sqlite3
import requests
"""
table name : birds
table values:
automatically generated rowid
RFID            text
datetime        text
GPS             text
hoppername      text
hopperweight    real
birdweight      real
feedingduration real
feedingamount   real
temperature     real
rainamount      real
filepath        text
"""

def write_to_databases(websiteList):
    connect = sqlite3.connect(r"./DatabaseWork/test.db")
    cursor = connect.cursor()

    for websiteData in websiteList:
        # Write to local
        cursor.execute('INSERT INTO birds VALUES("{0}","{1}","{2}","{3}",{4},{5},{6},{7},{8},{9},"{10}")'.format(
        websiteData["RFID"],
        websiteData["datetime"],
        websiteData["GPS"],
        websiteData["hopperName"],
        websiteData["hopperWeight"],
        websiteData["birdWeight"],
        websiteData["feedingDuration"],
        websiteData["feedingAmount"],
        websiteData["temperature"],
        websiteData["rainAmount"],
        websiteData["filePath"]))

        #write to the website
        myString = "https://alalacrow.herokuapp.com/enter/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/{9}/{10}".format(
        websiteData["RFID"],
        websiteData["datetime"],
        websiteData["GPS"],
        websiteData["hopperName"],
        websiteData["hopperWeight"],
        websiteData["birdWeight"],
        websiteData["feedingDuration"],
        websiteData["feedingAmount"],
        websiteData["temperature"],
        websiteData["rainAmount"],
        websiteData["filePath"])
        r = requests.get(myString)

    connect.commit()
    connect.close()

    return None
