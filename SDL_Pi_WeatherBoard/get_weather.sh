#!/bin/sh
# clear the csv file
cp /dev/null /home/pi/bird/weather.csv
# run these commands
sudo i2cdetect -y 1
sudo i2cdetect -y 1
# run the weather board
sudo python /home/pi/bird/SDL_Pi_WeatherBoard/WeatherBoard.py