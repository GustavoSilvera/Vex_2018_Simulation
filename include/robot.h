#if !defined(ROBOT_H)
#define ROBOT_H

#include <iostream>
#include <fstream>
#include <sstream>
//own stuff
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "element.h"
#include "randomstuff.h"
#include "vec3.h"

class field;//fwd declaration

//declares the class for the robot and all the data that goes with it. 
class robot{
public:
	robot();
	static float truSpeed(int degree, float value);
	void driveFwds(float power);//driving only fwds (+) and bkwds(-)
	void rotate(float power);//rotating CW (+) and CCW (-)
	void moveAround();//controls entire robot movement
	void stopAll();//stops robot instantly
	void updatePhysicalFeatures();//keeps initial proportions with size changes
	void updateVertices();//calculates the vertices for robot base and mogo (squares)
	void update(field *f, int rob);
	void readScript();//script parser
	bool directlyInPath(bool vertical, int range, vec3 pos);//directly in vert/hor path
	bool readyToReRun = false;//if starting rerun execution
	//getters and setters
	void setPos(vec3 pos);
	float getSize();
	///void collision(robot *r);
	///vec3 closestPoint;
	///int closestVertice = 0;
	enum action  {
		ACTION_ROTATE,
		ACTION_FWDS,
		ACTION_MOGO
	};
	struct command {
		enum action a;
		double amnt;
	};
	std::vector<command> commands;
	std::vector<vec3> pathPoints;
	struct debug {
		//lines
		float slope;//only need 1 slope, both sides of a square are parallel
		float Yint[2];
		vec3 vertices[4];
		//output text
		float distance;//simulating a real worl vex encoder shaft
		float rotDist;//simulating a real world vex gyro 
		//mogo square
		float MGYint[2];
		vec3 MGVert[4];
		//angle simulation
	};
	debug db;
	float size = 18;//important size 
					//claw
	struct intake {
		int goal = 0;
		float size;
		float baseSize;//base of the claw
		float position;
		float thickness;
		float length;
		float speed;
		float protrusion;//useful for mogo coming out
		bool grabbing = false;
		int holding = -1;//index for which element is being grabbed, to draw it above all the rest. 
		void claw(float robSize);
		void mogo(float robSize);
		bool liftUp, liftDown;
		float liftSpeed;
		float liftPos;
		float maxHeight = 50;//50 inches max height
	};
	intake c;
	intake mg;/*mogo intake is not rly a claw tho*/
	struct physics {
		float mRot = 0;//drawing rotation
		vec3 position;//X, Y, and Z
		vec3 velocity;//diff x, diff y, diff z
		vec3 acceleration = vec3(0, 0, 0);
		float rotVel = 0;
		float maxVel = 0;
		float maxRotVel = 0;
		float rotAcceleration = 0;
		void speedMult(float base, float rot);
		float amountOfFriction = 5;	//constant changing the amount of friction for the robot
	};
	physics p;
	struct details {
		float motorSpeed = (float)MAXSPEED;
		int basePower;
		int rotPower;
		int initCommandsSize = 1;
	};
	details d;
	struct controls {
		bool KeyUp = false;//if the arrow keys are triggering a forwards/backwards motion
		bool KeyDown = false;//if the arrow keys are triggering a forwards/backwards motion
		bool KeyRight = false;//used for whether the keys are supposed to be triggering a rotation
		bool KeyLeft = false;//used for whether the keys are supposed to be triggering a rotation
	};
	controls ctrl;
	//other stuffs
	static ci::gl::Texture TankBase;
	static ci::gl::Texture TankBase2;
	static ci::gl::Texture CChanel;
	static ci::gl::Texture CChanelVERT;
//};
//class autobot : public robot {//self driving robots
//public:
	bool thinking = false;
	void decisions(field *f, int rob);
	bool grabMoGo = false;//ideally want to get mogo first
	bool reRouting = false;
	int elementGoalIndex = 0;//which element to grab

	void checkReRunScript();

	//for autobots
	void findGoal(element *e);
	void goGrab(field *f, element *e, int coneIndex, int roboIndex);
	void stackOn(field *f, element *e);
	void placeIn(field *f, fence::zone *z);//weird rn
	void reRoute(field *f, element *e, int dir);

};
#endif  // ROBOT_H
