from django.urls import path
from . import views

app_name = 'birds'
urlpatterns = [
    path('', views.index, name='index'),
    path('<int:feeding_id>/', views.detail, name='detail'),
    path('new/', views.vote, name='vote'),
    path('<int:feeding_id>/delete/', views.delete, name='delete'),
    path('enter/<str:rfid>/<str:datetime>/<str:gps>/<birdweight>/<foodweight>/<temperature>/<humidity>/<windspeed>/<airquality>/<rain>/<str:video>/<str:pic1>/<str:pic2>/<str:pic3>/<str:pic4>', views.enter, name='enter'),
]
