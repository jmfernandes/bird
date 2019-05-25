#Importatnt, must run "git clone https://github.com/tatobari/hx711py in command window and place the HX711.py file
#in the same folder as this file in order for it to run.  It will not work without the HX711.py file.

import RPi.GPIO as GPIO
import time
import sys

EMULATE_HX711=False

if not EMULATE_HX711:
    import RPi.GPIO as GPIO
    from hx711 import HX711
else:
    from emulated_hx711 import HX711

#Create the exit function ahead of time.
def cleanAndExit():
    print "Cleaning..." #so you know the exit function ran

    if not EMULATE_HX711: #Not sure what this line does
        GPIO.cleanup() #prevents an error occuring when you run GPIO again or in another program
        C = A[0:-1] #Create Array C, that is all the values of A except for the last one
        print C #CHeck that C was created correctly
        B=len(C) #Create List B, that is equal to the length of C
        ave = sum(C)/B #Find the average weight by dividing the sum of C by the length of C (which is B)
        print ave 
        
    print "Bye!"
    sys.exit()

#Compression Loadcell = hxcomp1 & hxcomp2, bar load cells are hxbar1 & hxbar2
hxcomp1= HX711(5, 6)
#hxcomp2= HX711(13, 26)
#hxbar1= HX711(4, 17)
#hxbar2= HX711(27, 22)
# this value is different for every load cell, must be calculated by setting to 1, running the program, placing a known weight
#(continued) on the scale and dividing the reading by the known weight.  I.e., if the reading is 90,000 when 200g is on the scale, the set_reference_unit should be 90,000/200=450
hxcomp1.set_reference_unit(440.3) #222.55892255 
hxcomp1.reset()
hxcomp1.tare()
#hxcomp2.set_reference_unit(1)
#hxcomp2.reset()
#hxcomp2.tare()
print "Tare done! Add weight now..."
A=[]
#val = hxcomp1.get_weight(5)
#        val2 = int(hxcomp2.get_weight(5))
#print val

while True:
    try:
        val= int(hxcomp1.get_weight(5))
        
        if val < 201:
            print val
            time.sleep(0.1)
        if val > 200:
            A.append(val)
            print A


    except (KeyboardInterrupt,SystemExit):
        
         cleanAndExit()
