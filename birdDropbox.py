import dropbox

#f = open('helloalala.txt')
def save_photo(rfid,timestamp):
    f = open('../RFID{}_Camera1.jpg'.format(rfid))
    fdata = f.read()
    dbx = dropbox.Dropbox('pPgmIezcgqAAAAAAAAAAC9j36rWwsmEMbmqzghkGS7tamdM29obAqzux2bh-C2Tw')

    dbx.files_upload(fdata, '/media/{0}/{1}/Camera1.jpg'.format(rfid,timestamp))
    print(dbx.createSharedLinkWithSettings('/media/{0}/{1}/Camera1.jpg'.format(rfid,timestamp)))
    f.close()


save_photo('matt','2018-11-01 94:45:15')