from django.shortcuts import get_object_or_404, render
from django.http import HttpResponse, HttpResponseRedirect, Http404
from django.template import loader
from django.forms.models import model_to_dict
from django.urls import reverse
from django.utils import timezone
import csv

from .models import Feeding
# Functions
def enterValue(rfid,datetime,gps,hoppername,hopperweight,birdweight,feedingduration,feedingamount,temperature,rainamount,filepath):
    q = Feeding(RFID=rfid,datetime=datetime,GPS=gps,hoppername=hoppername,hopperweight=hopperweight,
    birdweight=birdweight,feedingduration=feedingduration,feedingamount=feedingamount,temperature=temperature,rainamount=rainamount,filepath=filepath)
    q.save()
# Create your views here.
def index(request):
    latest_feeding_list = Feeding.objects.order_by('-datetime')
    context = {
        'latest_feeding_list' : latest_feeding_list
    }
    return render(request, 'birds/index.html', context)

def clear(request):
    latest_feeding_list = Feeding.objects.order_by('-datetime')
    context = {
        'latest_feeding_list' : latest_feeding_list
    }
    return render(request, 'birds/clear.html', context)

def submit(request):
    if request.method == 'GET':
        return render(request, 'birds/enter.html',{'headernames': [f.name for f in Feeding._meta.get_fields() if f.name != "id"]})
    else:
        try:
            RFID = request.POST['RFID']
            datetime = request.POST['datetime']
            GPS = request.POST['GPS']
            hoppername = request.POST['hoppername']
            filepath = request.POST['filepath']
            hopperweight = request.POST['hopperweight']
            birdweight = request.POST['birdweight']
            feedingduration = request.POST['feedingduration']
            feedingamount = request.POST['feedingamount']
            temperature = request.POST['temperature']
            rainamount = request.POST['rainamount']
            if (not RFID):
                raise KeyError
            if (not datetime):
                raise KeyError
            if (not GPS):
                raise KeyError
            if (not hoppername):
                raise KeyError
            if (not hopperweight):
                raise KeyError
            if (not birdweight):
                raise KeyError
            if (not feedingduration):
                raise KeyError
            if (not feedingamount):
                raise KeyError
            if (not temperature):
                raise KeyError
            if (not rainamount):
                raise KeyError
            if (not filepath):
                filepath = "None"
            enterValue(RFID,datetime,GPS,hoppername,hopperweight,birdweight,feedingduration,feedingamount,temperature,rainamount,filepath)
            return HttpResponseRedirect(reverse('birds:index'))
        except (KeyError):
            return render(request, 'birds/enter.html',{'headernames': [f.name for f in Feeding._meta.get_fields() if f.name != "id"],'error_message': "Make Sure all fields are filled out. Filepath is optional."})

def detail(request, feeding_id):
    feeding = model_to_dict(get_object_or_404(Feeding, pk=feeding_id))
    return render(request, 'birds/detail.html', {'feeding': feeding})

def delete(request, feeding_id):
    try:
        feeding = Feeding.objects.get(pk=feeding_id)
    except Feeding.DoesNotExist:
        raise Http404("Feeding ID does not exist")
    feeding.delete()
    return HttpResponseRedirect(reverse('birds:clear'))

def enter(rfid,datetime,gps,hoppername,hopperweight,birdweight,feedingduration,feedingamount,temperature,rainamount,filepath):
    enterValue(rfid,datetime,gps,hoppername,hopperweight,birdweight,feedingduration,feedingamount,temperature,rainamount,filepath)
    return HttpResponse('')

def retrieve(request):
    myList = Feeding.objects.values_list('RFID', flat=True).order_by('RFID').distinct()
    return render(request, 'birds/retrieve.html',{'myList':myList})

def download(request,optionalRFID = None):
    # try to get post response
    try:
        optionalRFID = request.POST['combo']
    except (KeyError):
        pass
    # set up the csv file.
    response = HttpResponse(content_type='text/csv')
    writer = csv.writer(response)
    writer.writerow([f.name for f in Feeding._meta.get_fields()])
    # determine if all of the data or just partial data will be written.
    if (optionalRFID or optionalRFID == 'All Data'):
        response['Content-Disposition'] = 'attachment; filename="{}_Feedings.csv"'.format(optionalRFID)
        valueList = Feeding.objects.values_list().filter(RFID=optionalRFID).order_by('-datetime')
    else:
        response['Content-Disposition'] = 'attachment; filename="AllFeedings.csv"'
        valueList = Feeding.objects.values_list().order_by('-datetime')
    # write the data
    for row in valueList:
        writer.writerow(row)
    # return the response
    return response
