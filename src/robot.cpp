#include "vec3.h"
#include "robot.h"
#include "field.h"
#include "randomstuff.h"

//declares and defines the robot class and functions
ci::gl::Texture robot::TankBase;
ci::gl::Texture robot::TankBase2;
ci::gl::Texture robot::CChanel;
ci::gl::Texture robot::CChanelVERT;

robot::robot() {
	c.liftPos = 0;
	c.protrusion = 0;//not protruding from baseout
	c.liftUp = false;
	c.speed = 0.5;
	c.liftDown = false;
	mg.protrusion = 0;
	mg.liftPos = 0;
	db.distance = RESET;
	db.rotDist = RESET;
	p.position = vec3(69.6, 69.6, 0);//initial robot position
	updatePhysicalFeatures();
	c.position = c.size;
	//vector stuff
}//constructor 
void robot::updatePhysicalFeatures() {
	//physical features
	
	c.size = size / 6;//3
	c.baseSize = size / 18;//1
	c.thickness = size / 36;//0.5
	c.speed = size / 36;//0.5;
	c.liftSpeed = size / 180;//0.1
	c.length = size / 9;//2
	mg.speed = size / 36;//0.5
	mg.size = size / 4;//4
	mg.thickness = size / 18;//1
	mg.position = mg.size;//inf
	mg.length = (1 / 2.4)*size;//7.5
}
float accelFunc(float x, float power, float rateOfChange, float friction) {
	return 2 * (power / rateOfChange) - (friction * x);
}
void robot::driveFwds(float power) {
	//konstants that should be changed later
	if(power != 0) d.basePower = power;
	float rateOfChange = 40;//constant changing the amount of initial change the acceleration goes through? maibe
	//calculate acceleration taking friction into account
	float Xaccel = accelFunc(p.velocity.X, power, rateOfChange, p.amountOfFriction);
	float Yaccel = accelFunc(p.velocity.Y, power, rateOfChange, p.amountOfFriction);
	p.maxVel = -2 * (d.basePower / rateOfChange) / (-p.amountOfFriction);
	//limiting acceleration to 0.01, no need for further acceleration rly
		if (abs(Xaccel) > 0.01) p.acceleration.X = goTo(p.acceleration.X, Xaccel, 0.8);
		else p.acceleration.X = 0;
		if (abs(Yaccel) > 0.01) p.acceleration.Y = goTo(p.acceleration.Y, Yaccel, 0.8);
		else p.acceleration.Y = 0;
	if (abs(p.velocity.X) < 0.01) p.velocity.X = 0;
	if (abs(p.velocity.Y) < 0.01) p.velocity.Y = 0;
	//if (abs(power) > 0.01)
	//	encoder1 += power;//increments the encoder while going forwards or backwards
}
void robot::readScript() {//script parser
	#define MAX_LINE 100
	readyToReRun = false;
	std::ifstream file("script.txt");
	pathPoints.clear();
	while (!file.eof()) {
		char line[MAX_LINE];
		file.getline(line, MAX_LINE);
		char command[MAX_LINE];
		char num[MAX_LINE];
		char garb[MAX_LINE];

		sscanf(line, "%s %s %s", command, num, garb);
		enum action a;
		if (std::string(command) == "driveFor(") {
			a = ACTION_FWDS;
		}
		else if (std::string(command) == "rotFor("){
			a = ACTION_ROTATE;
		}
		else {
			a = ACTION_MOGO;
		}
		commands.push_back({a, atof(num)});
	}
	d.initCommandsSize = commands.size();
	readyToReRun = true;
}
void robot::rotate(float power) {
	if (power != 0) d.rotPower = power;
	//konstants that should be changed later
	float rateOfChange = 10;//constant changing the amount of initial change the acceleration goes through? maibe
							//calculate acceleration taking friction into account
	float rotAccel = 2 * (power / rateOfChange) - (p.amountOfFriction*p.rotVel);
	p.maxRotVel = -2 * (power / rateOfChange) / (-p.amountOfFriction);

	//limiting acceleration to 0.01, no need for further acceleration rly
	if (abs(rotAccel) > 0.3) p.rotAcceleration = rotAccel;
	else p.rotAcceleration = 0;
	if (abs(p.rotVel) < 0.1) p.rotVel = 0;
}
void robot::physics::speedMult(float base, float rot) {
	velocity.X = getSign(velocity.X) * abs(velocity.X*base);
	velocity.Y = getSign(velocity.Y) * abs(velocity.Y*base);
	rotVel = getSign(rotVel) * abs(rotVel*rot);
}
float robot::truSpeed(int degree, float value) {//see here for reference https://www.desmos.com/calculator/bhwj2xjmef
	float exponented = value;//finished value after being taken to the degree's exponent
	int divisor = 1;
	if (degree % 2 == 0) {//if degree is even (need to multiply by sign) 
		for (int i = 1; i < degree; i++) {
			exponented *= value;
			divisor *= 127;//what the value is to be divided by, increases degree based off power
		}
		return getSign(value) * (exponented) / (divisor);
	}
	else {
		for (int i = 1; i < degree; i++) {
			exponented *= value;
			divisor *= 127;//what the value is to be divided by, increases degree based off power
		}
		return (exponented) / (divisor);
	}
}
void robot::stopAll() {
	p.acceleration.X = 0;
	p.acceleration.Y = 0;
	p.velocity.X = 0;
	p.velocity.Y = 0;
	p.rotAcceleration = 0;
	p.rotVel = 0;
}
bool robot::directlyInPath(bool vertical, int range, vec3 pos) {//vertical lines
	vec3 origin = pos, topLeft, topRight, bottomLeft;//calculattes yintercepts for each cone relative to their position
	float x = 0;//finds the y intercept
	float cosDist = (range / 2) * cos((-p.mRot + 135) * PI / 180) * sqrt(2);
	float sinDist = (range / 2) * sin((-p.mRot + 135) * PI / 180) * sqrt(2);
	float protrusionSin = mg.protrusion * sin((p.mRot + 90) * PI / 180)*0.75;
	float protrusionCos = mg.protrusion * cos((p.mRot + 90) * PI / 180)*0.75;
	topLeft.X = p.position.X - cosDist;
	topLeft.Y = p.position.Y + sinDist;
	topRight.X = p.position.X + sinDist;//flipped sin and cos
	topRight.Y = p.position.Y + cosDist;
	//added protrusion snippet to account for size change of base as mogo is flipped out
	bottomLeft.X = p.position.X - sinDist;// -protrusionSin;//flipped sin and cos
	bottomLeft.Y = p.position.Y - cosDist;// +protrusionCos;
	if (vertical) {
		db.slope = (topLeft.Y - bottomLeft.Y) / (topLeft.X - bottomLeft.X);//checks for vertical y intercepts
		db.Yint[1] = db.slope * (x - (topRight.X - origin.X)) + (topRight.Y - origin.Y);
	}
	else {
		db.slope = (topLeft.Y - topRight.Y) / (topLeft.X - topRight.X);//checks for horizontal y intercepts
		db.Yint[1] = db.slope * (x - (bottomLeft.X - origin.X)) + (bottomLeft.Y - origin.Y);
	}	
	db.Yint[0] = db.slope * (x - (topLeft.X - origin.X)) + (topLeft.Y - origin.Y);//both vertical and horizontal use vertices 0
	//with the y intrcepts, checks if the y intercepts are not the same sign, thus the cone (origin) is between them
	return(getSign(db.Yint[0]) != getSign(db.Yint[1]));//works for telling me if between the two lines
}
void robot::updateVertices() {
	//gross i know, but its for calculating each vertice of the robot based off its current angle;
	//math behind is based off basic trig and 45 45 90° triangle analytic geometry
	float cosDist = (size / 2) * cos((-p.mRot + 135) * PI / 180) * sqrt(2);
	float sinDist = (size / 2) * sin((-p.mRot + 135) * PI / 180) * sqrt(2);
	float protrusionSin = mg.protrusion * sin((p.mRot+90) * PI / 180);
	float protrusionCos = mg.protrusion * cos((p.mRot+90) * PI / 180);
	float mogoProp = 2.5*(mg.size) / size;//proportion of MOGO size to robot
		db.vertices[0].X = p.position.X - cosDist;
		db.vertices[0].Y = p.position.Y + sinDist;
		db.vertices[1].X = p.position.X + sinDist;//flipped sin and cos
		db.vertices[1].Y = p.position.Y + cosDist;
		db.vertices[2].X = p.position.X + cosDist;
		db.vertices[2].Y = p.position.Y - sinDist;
		db.vertices[3].X = p.position.X - sinDist;//flipped sin and cos
		db.vertices[3].Y = p.position.Y - cosDist;
		//mogo verticies
		float centerCos = cos((p.mRot) * PI / 180) * 2;
		float centerSin = sin((p.mRot) * PI / 180) * 2;

		db.MGVert[0].X = p.position.X + centerCos - mogoProp*cosDist + protrusionSin;
		db.MGVert[0].Y = p.position.Y + centerSin + mogoProp*sinDist - protrusionCos;
		db.MGVert[1].X = p.position.X + centerCos + mogoProp*sinDist + protrusionSin;//flipped sin and cos
		db.MGVert[1].Y = p.position.Y + centerSin + mogoProp*cosDist - protrusionCos;
		db.MGVert[2].X = p.position.X + centerCos + mogoProp*cosDist + protrusionSin;
		db.MGVert[2].Y = p.position.Y + centerSin - mogoProp*sinDist - protrusionCos;
		db.MGVert[3].X = p.position.X + centerCos - mogoProp*sinDist + protrusionSin;//flipped sin and cos
		db.MGVert[3].Y = p.position.Y + centerSin - mogoProp*cosDist - protrusionCos;
}
void robot::intake::claw(float robSize) {
	//janky animations for claw 
	size = 0.1*liftPos + 3;
	baseSize = 0.05*liftPos + robSize / 18;
	length = 0.07*liftPos + 2;
	thickness = 0.01*liftPos + 0.5;
	speed = 0.051*liftPos + 0.5;

	if (grabbing) { if (position > robSize / 18) position -= speed; }//animation for claw close
	else { if (position < size) position += speed; }//animation for claw open
	if (grabbing == false) holding = -1;//reset index (TO -1 (for cones) )
	if (liftDown  && liftPos > 0 && position > size) position -= speed;
}
void robot::intake::mogo(float robSize) {	
	if (grabbing) { 
		if (protrusion < length) protrusion += (robSize/18)*0.3; 
		//holding = -1;
	}//animation for protrusion mogo
	else { 
		if (protrusion > 0) protrusion -= (robSize/18)*0.3;
	}//animation for intruding mogo
	//if (grabbing == false) holding = -1;//reset index (TO -1 (for mogos) )
}
void robot::update(field *f, int rob) {
	robot::updateVertices();
	p.velocity = p.velocity + p.acceleration.times(1.0/60.0);
	p.position.Y += p.velocity.Y * sin((p.mRot)*(PI / 180));//velocity scaled because of rotation
	p.position.X += p.velocity.X * cos((p.mRot)*(PI / 180));//velocity scaled because of rotation
	p.rotVel += 2*p.rotAcceleration*(1.0 / 60.0);//constant is based off realistic tests
	p.mRot += p.rotVel;
	decisions(f, rob);//does the thinkings
	//simulating gyroscope rotations (rolloff at +-360 deg)
	//COMMENT OUT THE LINE BELOW WHEN DOING PROG SKILLS #2 RN
	//p.mRot = ((p.mRot / 360) - (long)(p.mRot / 360)) * 360;//only within 360° and -360° (takes the decimal portion and discards the whole number)
	c.claw(size);
	mg.mogo(size);
	if (c.liftUp && c.liftPos < c.maxHeight) c.liftPos += 4.5*c.liftSpeed;
	else if (c.liftDown && c.liftPos > 0) c.liftPos -= 8.5*c.liftSpeed; //goes faster coming down
}
void robot::moveAround() {
	if (!readyToReRun) {//not rerunning
		if (ctrl.KeyUp) driveFwds(d.motorSpeed);//checking up key
		else if (ctrl.KeyDown) driveFwds(-d.motorSpeed);//checking down key
		else driveFwds(0);//welp, no movement

		if (ctrl.KeyLeft) rotate(d.motorSpeed);//checking left key
		else if (ctrl.KeyRight) rotate(-d.motorSpeed);//checking right key
		else rotate(0);//welp, no rotation
	}
}
void robot::setPos(vec3 pos) {
	p.position.X = pos.X;
	p.position.Y = pos.Y;
	p.mRot = pos.Z;
}
float robot::getSize() {
	return size;
}

