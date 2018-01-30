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
class button {//always going to be 2:1 ratio of W:H
public:
	button(std::function<void()> a, string t, vec3 p, float s, vec3 col) :action(a), text(t), pos(p), size(s), colour(col) {}
	std::function<void()> action;
	void draw();
	//booleans for border outline
	bool hovered = false;
	bool clicked = false;
	//other details
	vec3 colour;//RGB 
	vec3 pos;//upper left values of rectangle
	float size;
	string text;
};
//begin
int tX = 1200;
class VexSimApp : public AppNative {
public:
	VexSimApp() :v(3) {}
	void prepareSettings(Settings *settings);
	void setupButtons();
	void setup();
	void mouseDown(MouseEvent event);
	void mouseUp(MouseEvent event);
	void mouseMove(MouseEvent event);
	void keyDown(KeyEvent event);
	void keyUp(KeyEvent event);
	void update();
	void textDraw();
	void drawDials(vec3I begin);
	static void drawFontText(float text, vec3I pos, vec3I colour, int size);
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
	std::vector<button> b;//all the on screen buttons
private:
	// Change screen resolution
	int mScreenWidth, mScreenHeight;
	float initWidth, initHeight;
	void getScreenResolution(int& width, int& height);
};
void VexSimApp::prepareSettings(Settings *settings){
	//! setup our window
	settings->setTitle("In The Zone Simulation");
	settings->setFrameRate(60);//60fps
	gl::enableVerticalSync();//vsync
	getScreenResolution(mScreenWidth, mScreenHeight);//getss resolution relative to monitor
	settings->setWindowPos(mScreenWidth / 4, mScreenHeight / 6);
	int aspectRatio = mScreenWidth / 8;//using 4/7ths of monitor resolution
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
	setupButtons();
}
//getting screen resolution
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
//setup for on screen buttons
void VexSimApp::setupButtons() {
	//+/- robots
	b.emplace_back([this]() {v.r.emplace_back(); v.f.initialize(&v.r); }, "+bot", vec3(100, 25), 100, vec3(255, 255, 255));
	b.emplace_back([this]() {if (v.r.size() > 1) v.r.pop_back(); }, "-bot", vec3(210, 25), 100, vec3(255, 255, 255));
	//macro recording
	b.emplace_back([this]() {//ON
		v.recording = true;//toggles macro recording
		scriptFile = std::ofstream("script.txt");
	}, "rec", vec3(350, 25), 100, vec3(255, 255, 255));
	b.emplace_back([this]() {//OFF
		v.recording = false;//toggles macro recording
		scriptFile << "driveFor( 0.015);\n";//used for final script(no repeats basically)
		scriptFile = std::ofstream("script.txt", fstream::app);
	}, "stop", vec3(460, 25), 100, vec3(255, 255, 255));
	b.emplace_back([this]() {//READ
		v.r[0].readScript();
	}, "read", vec3(570, 25), 100, vec3(255, 255, 255));
	//reset field
	b.emplace_back([this]() {v.f.initialize(&v.r); }, "reset", vec3(720, 25), 100, vec3(255, 255, 255));
	//start autobots
	b.emplace_back([this]() {//turns on "thinking" for all autobots
		for (int rob = 1; rob < v.r.size(); rob++) {
			if (v.r[rob].thinking != true) v.r[rob].thinking = true;
			else {
				v.r[rob].thinking = false;
				v.r[rob].stopAll();
			}
		}
	}, "autos", vec3(850, 25), 100, vec3(255, 255, 255));
	//debug mode
	b.emplace_back([this]() {debuggingBotDraw = !debuggingBotDraw; }, "debug", vec3(1300, 250), 110, vec3(255, 255, 255));
}
//when mouse is clicked
void VexSimApp::mouseDown(MouseEvent event) {
	if (event.isLeft()) {
		for (int i = 0; i < b.size(); i++) {//checking all buttons
			bool withinXRange = (event.getX() > b[i].pos.X*winScale && event.getX() < b[i].pos.X*winScale + b[i].size*winScale);
			bool withinYRange = (event.getY() > b[i].pos.Y*winScale && event.getY() < b[i].pos.Y*winScale + 0.5*b[i].size*winScale);//height is half of width 
			if (withinXRange && withinYRange) {
				b[i].clicked = true;
				b[i].action();
				break;
			}
		}
	}
}
//when mouse is released
void VexSimApp::mouseUp(MouseEvent event) {
	for (int i = 0; i < b.size(); i++) {
		b[i].clicked = false;
	}
}
//when mouse is moved (used with joystick)
void VexSimApp::mouseMove(MouseEvent event) {
	for (int i = 0; i < b.size(); i++) {//checking all buttons
		bool withinXRange = (event.getX() > b[i].pos.X*winScale && event.getX() < b[i].pos.X*winScale + b[i].size*winScale);
		bool withinYRange = (event.getY() > b[i].pos.Y*winScale && event.getY() < b[i].pos.Y*winScale + 0.5*b[i].size*winScale);//height is half of width 
		if (withinXRange && withinYRange) {
			b[i].hovered = true;//red outline
			break;
		}
		else {
			b[i].hovered = false;//regular outline
		}
	}
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
	winScale = (float)(getWindowWidth() + getWindowHeight() ) / (float)(1*(initWidth + initHeight));//change in window size relative to initial
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
	v.r[0].checkReRunScript();

	v.r[0].db.distance += getSign(v.r[0].d.basePower)*v.r[0].p.position.distance(pastPos);
	v.r[0].db.rotDist += (v.r[0].p.mRot - pastRot);
	
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

void button::draw() {
	//initial main colour
	gl::color(colour.X / 255.0f, colour.Y / 255.0f, colour.Z / 255.0f);//RGB values
	//draws text
	gl::drawString(text, Vec2f(winScale*(pos.X + size*0.25), winScale*(pos.Y + size*0.125)), Color(colour.X / 255.0f, colour.Y / 255.0f, colour.Z / 255.0f), mFont);
	//border outline colours
	if (clicked) {
		gl::color(0, 1, 0);//green outline
	}
	else if (hovered) {
		gl::color(1, 0, 0);//red outline
	}
	//draw rounded rectangle
	gl::drawStrokedRoundedRect(Area(winScale*(pos.X), winScale*(pos.Y), winScale*(pos.X + size), winScale*(pos.Y + size / 2)), 7 * (size / 100));
	gl::color(1, 1, 1);
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
		{ "Angle:", ((v.r[0].p.mRot / 360) - (long)(v.r[0].p.mRot / 360)) * 360 },//only within 360° and -360° (takes the decimal portion and discards the whole number)
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

	gl::color(1, 0, 0);
	if (v.r[0].pathPoints.size() > 1) {//drawing trace lines after auton runs
		float endInches = v.f.f.inFromEnd*ppi*winScale;
		float offset = endInches + v.f.f.fieldSizeIn*ppi*winScale;//used bc screen coordinate offset thing
		for (int i = 1; i < v.r[0].pathPoints.size(); i++) {
			gl::drawLine(
				cinder::Vec2f(endInches + v.r[0].pathPoints[i - 1].X*ppi*winScale, offset - v.r[0].pathPoints[i - 1].Y*ppi*winScale),
				cinder::Vec2f(endInches + v.r[0].pathPoints[i].X*ppi*winScale, offset - v.r[0].pathPoints[i].Y*ppi*winScale)
			);
		}
	}

	//USER INTERFACE
	gl::drawString("Score:", Vec2f(winScale * 935, winScale * 50), Color(1, 1, 1), Font("Arial", winScale * 60));
	drawFontText(v.f.calculateScore(), vec3I(winScale * 1100, winScale * 70), vec3I(1, 1, 1), winScale * 70);
	gl::drawString("Time(s):", Vec2f(winScale * 1300, winScale * 50), Color(1, 1, 1), Font("Arial", winScale * 40));
	drawFontText(ci::app::getElapsedSeconds(), vec3I(winScale * 1430, winScale * 60), vec3I(1, 1, 1), winScale * 50);

	gl::color(1, 1, 1);
	drawDials(vec3I(1250, 500).times(winScale));
	for (int i = 0; i < b.size(); i++) {
		b[i].draw();//draws all buttons
	}
	//buttons::buttonsDraw(100);//size in px
	if (debuggingBotDraw) robotDebug();
	gl::drawString("FPS: ", Vec2f(getWindowWidth() - 150, 30), Color(0, 1, 0), Font("Arial", 30));
	drawFontText(getAverageFps(), vec3I(getWindowWidth() - 90, 30), vec3I(0, 1, 0), 30);
	if (v.debugText) textDraw();//dont run on truspeed sim, unnecessary
}
//awesomesause
CINDER_APP_NATIVE(VexSimApp, RendererGl)
