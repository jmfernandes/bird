import sys
import time
import traceback

from Phidget22.Devices.RFID import *
from Phidget22.PhidgetException import *
from Phidget22.Phidget import *
from Phidget22.Net import *

try:
    from PhidgetHelperFunctions import *
except ImportError:
    sys.stderr.write("\nCould not find PhidgetHelperFunctions. Either add PhdiegtHelperFunctions.py to your project folder "
                      "or remove the import from your project.")
    sys.stderr.write("\nPress ENTER to end program.")
    readin = sys.stdin.readline()
    sys.exit()

"""
* Displays info about the attached Phidget channel.
* Fired when a Phidget channel with onAttachHandler registered attaches
*
* @param self The Phidget channel that fired the attach event
"""
def onAttachHandler(self):
    
    ph = self
    try:
        #If you are unsure how to use more than one Phidget channel with this event, we recommend going to
        #www.phidgets.com/docs/Using_Multiple_Phidgets for information
        
        print("\nAttach Event:")
        
        """
        * Get device information and display it.
        """
        channelClassName = ph.getChannelClassName()
        serialNumber = ph.getDeviceSerialNumber()
        channel = ph.getChannel()
        if(ph.getDeviceClass() == DeviceClass.PHIDCLASS_VINT):
            hubPort = ph.getHubPort()
            print("\n\t-> Channel Class: " + channelClassName + "\n\t-> Serial Number: " + str(serialNumber) +
                "\n\t-> Hub Port: " + str(hubPort) + "\n\t-> Channel:  " + str(channel) + "\n")
        else:
            print("\n\t-> Channel Class: " + channelClassName + "\n\t-> Serial Number: " + str(serialNumber) +
                    "\n\t-> Channel:  " + str(channel) + "\n")
        
    except PhidgetException as e:
        print("\nError in Attach Event:")
        DisplayError(e)
        traceback.print_exc()
        return

"""
* Displays info about the detached Phidget channel.
* Fired when a Phidget channel with onDetachHandler registered detaches
*
* @param self The Phidget channel that fired the attach event
"""
def onDetachHandler(self):

    ph = self
    try:
        #If you are unsure how to use more than one Phidget channel with this event, we recommend going to
        #www.phidgets.com/docs/Using_Multiple_Phidgets for information
    
        print("\nDetach Event:")
        
        """
        * Get device information and display it.
        """
        serialNumber = ph.getDeviceSerialNumber()
        channelClass = ph.getChannelClassName()
        channel = ph.getChannel()
        
        deviceClass = ph.getDeviceClass()
        if (deviceClass != DeviceClass.PHIDCLASS_VINT):
            print("\n\t-> Channel Class: " + channelClass + "\n\t-> Serial Number: " + str(serialNumber) +
                  "\n\t-> Channel:  " + str(channel) + "\n")
        else:            
            hubPort = ph.getHubPort()
            print("\n\t-> Channel Class: " + channelClass + "\n\t-> Serial Number: " + str(serialNumber) +
                  "\n\t-> Hub Port: " + str(hubPort) + "\n\t-> Channel:  " + str(channel) + "\n")
        
    except PhidgetException as e:
        print("\nError in Detach Event:")
        DisplayError(e)
        traceback.print_exc()
        return


"""
* Writes Phidget error info to stderr.
* Fired when a Phidget channel with onErrorHandler registered encounters an error in the library
*
* @param self The Phidget channel that fired the attach event
* @param errorCode the code associated with the error of enum type ph.ErrorEventCode
* @param errorString string containing the description of the error fired
"""
def onErrorHandler(self, errorCode, errorString):

    sys.stderr.write("[Phidget Error Event] -> " + errorString + " (" + str(errorCode) + ")\n")

"""
* Outputs the RFID's most recently reported tag.
* Fired when a RFID channel with onTagHandler sees a tag
*
* @param self The RFID channel that fired the Tag event
* @param tag The reported tag from the RFID channel
"""
def onTagHandler(self, tag, protocol):

    #If you are unsure how to use more than one Phidget channel with this event, we recommend going to
    #www.phidgets.com/docs/Using_Multiple_Phidgets for information

    print("\n[Tag Event] -> Tag     : " + str(tag))
    print("            -> Protocol: " + RFIDProtocol.getName(protocol))