//FOR AUTOBOTS
void robot::decisions(field *f, int rob) {
	if (c.holding != c.goal) {//dynamically refreshes which cone is best in position to be picked up
		int closest = 0;//assumes cone 0 is closest
		for (int i = 0; i < f->c.size() - 1; i++) {
			if (f->c[i].targetted == false || (f->c[i].targetted == true && c.goal == i)) {
				if (f->c[i].pos.distance(f->pl[0].pos) > f->c[i].radius * 4 && f->c[i].pos.distance(f->pl[1].pos) > f->c[i].radius * 4) {
					if (p.position.distance(f->c[i].pos) < p.position.distance(f->c[closest].pos)) {
						if (f->c[i].pos.Z <= cHeight) {//so long as not already stacked or in the air
							closest = i;//updates "closest" to whichever cone is closest
						}
					}
					else f->c[i].targetted = false;
				}
			}
		}
		c.goal = closest;
		f->c[closest].targetted = true;//considers cone as targetted
	}
	if (mg.holding != mg.goal) {//dynamically refreshes which mogo is best in position to be picked up
		int closest = 0;//assumes cone 0 is closest
		for (int i = 0; i < f->mg.size(); i++) {
			if (p.position.distance(f->mg[i].pos) < p.position.distance(f->mg[closest].pos)) {
				if (f->mg[i].pos.Z <= cHeight)//so long as not already stacked or in the air
					closest = i;//updates "closest" to whichever cone is closest
			}
		}
		mg.goal = closest;
	}
	if (thinking) {
		if (grabMoGo) {//aiming for mogo
			if (mg.holding != mg.goal + 100) {
				goGrab(f, &f->mg[mg.goal], mg.goal, rob);
			}
			else {
				placeIn(f, f->f.z);
			}
		}
		else {//aiming for cones
			if (c.holding != c.goal) {
				goGrab(f, &f->c[c.goal], c.goal, rob);
			}
			else {
				int poleNum = 0;//assuming robot is closer to pole0 than pole1
				if (p.position.distance(f->pl[0].pos) > p.position.distance(f->pl[1].pos)) {
					poleNum = 1;//robot is closer to pole1 than pole0
				}
				//always stacks on pole 1
				/*int mogoNum = 0;
				for (int i = 0; i < f->mg.size(); i++) {
				if (f->mg[i].colour == 2) {
				if (p.position.distance(f->mg[mogoNum].pos) > p.position.distance(f->mg[i].pos))
				mogoNum = i;
				}
				}*/
				stackOn(f, &f->pl[poleNum]);
			}
		}
	}
}

