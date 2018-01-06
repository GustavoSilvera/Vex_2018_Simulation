#if !defined(FIELD_H)
#define FIELD_H

#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

#include "vec3.h"
#include "robot.h"
#include "element.h"
#include "randomstuff.h"

#include <stdlib.h>
#include <vector>
#include <set>
#include <fstream>
#include <string>
#include <iostream>
//declares the class for the robot and all the data that goes with it.
//LOOKIE HERE: http://vexcompetition.es/wp-content/uploads/2017/04/IntheZone-Field-specifications.pdf
class field {
public:
	field(std::vector<robot> *r);
	static ci::gl::Texture fieldBare;

	//std::set<mobileGoalIndex> pushMoGo;
	//std::set<coneIndex> pushCones;

	void FieldUpdate(std::vector<robot> *r);
	void initialize(std::vector<robot> *r);
	bool isInit; // suggestion: construct instead of initialize
	fence f;
	
	std::vector<cone> c;
	std::vector<MoGo> mg;
	std::vector<stago> pl;//poles in the field

	void physics(int index, element *e, robot *r, int type, int roboIndex);
	void fallingOn(cone *fall, robot *r, int index);
	void positionFall(cone *fall);
	int calculateScore();

	bool initialized;//if the field bare texture is visible or not. 
	bool fieldInit;
	//vec3 Rpos;
};

#endif