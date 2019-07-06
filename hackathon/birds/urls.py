from django.urls import path
from . import views

app_name = 'birds'
urlpatterns = [
    # path('', views.HomePageView.as_view(), name='home'),
    path('', views.index, name='index'),
    path('<int:feeding_id>/', views.detail, name='detail'),
    path('submit/', views.submit, name='submit'),
    path('charts/', views.charts, name='charts'),
    path('clear/', views.clear, name='clear'),
    path('<int:feeding_id>/delete/', views.delete, name='delete'),
    path('enter/<str:rfid>/<str:datetime>/<str:gps>/<str:hoppername>/<str:hopperweight>/<str:birdweight>/<feedingduration>/<feedingamount>/<temperature>/<rainamount>/<path:filepath>', views.enter, name='enter'),
    path('retrieve/', views.retrieve, name='retrieve'),
    path('download/', views.download, name='download'),
    path('download/<str:optionalRFID>/', views.download, name='download'),
]
