from django.db import models

# Create your models here.
class Feeding(models.Model):
    RFID = models.CharField(max_length=200)
    datetime = models.DateTimeField('date published')
    GPS = models.CharField(max_length=200)
    hoppername = models.CharField(max_length=200)
    hopperweight = models.FloatField()
    birdweight = models.FloatField()
    feedingduration = models.FloatField()
    feedingamount = models.FloatField()
    temperature = models.FloatField()
    rainamount = models.FloatField()
    filepath = models.CharField(max_length=200)

    def __str__(self):
        return "RFID: {0} - datetime: {1} - ".format(self.RFID,self.datetime)
