import sqlite3
import requests
import dropbox
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

def upload_images_to_dropbox(websiteList):
    accessCode = 'pPgmIezcgqAAAAAAAAAAC9j36rWwsmEMbmqzghkGS7tamdM29obAqzux2bh-C2Tw'
    dbx = dropbox.Dropbox(accessCode)
    for websiteData in websiteList:
        if (websiteData['RightSideCamera1'] != "None"):
            f = open(websiteData['RightSideCamera1'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/RightSideCamera1.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

        if (websiteData['RightSideCamera2'] != "None"):
            f = open(websiteData['RightSideCamera2'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/RightSideCamera2.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

        if (websiteData['LeftSideCamera1'] != "None"):
            f = open(websiteData['LeftSideCamera1'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/LeftSideCamera1.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

        if (websiteData['LeftSideCamera2'] != "None"):
            f = open(websiteData['LeftSideCamera2'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/LeftSideCamera2.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

        if (websiteData['OverheadCamera1'] != "None"):
            f = open(websiteData['OverheadCamera1'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/OverheadCamera1.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

        if (websiteData['OverheadCamera2'] != "None"):
            f = open(websiteData['OverheadCamera2'])
            fdata = f.read()
            dbx.files_upload(fdata, '/media/{0}/{1}/OverheadCamera2.jpg'.format(websiteData['RFID'],websiteData['datetime']))
            f.close()

def upload_data_to_database(websiteList):
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

    connect.commit()
    connect.close()

def upload_data_to_website(websiteList):
    for websiteData in websiteList:
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
