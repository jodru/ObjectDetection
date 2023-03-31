#include <opencv4/opencv2/opencv.hpp>
#include <iostream>
#include <chrono>

using namespace std::chrono;
using namespace cv;
using namespace std;

//genMask function; returns a Mat of the mask given a frame
Mat genMask(Mat frame){

	//declare variables needed
	Mat hsv, lower, upper, redMask, masked;

	//convert bgr frame to hsv frame
	cvtColor( frame, hsv, COLOR_BGR2HSV) ;

	//the following 2 are python code. the two lines after should work.
    //lower_mask = cv.inRange(hsv, lower1, upper1)
    //upper_mask = cv.inRange(hsv, lower2, upper2)

    inRange(hsv, Scalar(0, 70, 50), Scalar(10, 255, 255), lower);
    inRange(hsv, Scalar(170, 70, 50), Scalar(180, 255, 255), upper);

    //combine masks into one mask, then perform a bitwise_and on the frame
    redMask = lower | upper;
    bitwise_and(hsv,hsv, masked, redMask);

    return masked;

}

//structure to store results of getPos
struct result {
	double xAvg;
	double yAvg;
	int calculate; //if this is 0 it is false if it is 1 it is true
};

//getPos function; returns a structure
auto getPos(Mat frame) {

	//declare the result structure as calcs
    result calcs;

    //set values
	int calculate = 0;
	int height = frame.rows; // height
	int width = frame.cols; // width
	double xAvg = 0;
	double yAvg = 0;
	int count = 0;
	
	//go through all pixels, if pixel is not black then add it to the average
	for (int y = 0; y < height; y++){
		for (int x = 0; x < width; x++){
			if (frame.at<char>(y,x) != 0){ //input[frame.step * x + y]
				xAvg = xAvg + x;
				yAvg = yAvg + y;
				count = count+1;
			}
		}
	}
	
	//If red pixel count is above x amount
	if (count > 1200){ 
		xAvg = (xAvg/count);// * 2;
		yAvg = (yAvg/count);// * 2;
		calculate = 1;
	}

	//store the values gotten into calcs and return it
	calcs.xAvg = xAvg;
	calcs.yAvg = yAvg;
	calcs.calculate = calculate;

    return calcs;
}


//main function
int main(int argc, char* argv[])
{
	//open up the video capture, if it can't then just end the function
	VideoCapture cap(0);
	if (cap.isOpened() == false){
		cout << "Cannot open camera." << endl;
		cin.get();
		return -1;
	}
	
	
	//use 640x480 for all frames captured
	double width = 640;
	double height = 480;

	//name the window that displays the frame
	string windowName = "cam";
	namedWindow(windowName);


	//while loop for image processing
	while (true){

		auto start = high_resolution_clock::now();
		//declare variables and structures
		Mat frame, mask, vBin;
		result calcs;

		//break if frame cannot be read
		bool bSuccess = cap.read(frame);
		if (bSuccess == false){
			cout << "Can't read frame, disconnecting." << endl;
			cin.get();
			break;
		}

		//resize and declare a finalContour for use later
		resize(frame, frame, Size(width, height));
		Mat finalContour(frame.rows, frame.cols, CV_8UC1, Scalar::all(0));
		
		//generate a mask of all red pixels of frame
		mask = genMask(frame);
		

		//split the mask into hue, saturation, value, and keep value.
		Mat channels[3];
		split(mask, channels);
		Mat v = channels[2];
		
		//changes value to vBin which is a binary image of the red pixels detected
		threshold(v, vBin, 127, 255, 0);
		
		
		/*Find all contours, get biggest contour, print only that contour
		* This helps identify the object that needs to be tracked without taking
		* other objects into account
		*/
		vector<vector<Point> > contours;
        //vector<Vec4i> hierarchy;
		findContours(vBin.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		double bigArea = 0;
		int bigContour;
		for (int i = 0; i < contours.size(); i++){
			double area = contourArea(contours[i]);
			if (area > bigArea){
				bigArea = area;
				bigContour = i;
			}
		}
		
		if (bigContour > -1){
			drawContours(finalContour, contours, bigContour, Scalar(255), FILLED);
		}
		else{
			continue;
		}
		

		//rescale to be smaller if needed. Be sure to update getPos as well if used.
		//resize(v, rsMask, Size(width/2, height/2));

		//get position of the center of object, if it exists
		calcs = getPos(finalContour);

		//if it calculated enough reds, run.
		if (calcs.calculate == 1){

			//draw circle at center. unnecessary outside of testing.
			circle(frame, Point(calcs.xAvg, calcs.yAvg), 10, Scalar(255,0,0), 5);

			//declare these strings now
			string textx, texty;

			//the following is my thoughts on how to do this from python
			/*
			* Center of a screen with 640 and 480 is roughly 320,240
			* So let's make a "rectangle of center" that counts as the center.
			* As long as the average center is in that rectangle, 
			* it's for all intents and purposes in the center
			* Let's then define a larger rectangle that is also "in" the center.
			* If color pixels start popping outside of this bigger rectangle, 
			* the robot is close enough
			* That should account for safety
			* Let's say small square is 40x40 and big square is 200x200 to start
			*/
			//addendum: I have no idea how fast this thing will go and I think the main
			//thing is to have it track an object, so I'm leaving the "too close" clause
			//out for now.

			//idk what this is for but I have a feeling if I delete it something will break
			string text = "";

			//Center camera/head on object (needs ros strings to print to console)
			
			if (calcs.xAvg > 340){
				//look right until it's in center rect
				textx = "Object is to right";
			}
			else if (calcs.xAvg < 300){
				//look left until it's in center rect
				textx = "Object is to left";
			}
			else{
				textx = "Object is in center on x axis";
			}

			if (calcs.yAvg > 260){
				//look down until it's in center rect
				texty = "Object is down";
			}
			else if (calcs.yAvg < 220){
				//look up until it's in center rect
				texty = "Object is up";
			}
			else{
				texty = "Object is in center on y axis";
			}

			//more unnecessary code for final, but print location with textx and texty
			putText(frame, textx, Point(0, 430), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,255,0), 2);
			putText(frame, texty, Point(0, 460), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,255,0), 2);

		}
		
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(stop - start);
		
		cout << "Time taken: " << duration.count() << endl;
		
		imshow(windowName, frame);

		//hit esc for kill command
		if (waitKey(10) == 27)
		{
			cout << "Esc key was pressed, vid stopping..." << endl;
			break;
		}
	}
	return 0;
}

/*
* run commands
* apt install libopencv-dev
* g++ -o cappin cappin.cpp -I/usr/include/opencv4 -lopencv_core -lopencv_videoio -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs
* ./cappin
*/
