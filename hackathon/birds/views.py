from django.shortcuts import get_object_or_404, render
from django.http import HttpResponse
from django.template import loader

from .models import Feeding
# Create your views here.
def index(request):
    latest_feeding_list = Feeding.objects.order_by('-datetime')[:5]
    template = loader.get_template('birds/index.html')
    context = {
        'latest_feeding_list' : latest_feeding_list
    }
    output = ', '.join([f.RFID for f in latest_feeding_list])
    return render(request, 'birds/index.html', context)

def detail(request, feeding_id):
    feeding = get_object_or_404(Feeding, pk=feeding_id)
    return render(request, 'birds/detail.html', {'feeding': feeding})
    #return HttpResponse("You're looking at feeding %s." % feeding_id)
