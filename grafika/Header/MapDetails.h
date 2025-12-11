#ifndef MAPDETAILS_H
#define MAPDETAILS_H


#include <vector>
#include <GLFW/glfw3.h>



enum AppMode {
	MODE_WALKING,
	MODE_MEASURING
};


struct MeasurePoint {
	float x;
	float y;
};



extern std::vector<MeasurePoint> measurePoints;
extern float measureDistance;
extern double totalDistance;
extern const double mapScaleFactor;


extern AppMode mode;


extern float camX;
extern float camY;
extern float camSpeed;
extern const float zoomVal;


extern int screenWidth;
extern int screenHeight;



void processKeyboardInput(GLFWwindow* window);
void processMouse(GLFWwindow* window);
int findPointIndexNear(double mx, double my);
void addMeasurePoints(MeasurePoint p);
void removeMeasurePoint(int index);
MeasurePoint mouseToXY(double mx, double my);


#endif 




