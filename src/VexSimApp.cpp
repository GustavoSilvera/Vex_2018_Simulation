#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Vector.h"
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Utilities.h"

#include <ostream>
#include <fstream>
#include <vector>
#include <filesystem>
//my own headers
#include "field.h"
#include "robot.h"
#include "vec3.h"
#include "randomstuff.h"

//declaration for the main things, simulation and whatnot
using namespace ci;
using namespace ci::app;
using namespace std;
std::ofstream scriptFile;

extern int numCones;
float winScale;//scale of window size for displaying properly
Font mFont;//custom font for optimized drawing
gl::TextureFontRef mTextureFont;//custom opengl::texture
vec3 startPos;
vec3 mousePos;
class vex {
public:
	std::vector<robot> r;
	field f;
	vex(int numRob) :
		r(numRob),//how many robots on field?
		f(&r) {}
	bool debugText = true;
	bool recording = false;
};
//begin
int tX = 1200;
class VexSimApp : public AppNative {
public:
	VexSimApp() :v(3) {}//how to modify in real time????
	void prepareSettings(Settings *settings);
	void setup();
	void mouseDown(MouseEvent event);
	void mouseUp(MouseEvent event);
	void mouseMove(MouseEvent event);
	void keyDown(KeyEvent event);
	void keyUp(KeyEvent event);
	void update();
	struct button {
		void buttonClick(int x, int y, int num_buttons, int size);
		void buttonHover(int x, int y, int num_buttons, int size);
		void buttonsDraw(int size);
		int x, y, size;
	};
	void textDraw();
	void drawDials(vec3I begin);
	static void drawFontText(float text, vec3I pos, vec3I colour, int size);
	//for customize panel
	struct customizePanel {
		void controlPanel(robot *r);
		void callAction(bool increase, int buttonAction);
		bool buttonHover(vec3 mouse, int x, int y, int x2, int y2, int index, int buttonAction);
		void ctrlButton(vec3 mouse, int x, int y, int x2, int y2, float winScale, int buttonAction);
		float size = 18;
		float motorPower = MAXSPEED;//power for the base
		int numberOfRobots = 1;
		bool mouseClicked = false;
		float buttonXPos[2];//only used for numRobots
		float buttonX2Pos[2];//only used for numRobots
		float buttonYPos[2];//only used for numRobots
	};
	customizePanel cp;
	//for buttons and stuff
	struct text {
		string s;
	};
	//for drawing stuff
	Vec2f R2S2(vec3 robot_coord);
	Vec3f R2S3(float robot_coordX, float robot_coordY, float robot_coordZ);
	Rectf R2S4(float p1X, float p1Y, float p2X, float p2Y);
	void robotDebug();
	void drawClaw(robot *r);
	void drawRobot(robot *r);
	void draw();
	vex v;
	bool debuggingBotDraw = false;
	ci::gl::Texture dial;
private:
	// Change screen resolution
	int mScreenWidth, mScreenHeight;
	float initWidth, initHeight;
	void getScreenResolution(int& width, int& height);
};
void VexSimApp::prepareSettings(Settings *settings)
{
	//! setup our window
	settings->setTitle("In The Zone Simulation");
	settings->setFrameRate(60);//60fps
	gl::enableVerticalSync();//vsync
	getScreenResolution(mScreenWidth, mScreenHeight);//getss resolution relative to monitor
	settings->setWindowPos(mScreenWidth / 4, mScreenHeight / 6);
	int aspectRatio = mScreenWidth / 8;//using half of monitor resolution
	initWidth = aspectRatio * 4;
	initHeight = aspectRatio * 3;
	settings->setWindowSize(initWidth, initHeight);//maintains 4:3 aspect ratio
}
//initial setup for all the variables and stuff
void VexSimApp::setup() {
	srand(time(NULL));//seeds random number generator
	try {
		robot::TankBase = gl::Texture(loadImage(loadAsset("Tank Drive.png")));
		robot::TankBase2 = gl::Texture(loadImage(loadAsset("Tank Drive WheelSpin.png")));
		robot::CChanel = gl::Texture(loadImage(loadAsset("CChanelSmall.png")));
		robot::CChanelVERT = gl::Texture(loadImage(loadAsset("CChanelSmallVERT.png")));
		field::fieldBare = gl::Texture(loadImage(loadAsset("InTheZoneFieldBare.jpg")));
		element::coneTexture = gl::Texture(loadImage(loadAsset("InTheZoneCone.png")));
		element::MobileGoal = gl::Texture(loadImage(loadAsset("MoGoWhite.png")));
		dial = gl::Texture(loadImage(loadAsset("dialBkgrnd.png")));
	}
	catch (const std::exception &e) {
		app::console() << "Could not load and compile shader:" << e.what() << std::endl;
	}
	winScale = (float)getWindowWidth() / (float)initWidth;
	mFont = Font("Arial", 35*winScale);//fixed custom font
	mTextureFont = gl::TextureFont::create(mFont);
}
void VexSimApp::getScreenResolution(int& width, int& height){
	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();

	// Get the size of the screen into a rectangle
	RECT rDesktop;
	GetWindowRect(hDesktop, &rDesktop);

	// The top left corner will have coordinates (0, 0)
	// and the bottom right corner will have coordinates
	// (width, height)
	width = rDesktop.right;
	height = rDesktop.bottom;

}
//when mouse is clicked
void VexSimApp::mouseDown(MouseEvent event) {
	//if (event.isLeft()) s.mouseClicked = true;
	//buttonClick(event.getX(), event.getY(), 6, 100);
	/*if (s.SimRunning == s.CUSTOMIZE) {
		cp.mouseClicked = true;
		if (event.getY() > (cp.buttonYPos[0]) &&
			event.getY() < (cp.buttonYPos[1])) {//within y bounds
			if (event.getX() > (cp.buttonXPos[0]) &&//within the DECREASE bounds
				event.getX() < (cp.buttonX2Pos[0])) {
				if (!v.r.empty()) v.r.pop_back();
			}
			else if (event.getX() > (cp.buttonXPos[1]) &&//within the INCREASE bounds
				event.getX() < (cp.buttonX2Pos[1])) {
				robot rob;//creates new robit;
				v.r.push_back(rob);
				v.f.initialize(&v.r);
			}
		}
	}*/
}
//when mouse is released
void VexSimApp::mouseUp(MouseEvent event) {
	///if (event.isLeft())	s.mouseClicked = false;
	cp.mouseClicked = false;
}
//when mouse is moved (used with joystick)
void VexSimApp::mouseMove(MouseEvent event) {
	//buttonHover(event.getX(), event.getY(), 6, 100);
	//if (s.SimRunning == s.CUSTOMIZE) {
	//	mousePos.X = event.getX();
	//	mousePos.Y = event.getY();
	//}
}
//what to do when keyboard key is pressed
void VexSimApp::keyDown(KeyEvent event) {
	//base
	if (event.getCode() == KeyEvent::KEY_UP || event.getChar() == 'w' || event.getChar() == 'W')	v.r[0].ctrl.KeyUp = true;
	if (event.getCode() == KeyEvent::KEY_DOWN || event.getChar() == 's' || event.getChar() == 'S')	v.r[0].ctrl.KeyDown = true;
	if (event.getCode() == KeyEvent::KEY_LEFT || event.getChar() == 'a' || event.getChar() == 'A')	v.r[0].ctrl.KeyLeft = true;
	if (event.getCode() == KeyEvent::KEY_RIGHT || event.getChar() == 'd' || event.getChar() == 'D') v.r[0].ctrl.KeyRight = true;
	//intakes
	if (event.getChar() == 'p' || event.getChar() == 'P' || event.getChar() == 'e' || event.getChar() == 'E') v.r[0].c.grabbing = !v.r[0].c.grabbing;//if want toggling, else look at a while ago
	if (event.getChar() == 'o' || event.getChar() == 'O' || event.getChar() == 'r' || event.getChar() == 'R') v.r[0].mg.grabbing = !v.r[0].mg.grabbing;//if want toggling, else look at a while ago
																																					   //lift
	if (event.getCode() == KeyEvent::KEY_SPACE) v.r[0].c.liftUp = true;
	if (event.getCode() == KeyEvent::KEY_RSHIFT || event.getCode() == KeyEvent::KEY_LSHIFT) v.r[0].c.liftDown = true;//left Z button
																													 //debug
	if (event.getChar() == 'm' || event.getChar() == 'M') v.debugText = !v.debugText;
	if (event.getChar() == 'n' || event.getChar() == 'N') v.f.initialize(&v.r);//reset field
	if (event.getChar() == 'b' || event.getChar() == 'B') debuggingBotDraw = !debuggingBotDraw;//draw cool lines
																							   //other
	if (event.getChar() == 'c') v.r[0].readScript();
	if (event.getChar() == 'q') {
		for (int rob = 1; rob < v.r.size(); rob++) {
			if (v.r[rob].thinking != true) v.r[rob].thinking = true;
			else {
				v.r[rob].thinking = false;
				v.r[rob].stopAll();
			}
		}
	}
}
//what to do when keyboard key is released
void VexSimApp::keyUp(KeyEvent event) {
	//base
	if (event.getCode() == KeyEvent::KEY_DOWN || event.getChar() == 's' || event.getChar() == 'S') v.r[0].ctrl.KeyDown = false;
	if (event.getCode() == KeyEvent::KEY_UP || event.getChar() == 'w' || event.getChar() == 'W') v.r[0].ctrl.KeyUp = false;
	if (event.getCode() == KeyEvent::KEY_RIGHT || event.getChar() == 'd' || event.getChar() == 'D') v.r[0].ctrl.KeyRight = false;
	if (event.getCode() == KeyEvent::KEY_LEFT || event.getChar() == 'a' || event.getChar() == 'A') v.r[0].ctrl.KeyLeft = false;
	//lift
	if (event.getCode() == KeyEvent::KEY_SPACE) v.r[0].c.liftUp = false;
	if (event.getCode() == KeyEvent::KEY_RSHIFT || event.getCode() == KeyEvent::KEY_LSHIFT) v.r[0].c.liftDown = false;//left Z button
}
//overall application update function
void VexSimApp::update() {
	winScale = (float)(getWindowWidth() + getWindowHeight() ) / (float)(initWidth + initHeight);//change in window size relative to initial
	float pastRot = v.r[0].p.mRot;
	vec3 pastPos(v.r[0].p.position);
	float pastTime = ci::app::getElapsedSeconds();
	int signVX = getSign(v.r[0].p.velocity.X), signVY = getSign(v.r[0].p.velocity.Y), signRot = getSign(v.r[0].p.rotVel);
	for (int rob = 0; rob < v.r.size(); rob++) {
		v.r[rob].update(&v.f, rob);//calls robot update function
	}
	tX = 1200;
	v.f.isInit = true;
	v.f.FieldUpdate(&v.r);
	v.r[0].moveAround();
	v.r[0].db.distance += getSign(v.r[0].d.basePower)*v.r[0].p.position.distance(pastPos);
	v.r[0].db.rotDist += (v.r[0].p.mRot - pastRot);
	if (v.r[0].readyToReRun) {
		enum action {
			ACTION_ROTATE,
			ACTION_FWDS,
			ACTION_MOGO
		};
		if (v.r[0].commands.size() > 0) {//make this more accriate
			float maxSpeed = 127;
			if (v.r[0].commands[0].amnt != 0 || v.r[0].commands[0].a == ACTION_MOGO) {//at least 
				if (v.r[0].commands[0].a == ACTION_ROTATE) {//for rotate
					if (abs(v.r[0].db.rotDist) <= abs(v.r[0].commands[0].amnt)) {
						float power = limitTo(127, sqr(v.r[0].commands[0].amnt - v.r[0].db.rotDist));
						v.r[0].rotate(getSign(v.r[0].commands[0].amnt) * power);
						v.r[0].driveFwds(0);
					}
					else {
						v.r[0].stopAll();
						v.r[0].db.rotDist = RESET;
						v.r[0].rotate(0);
						v.r[0].commands.erase(v.r[0].commands.begin());//removes first element of vector
					}
				}
				else if (v.r[0].commands[0].a == ACTION_FWDS) {//for fwds
					if (abs(v.r[0].db.distance) <= abs(v.r[0].commands[0].amnt)) {
						v.r[0].driveFwds(getSign(v.r[0].commands[0].amnt) * v.r[0].d.motorSpeed);
						v.r[0].rotate(0);
					}
					else {
						v.r[0].pathPoints.push_back(v.r[0].p.position);//create another line to this point
						v.r[0].stopAll();
						v.r[0].db.distance = RESET;
						v.r[0].driveFwds(0);
						v.r[0].commands.erase(v.r[0].commands.begin());//removes first element of vector
					}
				}
				else if (v.r[0].commands[0].a == ACTION_MOGO) {//for fwds
					v.r[0].mg.grabbing = !v.r[0].mg.grabbing;
					v.r[0].commands.erase(v.r[0].commands.begin());//removes first element of vector
				}
			}
			else {
				v.r[0].commands.erase(v.r[0].commands.begin());
			}
		}
		else {
			v.r[0].db.distance = RESET;
			v.r[0].db.rotDist = RESET;
			v.r[0].driveFwds(0);
			v.r[0].rotate(0);
			v.r[0].stopAll();
			v.r[0].readyToReRun = false;
		}
	}
	if (v.recording) {//macro recording		//less accurate (straight line running)
		if ((abs(v.r[0].p.velocity.X) > 0.1 && abs(v.r[0].p.velocity.Y == 0)>0.1) ||
			(getSign(v.r[0].p.velocity.X) != signVX &&
				getSign(v.r[0].p.velocity.Y) != signVY)) {//stopped or changed direction
			if (abs(((int)(v.r[0].db.distance * 10)) / 10) >= 0.01) {//but still moved 
				std::stringstream dummyText2;
				std::string distance;
				dummyText2 << (((int)(v.r[0].db.distance * 10)) / 10);
				dummyText2 >> distance;
				scriptFile << "driveFor( " + distance + ");\n";//used for scripting

				v.r[0].db.distance = RESET;//resets change in position after a while
			}
		}
		if (v.r[0].p.rotVel == 0) {//rotation changed
			if (abs(v.r[0].db.rotDist) > 2) {//difference in rotation
				std::stringstream dummyText;
				std::string newAngle;
				dummyText << (v.r[0].db.rotDist);//angle change
				dummyText >> newAngle;
				scriptFile << "rotFor( " + newAngle + ");\n";//used for scripting
				v.r[0].db.rotDist = RESET;//resets change in position after a while
			}
		}
	}
}

