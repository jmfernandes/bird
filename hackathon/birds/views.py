from django.shortcuts import get_object_or_404, render
from django.http import HttpResponse, HttpResponseRedirect, Http404
from django.template import loader
from django.forms.models import model_to_dict
from django.urls import reverse
from django.utils import timezone

from .models import Feeding
# Create your views here.
def index(request):
    latest_feeding_list = Feeding.objects.order_by('-datetime')
    template = loader.get_template('birds/index.html')
    context = {
        'latest_feeding_list' : latest_feeding_list
    }
    return render(request, 'birds/index.html', context)

def vote(request):
    if request.method == 'GET':
        return render(request, 'birds/enter.html')
    else:
        try:
            RFID = request.POST['RFID']
            datetime = request.POST['datetime']
            GPS = request.POST['GPS']
            birdweight = request.POST['birdweight']
            foodweight = request.POST['foodweight']
            temperature = request.POST['temperature']
            humidity = request.POST['humidity']
            windspeed = request.POST['windspeed']
            airquality = request.POST['airquality']
            rain = request.POST['rain']
            video = request.POST['video']
            pic1 = request.POST['pic1']
            pic2 = request.POST['pic2']
            pic3 = request.POST['pic3']
            pic4 = request.POST['pic4']
            if (not RFID):
                raise KeyError
            if (not GPS):
                raise KeyError
            if (not datetime):
                raise KeyError
            if (not birdweight):
                raise KeyError
            if (not foodweight):
                raise KeyError
            if (not temperature):
                raise KeyError
            if (not humidity):
                raise KeyError
            if (not windspeed):
                raise KeyError
            if (not airquality):
                raise KeyError
            if (not rain):
                raise KeyError
            if (not video):
                video = " "
            if (not pic1):
                pic1 = " "
            if (not pic2):
                pic2 = " "
            if (not pic3):
                pic3 = " "
            if (not pic4):
                pic4 = " "
            return HttpResponseRedirect(reverse('birds:enter', args=(RFID,datetime,GPS,
            birdweight,foodweight,temperature,humidity,windspeed,airquality,rain,video,pic1,pic2,pic3,pic4)))
        except (KeyError):
            return render(request, 'birds/enter.html',{'error_message': "Make Sure all fields are filled out. Video and Picture paths are optional."})

def detail(request, feeding_id):
    feeding = model_to_dict(get_object_or_404(Feeding, pk=feeding_id))
    return render(request, 'birds/detail.html', {'feeding': feeding})

def delete(request, feeding_id):
    try:
        feeding = Feeding.objects.get(pk=feeding_id)
    except Feeding.DoesNotExist:
        raise Http404("Feeding ID does not exist")
    feeding.delete()
    return HttpResponseRedirect(reverse('birds:index'))

def enter(request,rfid,datetime,gps,birdweight,foodweight,temperature,humidity,windspeed,airquality,rain,video,pic1,pic2,pic3,pic4):
    q = Feeding(RFID=rfid,datetime=datetime,GPS=gps,birdweight=birdweight,foodweight=foodweight,temperature=temperature,
    humidity=humidity,windspeed=windspeed,airquality=airquality,rain=rain,video=video,picture1=pic1,picture2=pic2,picture3=pic3,picture4=pic4)
    q.save()
    return HttpResponseRedirect(reverse('birds:index'))
