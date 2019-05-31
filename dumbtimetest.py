import time
import PyPi
from simpleRFID import checkRFID
import multiprocessing.pool

t_end = time.time() + 1

while time.time() < t_end:
    checkRFID(id)
    if id > 0:
        print('its no longer time')
        
print ('time''s up')