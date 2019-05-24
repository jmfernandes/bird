from django.db import models

# Create your models here.
class Feeding(models.Model):
    RFID = models.CharField(max_length=200)
    datetime = models.DateTimeField('date published')
    GPS = models.CharField(max_length=200)
    birdweight = models.FloatField()
    foodweight = models.FloatField()
    temperature = models.FloatField()
    humidity = models.FloatField()
    windspeed = models.FloatField()
    airquality = models.FloatField()
    rain = models.FloatField()
    video = models.CharField(max_length=200)
    picture1 = models.CharField(max_length=200)
    picture2 = models.CharField(max_length=200)
    picture3 = models.CharField(max_length=200)
    picture4 = models.CharField(max_length=200)

    def __str__(self):
        return "RFID - {0} datetime - {1}".format(self.RFID,self.datetime)
