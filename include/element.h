#if !defined(ELEMENT_H)
#define ELEMENT_H

#include "randomstuff.h"
#include <stdlib.h>
#include <vector>
#include <set>
#include <fstream>
#include <string>
#include <iostream>
class robot;//fwd declaration

typedef int mobileGoalIndex;
typedef int coneIndex;
typedef int robotIndex;

struct fence {
	/*const*/ float fieldSizeIn = 141.05;// 140.5 + 2 * (1.27);wall thickness accounted for
	float depthIn = 1.27;//thickness of the vex fence
	void wallPush(robot *r);
	float inFromEnd = 13.9198;
	struct zone {
		std::set<mobileGoalIndex> tenPoint;
		std::set<mobileGoalIndex> fivePoint;
		std::set<mobileGoalIndex> twentyPoint;
	};
	zone z[2]; // red and blue
	void clearZones();
	float poleEquation(float xPoint, float yPoint, float slope, float value);//red and bleu
	void robotPole(robot *r);//collision between robot and zone poles
	bool isSkills = false;
};
struct element {
	element(vec3 initpos, float initradius, float initheight) : pos(initpos), radius(initradius), height(initheight) {}
	vec3 pos;
	float radius;//size of the object;
	/*const*/ float height;
	void fencePush(fence *f);
	void robotColl(int index, robot *r, int type, fence *f, int roboIndex);
	void collision(element *e);
	void collideWith(robot *robit, vec3 closestPoint, int type, int index, int roboIndex);
	std::set<coneIndex> stacked; // for goals only, cones stacked on it.
	std::set<robotIndex> inPossession;//if being held or whatnot

	static ci::gl::Texture MobileGoal;
	static ci::gl::Texture coneTexture;
};
struct cone : public element {
	cone(vec3 pos) : element(pos, cRad, cHeight), fellOn(0), landed(false) {}
	int fellOn;
	bool landed;
	bool targetted = false;//if being targetted by an autobot
	int grabbingRobotIndex = -1;//-1 if no robot grabbing
	void coneGrab(robot *robit, int index, int robIndex);
};
struct MoGo : public element {
	MoGo(vec3 pos, int initColour) : element(pos, MGRad, mgHeight), colour(initColour) {}
	int colour;//0 is yellow, 1 is red, 2 is blue 
	void mogoGrab(robot *robit, int index);
	void zoneScore(fence *f, int index);
};
struct stago : public element {
	stago(vec3 pos, float initRad, float initHeight) : element(pos, initRad, initHeight) {}
};

#endif