//for on screen buttons, what to do when being pressed
void VexSimApp::button::buttonClick(int x, int y, int numButtons, int size) {
	/*
	for (int i = 0; i < numButtons; i++) {//for each button in the array 
		if (x > winScale * (size * (i + 1) - (size / 2) + (25 * (i + 1))) &&
			x < winScale * (size * (i + 1) + (size / 2) + (25 * (i + 1))) &&
			y > winScale * 25 && y < winScale * 75) {//within boundaries for each button based off their index
			if (i <= 3) {//first four buttons
				s.SimRunning = simulation::SimulationType(i);
			}
			else if (i == 4) {//fifth button
				v.recording = true;//toggles macro recording
				scriptFile = std::ofstream("script.txt");
			}
			else if (i == 5) {//sixth button
				v.recording = false;//toggles macro recording
				scriptFile << "driveFor( 0.015);\n";//used for final script(no repeats basically)
				scriptFile = std::ofstream("script.txt", fstream::app);
			}
		}
	}*/
}
//for on screen upper buttons, which get outlined red when being hovered over
void VexSimApp::button::buttonHover(int x, int y, int numButtons, int size) {
	/*for (int i = 0; i < numButtons; i++) {//for each button in the array 
		if (x > winScale * (size * (i + 1) - (size / 2) + (25 * (i + 1))) &&
			x < winScale * (size * (i + 1) + (size / 2) + (25 * (i + 1))) &&
			y > winScale * 25 && y < winScale * 75) {//within boundaries for each button based off their index
			s.hovering = i;
		}
	}*/
}
//drawing on screen buttons
void VexSimApp::button::buttonsDraw(int buttonSize) {//function for drawing the buttons
	int bY = 50, dInBtw = 25;//array for #buttons, bY is y position of each btn, dInBtw is distance in bwtween buttons
	int i = 1;
	text t[] = {//6 is numButtons
		{ "InitRec:" },
		{ "StopRec:" }
	};
	//use str.length() to get number of chars and dynamically push back other things
	/*for (text& ti : t) {
		if (i - 1 == s.SimRunning) { gl::color(0, 1, 0); }//if the button's index is equal to whichever button's index is being hovered over
		else if (i - 1 == s.hovering) { gl::color(1, 0, 0); }//if the button's index is equal to whichever button's index is being hovered over
		else { gl::color(1, 1, 1); }
		gl::drawStrokedRoundedRect(Area(winScale *(i * buttonSize - buttonSize / 2 + dInBtw*i), winScale*(bY - 25), winScale*(i*buttonSize + buttonSize / 2 + dInBtw*i), winScale*(bY + 25)), 5);//ROUNDED rectangle with corner rad of 7
		gl::color(1, 1, 1);//resets colour 
		gl::drawString(t[i - 1].s, Vec2f(winScale*(i * buttonSize - t[i - 1].s.length() * 5 + dInBtw*i), winScale*(bY - 12.5)), Color(1, 1, 1), Font("Arial", winScale * 25));
		i++;
	}*/
}
//drawing the dials and their needles
void drawDial(float amnt, vec3 pos, float max, float scale, ci::gl::Texture dial) {
	int width = 2 * scale;//px fixed
	int radius = 80 * scale;//px fixed
	int init = 226;//initial angle
	gl::draw(dial, Area(
		(pos.X - radius*1.2),
		(pos.Y - radius*1.2),
		(pos.X + radius*1.2),
		(pos.Y + radius*1.2)));
	glPushMatrix();//rtation
	float angle = init - amnt*((init - 35) / max);
	gl::translate(Vec3f(pos.X, pos.Y, 0));//origin of rotation
	gl::rotate(Vec3f(0, 0, -angle - 90));//something for like 3D rotation.... ugh
	gl::color(abs(init / limitFrom(0.1, angle)), abs(angle / init), 0);//cool colour transition (neat maths)
	gl::drawSolidRect(Area(Vec2d(-width, 0), Vec2d(width, radius)));//draws dial
	glPopMatrix();//end of rotation code
	gl::color(1, 1, 1);//resets colour to white
}
//drawing the text used for debugging or just other details
void VexSimApp::textDraw() {//function for drawing the buttons 
								//(	WARNING: RESOURCE HOG!!!!!!!!!!!)
	const int dInBtw = 50;//array for #buttons, bY is y position of each btn, dInBtw is distance in bwtween buttons
	struct text {
		string s;
		double f;
	};
	text t[] = {
		{ "Angle:", ((v.r[0].p.mRot / 360) - (long)(v.r[0].p.mRot / 360)) * 360 },//only within 360� and -360� (takes the decimal portion and discards the whole number)
		{ "X Pos:", v.r[0].p.position.X },
		{ "Y Pos:", v.r[0].p.position.Y },
		{ "L-Pos:", v.r[0].c.liftPos },
		{ "R-Acc:", v.r[0].p.rotAcceleration },
		{ "RDist:", v.r[0].db.rotDist },
		{ "Auto%", ((float)v.r[0].commands.size() / (float)v.r[0].d.initCommandsSize) }
		//velocity and acceleration measured with drawDials
	};
	int i = 0;
	for (text& ti : t) {
		int tY = (i + 1) * dInBtw;//increment x position for each button based off index
		gl::drawString(ti.s, Vec2f(winScale*(tX - 70), winScale*(tY)), Color(1, 1, 1), Font("Arial", winScale * 30));
		drawFontText(ti.f, vec3I(winScale*(tX), winScale*(tY)), vec3I(1, 1, 1), winScale * 30);
		++i;
	}
	gl::color(1, 1, 1);

}
//returning hypotenuse between x and y values of a triangle
float getHypo(vec3 val) {
	return sqrt(sqr(val.X) + sqr(val.Y));
}
//drawing the dials used for measuring variables
void VexSimApp::drawDials(vec3I begin) {
	//drawwing DIALs
	struct text {
		vec3 val;
		string s;
		float max;
	};
	text t[] = {
		{ v.r[0].p.velocity, "Vel:", abs(v.r[0].p.maxVel) },//even
		{ v.r[0].p.acceleration, "Acc:", 5 },//odd
		{ v.r[0].p.rotVel, "rVel:", 3.5 }//even
										 //{ v.r[1].p.velocity, "Vel:", abs(v.r[0].p.maxVel) },//even
										 //{ v.r[1].p.acceleration, "Acc:", 5 },//odd
										 //{ v.r[1].p.rotVel, "rVel:", 3.5 }//even

	};
	int i = 0;
	vec3 offset;
	for (text& ti : t) {//draws in grid like pattern, events on right, odds on left
		float total = getHypo(ti.val);
		if (i % 2 == 0) offset = vec3(0, 120 * i).times(winScale);
		else offset = vec3(100 * 2, 120 * (i - 1)).times(winScale);
		drawDial(total, vec3(begin.X + offset.X, begin.Y + offset.Y), ti.max, winScale, dial);//draws the dials
		vec3I stringBegin = vec3I(begin.X + offset.X - 70 * winScale, begin.Y + offset.Y + 120 * winScale);//innitial sstring position
		mTextureFont->drawString(ti.s, Vec2f(stringBegin.X, stringBegin.Y));//draws words
		drawFontText(total, vec3I(stringBegin.X + 50 * winScale, stringBegin.Y - 20 * winScale), vec3I(1, 1, 1), 30);//draws values
		i++;
	}

}
//drawing text in an optimized way using custom fonts
void VexSimApp::drawFontText(float text, vec3I pos, vec3I colour, int size) {
	std::stringstream dummyText;
	std::string PRINT;
	dummyText << text;
	dummyText >> PRINT;
	gl::color(colour.X, colour.Y, colour.Z);
	mTextureFont->drawString(PRINT, Vec2f(pos.X, pos.Y + 20 * winScale));
	gl::color(1, 1, 1);
}
//like buttonpress for control panel buttons
void VexSimApp::customizePanel::callAction(bool increase, int buttonAction) {
	if (buttonAction == 0) {
		if (increase) size += 0.1;
		else size -= 0.1;
	}
	else if (buttonAction == 1) {
		if (increase) motorPower++;
		else motorPower--;
	}
}
//for hovering over buttons in control panel
bool VexSimApp::customizePanel::buttonHover(vec3 mouse, int x, int y, int x2, int y2, int index, int buttonAction) {
	if (mouse.X > (x) &&
		mouse.X < (x2) &&
		mouse.Y >(y) &&
		mouse.Y < (y2)) {//within boundaries for each button based off their index
						 //mouse is hovering
		if (mouseClicked) {///bad still (should be implemented in the mouse event)
			if (index == 0)//left button
				callAction(false, buttonAction);
			else if (index == 1)//right button
				callAction(true, buttonAction);
		}
		return true;
	}
	return false;
}
//for drawing and defining buttons in control panel
void VexSimApp::customizePanel::ctrlButton(vec3 mouse, int x, int y, int x2, int y2, float winScale, int buttonAction) {
	Color(1, 0, 0);
	int sizeOf = (x2 - x);
	x *= winScale; y *= winScale; x2 *= winScale; y2 *= winScale;//gets scaling out of the way
	float dInBtwn = sizeOf*1.15;
	buttonXPos[0] = x + 0 * dInBtwn;
	buttonX2Pos[0] = x2 + 0 * dInBtwn;
	buttonXPos[1] = x + 1 * dInBtwn;
	buttonX2Pos[1] = x2 + 1 * dInBtwn;
	buttonYPos[0] = y;
	buttonYPos[1] = y2;

	for (int i = 0; i < 2; i++) {
		if (buttonHover(mouse, x + i*dInBtwn, y, x2 + i*dInBtwn, y2, i, buttonAction))
			gl::color(1, 0, 0);//if the button's index is equal to whichever button's index is being hovered over
		else  gl::color(1, 1, 1);
		gl::drawStrokedRoundedRect(Area(x + i*dInBtwn, y, x2 + i*dInBtwn, y2), 5);//ROUNDED rectangle with corner rad of 7
		if (i == 0) gl::drawString("<--", Vec2f(0.5*(x + x2) - 20 + i*dInBtwn, 0.5*(y + y2) - 10), Color(1, 1, 1), Font("Arial", winScale * 30));
		else gl::drawString("-->", Vec2f(0.5*(x + x2) - 20 + i*dInBtwn, 0.5*(y + y2) - 10), Color(1, 1, 1), Font("Arial", winScale * 30));
	}
}
//for defining control panel and what it does to the robots variables
void VexSimApp::customizePanel::controlPanel(robot *r) {//function for drawing the buttons 
	const int dInBtw = 50;//array for #buttons, bY is y position of each btn, dInBtw is distance in bwtween buttons
	struct text {
		string s;
		double f;
	};
	text t[] = {
		{ "Size:", size },
		{ "Power:", motorPower },
		{ "robots:", numberOfRobots }//number of robots ERROR WHEN INCREASING
	};
	int i = 0;
	//use str.length() to get number of chars and dynamically push back other things
	for (text& ti : t) {
		int tY = (i + 1) * dInBtw;//increment x position for each button based off index
		gl::drawString(ti.s, Vec2f(winScale*(tX - ti.s.length() * 17), winScale*(tY)), Color(1, 1, 1), Font("Arial", winScale * 40));
		VexSimApp::drawFontText(ti.f, vec3I(winScale*(tX), winScale*(tY)), vec3I(1, 1, 1), winScale * 40);
		int buttonStart = tX + ti.s.length() * 10;
		ctrlButton(mousePos, buttonStart, tY, buttonStart + 60, tY + 30, winScale, i);
		++i;
	}
}
// Map 2D robot coordinates to screen coordinates.
Vec2f VexSimApp::R2S2(vec3 robot_coord) {
	return Vec2f(ppi * winScale * (v.f.f.inFromEnd + robot_coord.X), ppi * winScale * (v.f.f.inFromEnd + v.f.f.fieldSizeIn - robot_coord.Y));
}
// Map 3D robot coordinates to screen coordinates.
Vec3f VexSimApp::R2S3(float robot_coordX, float robot_coordY, float robot_coordZ) {//for 3d coords, usually z is 0 coords
	return Vec3f(ppi * winScale * (v.f.f.inFromEnd + robot_coordX), ppi * winScale * (v.f.f.inFromEnd + v.f.f.fieldSizeIn - robot_coordY), robot_coordZ);
}
// Map 4D robot coordinates to screen coordinates.
Rectf VexSimApp::R2S4(float p1X, float p1Y, float p2X, float p2Y) {//for rectangular coords
	return Rectf(ppi * winScale * (v.f.f.inFromEnd + p1X), ppi * winScale * (v.f.f.inFromEnd + v.f.f.fieldSizeIn - p1Y), ppi * winScale * (v.f.f.inFromEnd + p2X), ppi * winScale * (v.f.f.inFromEnd + v.f.f.fieldSizeIn - p2Y));
}
// Draw lines used for debugging robot vertices. edges, and physical features.
void VexSimApp::robotDebug() {
	gl::color(1, 0, 0);
	for (int i = 0; i < 4; i++) {//simplified version of drawing the vertices
		gl::drawSolidCircle(R2S2(v.r[0].db.vertices[i]), 5 + i);
		//else gl::drawSolidCircle(Vec2f(ppi*winScale * v.r[0].db.vertices[i].X, ppi*winScale * v.r[0].db.vertices[i].Y), 5 + i);
	}
	float endInches = v.f.f.inFromEnd*ppi*winScale;
	float offset = endInches + v.f.f.fieldSizeIn*ppi*winScale;//used bc screen coordinate offset thing
															  //vertice rectangles
	gl::drawStrokedRect(Area(endInches + v.r[0].db.vertices[0].X*ppi*winScale, offset - v.r[0].db.vertices[0].Y*ppi*winScale, endInches + v.r[0].db.vertices[1].X*ppi*winScale, offset - v.r[0].db.vertices[1].Y*ppi*winScale));
	gl::drawStrokedRect(Area(endInches + v.r[0].db.vertices[2].X*ppi*winScale, offset - v.r[0].db.vertices[2].Y*ppi*winScale, endInches + v.r[0].db.vertices[3].X*ppi*winScale, offset - v.r[0].db.vertices[3].Y*ppi*winScale));
	//vertical lines
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.vertices[1].X + 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.vertices[1].Y + 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.vertices[2].X - 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.vertices[2].Y - 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.vertices[0].X + 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.vertices[0].Y + 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.vertices[3].X - 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.vertices[3].Y - 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	//horizontal lines
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.vertices[0].X + 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.vertices[0].Y + 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.vertices[1].X - 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.vertices[1].Y - 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.vertices[2].X + 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.vertices[2].Y + 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.vertices[3].X - 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.vertices[3].Y - 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	//for mogo legs
	//for (int mog = 0; mog < 1; mog++) {//used for dual mogo side strat LATER
	for (int i = 0; i < 4; i++) {//simplified version of drawing the MGVert
		gl::drawSolidCircle(R2S2(v.r[0].db.MGVert[i]), 5 + i);
		//else gl::drawSolidCircle(Vec2f(ppi*winScale * v.r[0].db.MGVert[i].X, ppi*winScale * v.r[0].db.MGVert[i].Y), 5 + i);
	}
	//vertice rectangles
	gl::drawStrokedRect(Area(endInches + v.r[0].db.MGVert[0].X*ppi*winScale, offset - v.r[0].db.MGVert[0].Y*ppi*winScale, endInches + v.r[0].db.MGVert[1].X*ppi*winScale, offset - v.r[0].db.MGVert[1].Y*ppi*winScale));
	gl::drawStrokedRect(Area(endInches + v.r[0].db.MGVert[2].X*ppi*winScale, offset - v.r[0].db.MGVert[2].Y*ppi*winScale, endInches + v.r[0].db.MGVert[3].X*ppi*winScale, offset - v.r[0].db.MGVert[3].Y*ppi*winScale));
	//vertical lines
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.MGVert[1].X + 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.MGVert[1].Y + 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.MGVert[2].X - 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.MGVert[2].Y - 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.MGVert[0].X + 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.MGVert[0].Y + 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.MGVert[3].X - 300 * cos((v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.MGVert[3].Y - 300 * sin((v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	//horizontal lines
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.MGVert[0].X + 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.MGVert[0].Y + 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.MGVert[1].X - 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.MGVert[1].Y - 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale));
	gl::drawLine(cinder::Vec2f(endInches + (v.r[0].db.MGVert[2].X + 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
		offset - (v.r[0].db.MGVert[2].Y + 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale),
		cinder::Vec2f(endInches + (v.r[0].db.MGVert[3].X - 300 * sin((-v.r[0].p.mRot) * PI / 180))*ppi*winScale,
			offset - (v.r[0].db.MGVert[3].Y - 300 * cos((-v.r[0].p.mRot) * PI / 180))*ppi*winScale));

	//draw circle
	gl::drawStrokedCircle(Vec2f(endInches + v.r[0].p.position.X*ppi*winScale, offset - v.r[0].p.position.Y*ppi*winScale), renderRad * v.r[0].size*ppi*winScale);
	gl::drawStrokedRect(Area(endInches + v.r[0].db.vertices[0].X*ppi*winScale, offset - v.r[0].db.vertices[0].Y*ppi*winScale, endInches + v.r[0].db.vertices[2].X*ppi*winScale, offset - v.r[0].db.vertices[2].Y*ppi*winScale));

	gl::drawSolidCircle(Vec2f(endInches + ppi*winScale*(v.r[0].p.position.X + (v.r[0].size / 2) * cos((-v.r[0].p.mRot) * PI / 180) * sqrt(2)),
		endInches + v.f.f.fieldSizeIn*ppi*winScale - ppi*winScale*(v.r[0].p.position.Y - (v.r[0].size / 2) * sin((-v.r[0].p.mRot) * PI / 180) * sqrt(2))), 5);
	//draw path lines
	gl::color(1, 1, 1);//resets colours to regular
					   // Draw lines used for debugging robot vertices. edges, and physical features.
}
//drawing front robot cone claw
void VexSimApp::drawClaw(robot *r) {
	gl::draw(v.r[0].CChanel, Area((r->c.size)*ppi*winScale, (r->size*.5 + r->c.baseSize)*ppi*winScale, (-r->c.size)*ppi*winScale, (r->size*.5)*ppi*winScale));
	gl::color(222.0 / 225, 229.0 / 225, 34.0 / 225);
	gl::drawSolidRect(Area(Vec2d((r->c.position + r->c.thickness)*ppi*winScale, (r->size*.5 + r->c.length + r->c.baseSize)*ppi*winScale), Vec2d((r->c.position - r->c.thickness)*ppi*winScale, (r->size*.5 + r->c.baseSize)*ppi*winScale)));
	gl::drawSolidRect(Area(Vec2d((-r->c.position - r->c.thickness)*ppi*winScale, (r->size*.5 + r->c.length + r->c.baseSize)*ppi*winScale), Vec2d((-r->c.position + r->c.thickness)*ppi*winScale, (r->size*.5 + r->c.baseSize)*ppi*winScale)));
	gl::color(1, 1, 1);//reset colour
					   // Draw lines used for debugging robot vertices. edges, and physical features.
}
//drawing the robot and mogo intake
void VexSimApp::drawRobot(robot *r) {
	glPushMatrix();
	///robotDebug(&v, true);
	//had to modify the y because the origin is bottom left hand corner
	gl::translate(Vec3f(R2S3(r->p.position.X, r->p.position.Y, 0.0)));//origin of rotation
	gl::rotate(Vec3f(0, 0, -r->p.mRot - 90));//something for like 3D rotation.... ugh
	gl::color(1, 1, 1);
	if ((r->p.velocity.X != 0 && r->p.velocity.Y != 0) || r->p.rotVel != 0) {//changes with small increments of velocity and rotation
		if ((int)(10 * (r->p.velocity.X + 2 * r->p.rotVel)) % 2 == 0/* || (int)(10 * r->p.rotVel) % 2 == 0*/)
			gl::draw(robot::TankBase, Area((-(r->size / 2))*ppi*winScale, (-(r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale));
		else
			gl::draw(robot::TankBase2, Area((-(r->size / 2))*ppi*winScale, (-(r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale));
	}
	else gl::draw(robot::TankBase, Area((-(r->size / 2))*ppi*winScale, (-(r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale, ((r->size / 2))*ppi*winScale));

	//mogo
	gl::draw(v.r[0].CChanelVERT, Area((-r->mg.position - r->mg.thickness)*ppi*winScale, r->mg.protrusion*ppi*winScale, (-r->mg.position + r->mg.thickness)*ppi*winScale, (r->mg.protrusion + r->mg.length)*ppi*winScale));
	gl::draw(v.r[0].CChanelVERT, Area((r->mg.position + r->mg.thickness)*ppi*winScale, r->mg.protrusion*ppi*winScale, (r->mg.position - r->mg.thickness)*ppi*winScale, (r->mg.protrusion + r->mg.length)*ppi*winScale));
	//draw mogo
	drawClaw(r);
	glPopMatrix();//end of rotation code
}
//overall application drawing function
void VexSimApp::draw() {
	gl::enableAlphaBlending();//good for transparent images
	gl::clear(Color(0, 0, 0));
	//joystick analog drawing
	if (v.recording) {
		int size = 10;//pixels in which to draw the rectangles width
		gl::color(1, 0, 0);
		gl::drawSolidRect(Area(0, 0, size, getWindowHeight()));
		gl::drawSolidRect(Area(0, 0, getWindowWidth(), size));
		gl::drawSolidRect(Area(0, getWindowHeight() - size, getWindowWidth(), getWindowHeight()));
		gl::drawSolidRect(Area(getWindowWidth() - size, 0, getWindowWidth(), getWindowHeight()));
		gl::color(1, 1, 1);//reset to white
	}
	ci::gl::draw(v.f.fieldBare, ci::Area(v.f.f.inFromEnd*ppi*winScale, v.f.f.inFromEnd*ppi*winScale, v.f.f.inFromEnd*ppi*winScale + v.f.f.fieldSizeIn*ppi*winScale, v.f.f.inFromEnd*ppi*winScale + v.f.f.fieldSizeIn*ppi*winScale));
	for (int rob = 0; rob < v.r.size(); rob++) {
		drawRobot(&v.r[rob]);//drawing robot 1
	}
	gl::drawString("Score:", Vec2f(winScale * 850, winScale * 50), Color(1, 1, 1), Font("Arial", winScale * 50));
	drawFontText(v.f.calculateScore(), vec3I(winScale * 1000, winScale * 50), vec3I(1, 1, 1), winScale * 50);
	gl::drawString("Time(s):", Vec2f(winScale * 1350, winScale * 100), Color(1, 1, 1), Font("Arial", winScale * 40));
	drawFontText(ci::app::getElapsedSeconds(), vec3I(winScale * 1480, winScale * 100), vec3I(1, 1, 1), winScale * 40);

	for (int i = 0; i < v.f.mg.size(); i++) {
		vec3 RGB;//true color value because cinder uses values from 0->1 for their colours
		if (v.f.mg[i].colour == 1)/*red mogo*/			RGB = vec3(217, 38, 38);
		else if (v.f.mg[i].colour == 2)/*blue mogo*/	RGB = vec3(0, 64, 255);
		gl::color(RGB.X / 255, RGB.Y / 255, RGB.Z / 255);
		gl::draw(element::MobileGoal, Area(R2S4(
			(v.f.mg[i].pos.X - MGRad),
			(v.f.mg[i].pos.Y - MGRad),
			(v.f.mg[i].pos.X + v.f.mg[i].radius),
			(v.f.mg[i].pos.Y + v.f.mg[i].radius)))
		);
	}
	//drawing each individual cone. oh my
	std::vector<cone> sortedCones(v.f.c);//created copy of v.f.c to not directly affect anything badly
	std::sort(sortedCones.begin(), sortedCones.end(),//lambda function for sorting new cone vector based off pos.Z position
		[](const cone &c1, const cone &c2) {
		return c1.pos.Z < c2.pos.Z;
	});
	for (int i = 0; i < sortedCones.size(); i++) {//find a way to draw based off z distance
		gl::color(1, 1, 1);
		gl::draw(element::coneTexture, Area(R2S4(
			(sortedCones[i].pos.X - sortedCones[i].radius),
			(sortedCones[i].pos.Y - sortedCones[i].radius),
			(sortedCones[i].pos.X + sortedCones[i].radius),
			(sortedCones[i].pos.Y + sortedCones[i].radius)))
		);
	}
	for (int i = 0; i < v.f.mg.size(); i++) {//drawing how many are stacked
		if (v.f.mg[i].stacked.size() > 0) {
			drawFontText(v.f.mg[i].stacked.size(), vec3I(
				(70 + (int)((v.f.mg[i].pos.X - cRad) * (int)ppi)*winScale),
				(50 + (int)((v.f.f.fieldSizeIn - v.f.mg[i].pos.Y + cRad) * (int)ppi)*winScale)),
				vec3I(1, 1, 1), winScale * 50);
		}
	}
	for (int i = 0; i < v.f.pl.size(); i++) {//drawing how many are stacked
		if (v.f.pl[i].stacked.size() > 0) {
			drawFontText(v.f.pl[i].stacked.size(), vec3I(
				(70 + (int)((v.f.pl[i].pos.X - cRad) * (int)ppi)*winScale),
				(50 + (int)((v.f.f.fieldSizeIn - v.f.pl[i].pos.Y + cRad) * (int)ppi)*winScale)),
				vec3I(1, 1, 1), winScale * 50);
		}
	}
	gl::color(1, 1, 1);
	gl::color(1, 0, 0);
	//indicator for cone c.goal
	for (int rob = 1; rob < v.r.size(); rob++) {//not counting c.goal of first robot (manual one)
		gl::drawSolidCircle(R2S2(vec3(v.f.c[v.r[rob].c.goal].pos.X, v.f.c[v.r[rob].c.goal].pos.Y)), winScale * 4);
		for (int i = 0; i < 3; i++) {
			gl::drawStrokedCircle(R2S2(vec3(v.f.c[v.r[rob].c.goal].pos.X, v.f.c[v.r[rob].c.goal].pos.Y)), winScale*v.f.c[v.r[rob].c.goal].radius*ppi + i, 10);
		}
		v.f.c[v.r[rob].c.goal].targetted = true;
	}
	//debug text
	gl::drawSolidCircle(R2S2(vec3(
		v.r[0].p.position.X + v.r[0].mg.protrusion * cos((v.r[0].p.mRot) * PI / 180) * 2,
		v.r[0].p.position.Y + v.r[0].mg.protrusion * sin((v.r[0].p.mRot) * PI / 180) * 2)), winScale * 5);

	//gl::drawSolidCircle(R2S2(vec3(v.f.c[30].closestPoint.X, v.f.c[30].closestPoint.Y)), winScale * 5);
	//	gl::color(0, 1, 0);
	//	gl::drawSolidCircle(R2S2(vec3(v.r[0].db.vertices[v.r[0].closestVertice].X, v.r[0].db.vertices[v.r[0].closestVertice].Y)), winScale * 5);

	//if (v.f.mg[5].inPossession[1]) gl::drawString("YES", Vec2f(1010, 600), Color(1, 1, 1), Font("Arial", 30));
	//else gl::drawString("NO", Vec2f(1010, 600), Color(1, 1, 1), Font("Arial", 30));
	//drawFontText(v.r[0].mg.holding, vec3I(1000, 800), vec3I(0, 1, 0), 30);

	//if (v.r[0].mg.grabbing) gl::drawString("YES", Vec2f(1010, 600), Color(1, 1, 1), Font("Arial", 30));
	//else gl::drawString("NO", Vec2f(1010, 600), Color(1, 1, 1), Font("Arial", 30));
	//drawFontText(v.r[0].d.motorSpeed, vec3I(1010, 660), vec3I(1, 1, 1), 30);
	//	drawFontText(v.f.pl[0].height, vec3I(1010, 500), vec3I(1, 1, 1), 30);
	//	drawFontText(v.r[0].db.rotDist, vec3I(1010, 400), vec3I(1, 1, 1), 30);

	//USER INTERFACE
	gl::color(1, 0, 0);
	if (v.r[0].pathPoints.size() > 1) {
		float endInches = v.f.f.inFromEnd*ppi*winScale;
		float offset = endInches + v.f.f.fieldSizeIn*ppi*winScale;//used bc screen coordinate offset thing

		for (int i = 1; i < v.r[0].pathPoints.size(); i++) {

			gl::drawLine(
				cinder::Vec2f(endInches + v.r[0].pathPoints[i - 1].X*ppi*winScale, offset - v.r[0].pathPoints[i - 1].Y*ppi*winScale),
				cinder::Vec2f(endInches + v.r[0].pathPoints[i].X*ppi*winScale, offset - v.r[0].pathPoints[i].Y*ppi*winScale)
			);
		}
	}

	gl::color(1, 1, 1);
	drawDials(vec3I(1250, 500).times(winScale));
	//buttons::buttonsDraw(100);//size in px
	if (debuggingBotDraw) robotDebug();
	gl::drawString("FPS: ", Vec2f(getWindowWidth() - 150, 30), Color(0, 1, 0), Font("Arial", 30));
	drawFontText(getAverageFps(), vec3I(getWindowWidth() - 90, 30), vec3I(0, 1, 0), 30);
	if (v.debugText) textDraw();//dont run on truspeed sim, unnecessary
}
//awesomesause
CINDER_APP_NATIVE(VexSimApp, RendererGl)