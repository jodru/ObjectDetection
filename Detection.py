'''yabba dabba do'''

import numpy as np
import cv2 as cv
import time

#make mask
def genMask(frame):
      
    hsv = cv.cvtColor(frame, cv.COLOR_BGR2HSV)
    # lower boundary RED color range values; Hue (0 - 10)
    lower1 = np.array([0, 100, 20])
    upper1 = np.array([10, 255, 255])
 
    # upper boundary RED color range values; Hue (160 - 180)
    lower2 = np.array([160,50,50])
    upper2 = np.array([179,255,255])

    lower_mask = cv.inRange(hsv, lower1, upper1)
    upper_mask = cv.inRange(hsv, lower2, upper2)
 
    redMask = lower_mask + upper_mask;
    mask = cv.bitwise_and(frame,frame, mask=redMask)
    
    return mask

#get position
def getPos(frame):
    
    start = time.process_time()
        
    calculate = False
    height = frame.shape[0] # height
    width = frame.shape[1] # width
    xAvg = 0
    yAvg = 0
    count = 0
    
    #go through all pixels, if pixel is not black then add it to the average
    for y in range(0, height):
        for x in range(0, width):
            if (frame[y,x] != 0):
                xAvg += x
                yAvg += y
                count = count+1
                
    print(time.process_time() - start , "getPos")
    
    #get average positions to determine center of object           
    if count > 750: #If red pixel count is above 1000
        xAvg = (xAvg/count) * 4
        yAvg = (yAvg/count) * 4
        #above two are multiplied by 4 because pi is terrible at detecting shit
        #when the resolution is big so I have the mask scaled by a factor of 4
        calculate = True
        #print(xAvg, "," , yAvg , count)
    
    return (xAvg, yAvg, calculate)

#resize the fram
def rescale_frame(frame, percent):
    width = int(frame.shape[1] * percent/ 100)
    height = int(frame.shape[0] * percent/ 100)
    dim = (width, height)
    return cv.resize(frame, dim, interpolation =cv.INTER_AREA)

class Main():
    cap = cv.VideoCapture(0)
    if not cap.isOpened():
        print("Cannot open camera")
        exit()
    x = 1
    
    while x == 1:
        # Capture frame-by-frame
        ret, frame = cap.read()
        # if frame is read correctly ret is True
        if not ret:
            print("Can't receive frame (stream end?). Exiting ...")
            break
    
        mask = genMask(frame)
        
        h, s, v = cv.split(mask)
        #Scales down image to speed up algorithm
        #rn set to 25%
        v = rescale_frame(v, 25)
        
        xAvg, yAvg, calculate = getPos(v)
    
        if calculate != False:
            
            cv.circle(frame, (int(xAvg), int(yAvg)), 10, (255,0,0), 5)
        
        
            '''
            Center of a screen with 640 and 480 is roughly 320,240
            So let's make a "rectangle of center" that counts as the center.
            As long as the average center is in that rectangle, it's for all intents and purposes in the center
            
            Let's then define a larger rectangle that is also "in" the center.
            If color pixels start popping outside of this bigger rectangle, the robot is close enough
            That should account for safety
            
            Let's say small square is 40x40 and big square is 200x200 to start
            '''
            text = ""
            #Center camera/head on object
            
            if (xAvg > 340):
                #look right until it's in center rect
                textx = "Object is to right"
            elif (xAvg < 300):
                #look left until it's in center rect
                textx = "Object is to left"
            else:
                textx = "Object is in center on x axis"
            
            if (yAvg > 260):
                #look down until it's in center rect
                texty = "Object is down"
            elif (yAvg < 220):
                #look up until it's in center rect
                texty = "Object is up"
            else:
                texty = "Object is in center on y axis"
            
                
            #turn body so that it and the head are facing object, then take a step forward
            
            cv.putText(frame, textx, (0, 430), cv.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2)
            cv.putText(frame, texty, (0, 460), cv.FONT_HERSHEY_SIMPLEX, 1, (0,255,0), 2)
            
            
        
        # Display the resulting frame
        cv.imshow('frame', frame)
        if cv.waitKey(1) == ord('q'):
            break
    
    # When everything done, release the capture
    cap.release()
    cv.destroyAllWindows()
    