//for autobot going to grabbing an element
void robot::goGrab(field *f, element *e, int index, int roboIndex) {
	float d2V[4];
	for (int ver = 0; ver < 4; ver++) {
		d2V[ver] = e->pos.distance(db.vertices[ver]);
	}
	int dir = 1;
	if (!reRouting) {
		float speed = limitTo(MAXSPEED, 3 * p.position.distance(e->pos));
		bool inFront = ((d2V[0] + d2V[1]) < (d2V[2] + d2V[3]));//checking if c.goal[rob-1] is closer to the front side
		bool onRight = ((d2V[1] + d2V[2]) < (d2V[0] + d2V[3]));//checking if c.goal[rob-1] is closer to the right side
		if (onRight) dir = -1;
		if (grabMoGo) {/*WORK ON THIS*/
			if (e->inPossession.find(roboIndex) == e->inPossession.end() && p.position.distance(e->pos) >= (size / 2 + 1 + e->radius)) {
				if (mg.holding == -1) mg.grabbing = true;//pulls out mogo-inator
				rotate(0);
				if (!directlyInPath(true, size / 4, e->pos)) {
					rotate(dir * speed);
					driveFwds(0);
				}
				else driveFwds(speed);
			}
			else {
				mg.holding = index + 100;
				mg.grabbing = false;//only closes claw if on same level (height wise)
				driveFwds(0);
				rotate(0);
			}
			if (p.velocity.X == 0 && p.velocity.Y == 0) {//get it to do the same if touching fence
				reRouting = true;
			}
		}
		else {
			if (!directlyInPath(true, size / 2, e->pos) || !inFront) {//angle is not pointing towards goal
				rotate(dir * speed);
				driveFwds(0);
			}
			else rotate(0);
			float offset = 0.5;//dosent update fast enough for small cones, needed little offset heuristic
			if (p.position.distance(e->pos) > (size / 2 + e->radius) + offset && inFront) {//drive fwds towards c.goal[rob-1]
				c.grabbing = false;
				driveFwds(speed);
			}
			else {
				if (abs(c.liftPos - e->pos.Z) < e->height) c.grabbing = true;//only closes claw if on same level (height wise)
				driveFwds(0);
			}
			if (c.grabbing && c.holding == index) {//holding the cone
				if (c.liftPos < 24 + 4) c.liftUp = true;//24 is height of stagos
				else c.liftUp = false;
			}
			else {
				c.liftUp = false;
				rotate(dir * 50);//just do a simple rotation to try and minimize error, gets it closer to the center 
			}
			if (c.holding == -1) { // not holding the cone
				if (c.liftPos > 0) c.liftDown = true;
				else c.liftDown = false;
			}
			else {
				c.liftDown = false;
			}
		}
		if (p.velocity.X == 0 && p.velocity.Y == 0) {//get it to do the same if touching fence
			reRouting = true;
		}
	}
	else {
		reRoute(f, &f->c[c.goal], dir);
	}
}
//for autobot choosing another route because blocked in some ways
void robot::reRoute(field *f, element *e, int dir) {
	int poleNum = 0;//assuming robot is closer to pole0 than pole1
	if (p.position.distance(f->pl[0].pos) > p.position.distance(f->pl[1].pos)) {
		poleNum = 1;//robot is closer to pole1 than pole0
	}
	if (p.position.distance(f->pl[poleNum].pos) < (size)) {//far enough out of the way
		driveFwds(-127);
	}
	else {//fix this little "heuristic" make it actually detect when theres an obstacle before it and turn around it
		if (directlyInPath(true, size / 4, f->c[rand() % f->c.size()].pos)) rotate(dir * MAXSPEED);//moving to horizontal path (turning like 90°)
		else reRouting = false;
		driveFwds(0);
		//v.reRouting = false;
	}
}
//for autobot, placing element into a specific field zone
void robot::placeIn(field *f, fence::zone *z) {
	/*vec3 zone;
	if(z[1].twentyPoint.size() < 1) zone = vec3(127, 127);
	else if (z[1].tenPoint.size() < 1) zone = vec3(120, 120);
	else zone = vec3(108, 108);*/
	float d2V[4];
	int dir = 1;
	for (int ver = 0; ver < 4; ver++) {
		d2V[ver] = vec3(140, 140).distance(db.vertices[ver]);
	}
	bool inFront = ((d2V[0] + d2V[1]) < (d2V[2] + d2V[3]));//checking if c.goal[rob-1] is closer to the front side
	bool onRight = ((d2V[1] + d2V[2]) < (d2V[0] + d2V[3]));//checking if c.goal[rob-1] is closer to the right side
	if (onRight) dir = -1;
	if (!reRouting) {
		float speed = limitTo(MAXSPEED, 3 * p.position.distance(vec3(140, 140)));
		if (abs(p.position.Y - f->f.poleEquation(140.05, 117.5, -1, p.position.X))>15) {
			if (!directlyInPath(true, size / 2, vec3(140, 140)) || !inFront) {//angle is not pointing towards c.goal[rob-1]
				rotate(dir * speed);
				driveFwds(0);
			}
			else {
				rotate(0);
				driveFwds(dir * speed);
			}
		}
		else {
			//driveFwds(0);
			mg.grabbing = true;//lets go of mogo
			if (mg.protrusion > 7) {//not quite at the ground yet
				driveFwds(-MAXSPEED);//get outta there
			}
		}
	}
	else {//stop moving
		driveFwds(0);
		rotate(0);
	}
}
//for autobot stacking cone on certain element
void robot::stackOn(field *f, element *e) {
	float d2V[4];
	for (int ver = 0; ver < 4; ver++) {
		d2V[ver] = e->pos.distance(db.vertices[ver]);
	}
	int dir = 1;
	bool inFront = (d2V[0] + d2V[1] < d2V[2] + d2V[3]);//checking if c.goal[rob-1] is closer to the front side
	bool onRight = (d2V[1] + d2V[2] < d2V[0] + d2V[3]);//checking if c.goal[rob-1] is closer to the right side
	if (onRight) dir = -1;
	if (!reRouting) {
		float speed = limitTo(MAXSPEED, 3 * p.position.distance(e->pos));
		if (!directlyInPath(true, size / 2, e->pos) || !inFront)//angle is not pointing towards c.goal[rob-1]
			rotate(dir * speed);
		else rotate(0);
		if (c.grabbing) {//holding the cone
			if (c.liftPos < e->height + e->stacked.size() + 4) c.liftUp = true;
			else c.liftUp = false;
		}
		else {
			c.liftUp = false;
			rotate(dir * 50);//just do a simple smaller rotation to try and minimize error, gets it closer to the center 
		}
		if (c.liftPos >= e->height + 2 /*+ADD STACKED POS CHANGER HERE*/) {//wait until lift is reasonably high
			if (p.position.distance(e->pos) > size*0.5 + e->radius + 2 && inFront) {//drive fwds towards c.goal[rob-1]
				driveFwds(speed*0.7);//slower since carrying object? eh idk
			}
			else {
				driveFwds(0);
				if (p.position.distance(e->pos) <= size*0.5 + e->radius + 2) {//reall close to the c.goal[rob-1]
					if (c.liftPos >= e->height && c.grabbing) {
						rotate(0);
						p.acceleration = vec3(0, 0, 0);
						p.velocity = vec3(0, 0, 0);
						p.rotAcceleration = 0;
						p.rotVel = 0;
						if (directlyInPath(true, size / 4, e->pos)) {
							c.grabbing = false;//opens claw
												  //thinking = false;//basically turns off autonomous thing
							c.goal = 0;//resets goal
						}
						else rotate(dir * 127);//just do a simple smaller rotation to try and minimize error, gets it closer to the center 
					}
				}
				//else rotate(dir * 127);//just do a simple smaller rotation to try and minimize error, gets it closer to the center 
			}
		}
		else {//stop moving
			driveFwds(0);
			rotate(0);
		}
	}
	else {//stop moving
		driveFwds(0);
		rotate(0);
	}
}