"""
* Outputs the RFID's most recently lost tag.
* Fired when a RFID channel with onTagLostHandler loses a tag
*
* @param self The RFID channel that fired the Tag event
* @param tag The reported tag from the RFID channel
"""
def onTagLostHandler(self, tag, protocol):

    #If you are unsure how to use more than one Phidget channel with this event, we recommend going to
    #www.phidgets.com/docs/Using_Multiple_Phidgets for information
    
    print("\n[Tag Lost Event] -> Tag     : " + str(tag))
    print("                 -> Protocol: " + RFIDProtocol.getName(protocol))

"""
* Prints descriptions of how events related to this class work
"""
def PrintEventDescriptions():

    print("\n--------------------\n"
        "\n  | Tag events will call their associated function every time new tag data is received from the device.\n")

    print(
        "\n  | Tag lost events will call their associated function every time a tag is no longer seen by device.\n"
        "  | Press ENTER once you have read this message.\n")
        
    #readin = sys.stdin.readline(1)
    
    print("\n--------------------")
   
"""
* Creates, configures, and opens a RFID channel.
* Displays Tag events for 10 seconds
* Closes out RFID channel
*
* @return 0 if the program exits successfully, 1 if it exits with errors.
"""
def main():
    try:
        """
        * Allocate a new Phidget Channel object
        """
        print("first")
        ch = RFID()
        print("here")
        
        """
        * Set matching parameters to specify which channel to open
        """
        
        #You may remove this line and hard-code the addressing parameters to fit your application
        #channelInfo = AskForDeviceParameters(ch)
        #564577
        
        ch.setDeviceSerialNumber(564577)
        ch.setHubPort(-1)
        ch.setIsHubPortDevice(0)
        ch.setChannel(0)   
        
        #if(channelInfo.netInfo.isRemote):
        #    ch.setIsRemote(channelInfo.netInfo.isRemote)
        #    if(channelInfo.netInfo.serverDiscovery):
        #        try:
        #            Net.enableServerDiscovery(PhidgetServerType.PHIDGETSERVER_DEVICEREMOTE)
        #        except PhidgetException as e:
        #            PrintEnableServerDiscoveryErrorMessage(e)
        #            raise EndProgramSignal("Program Terminated: EnableServerDiscovery Failed")
        #    else:
        #        Net.addServer("Server", channelInfo.netInfo.hostname,
        #            channelInfo.netInfo.port, channelInfo.netInfo.password, 0)
        
        """
        * Add event handlers before calling open so that no events are missed.
        """
        print("\n--------------------------------------")
        print("\nSetting OnAttachHandler...")
        ch.setOnAttachHandler(onAttachHandler)
        
        print("Setting OnDetachHandler...")
        ch.setOnDetachHandler(onDetachHandler)
        
        print("Setting OnErrorHandler...")
        ch.setOnErrorHandler(onErrorHandler)
        
        #This call may be harmlessly removed
        PrintEventDescriptions()
        
        print("\nSetting OnTagHandler...")
        ch.setOnTagHandler(onTagHandler)
        
        print("\nSetting OnTagLostHandler...")
        ch.setOnTagLostHandler(onTagLostHandler)
        
        """
        * Open the channel with a timeout
        """
        
        print("\nOpening and Waiting for Attachment...")
        
        try:
            ch.openWaitForAttachment(5000)
        except PhidgetException as e:
            PrintOpenErrorMessage(e, ch)
            raise EndProgramSignal("Program Terminated: Open Failed")
        
        print("Sampling data for 30 seconds...")
        
        print("You can do stuff with your Phidgets here and/or in the event handlers.")
        
        time.sleep(180)
        
        """
        * Perform clean up and exit
        """        
        print("\nDone Sampling...")

        print("Cleaning up...")
        ch.close()
        print("\nExiting...")
        return 0

    except PhidgetException as e:
        sys.stderr.write("\nExiting with error(s)...")
        DisplayError(e)
        traceback.print_exc()
        print("Cleaning up...")
        ch.close()
        return 1
    except EndProgramSignal as e:
        print(e)
        print("Cleaning up...")
        ch.close()
        return 1
    except RuntimeError as e:
         sys.stderr.write("Runtime Error: \n\t" + e)
         traceback.print_exc()
         return 1
    finally:
        print("Press ENTER to end program.")
        readin = sys.stdin.readline()

main()
