#pragma once

#include "ofMain.h"
#include "ofxQuadWarp.h"
#include "ofxNetwork.h"
#include "ofxBox2d.h"
#include "ElemAnchor.h"
#include "ElemBall.h"
#include "Utils.h"

enum BallEvent{
    NORMAL,
    HUGE_BALL,
    TINY_BALL,
    MULTI_BALL
};

enum WorldEvent{
    NORMAL_WORLD,
    WIND,
    INVERSE_GRAVITY
};

struct PlayerData{
    long lastShot = 0;
    ofVec2f catapultPos;
    ofColor color;
    int score;
    BallEvent nextBall = NORMAL;
    ofTexture texNextBall;
    ofVec2f posTexNextBall;
};

struct TextData {
    string content = "none";
    string font = "none";
    float textLen = 0.f;
    int r = 255,
        g = 255,
        b = 255,
        a = 255;
    int x = 0,
        y = 0;
};

class EntityData {
public:
    string id = "none";
    string type = "none";
    int owner = -1;
};

class JointData {
public:
    string id = "none";
    string idAnchor = "";
    string idBall = "";
    float strenght = 30.f;
};

struct GameEventArgs {
    int id;
    ofVec2f position;
};

enum GameState {
    IDLE,
    START,
    GAME,
    FINISH
};




class ofApp : public ofBaseApp {
  
public:
		
    void setup();
    void setupTexts();
    void update();
    void draw();
    void drawText(string textID);
    void drawWindow2(ofEventArgs& args);
    void drawWindow3(ofEventArgs& args);
    void drawWindow4(ofEventArgs& args);
    void keyPressedWindow2(ofKeyEventArgs& args);
    void keyPressedWindow3(ofKeyEventArgs& args);
    void keyPressedWindow4(ofKeyEventArgs& args);
    void exit();

    void drawScreen(int screenId);
    void processKeyPressedEvent(int key, int screenId);
    void keyPressed(int key);
    void mouseMoved(int x, int y);
    void mousePressed(int x, int y, int button);
    void onGameEvent(GameEventArgs& ev);

    // this is the function for contacts
    void contactStart(ofxBox2dContactArgs& e);
    void contactEnd(ofxBox2dContactArgs& e);

    // game functions
    void createBall(int x, int y, int owner);
    void createAnchor(int x, int y);
    void updateJoints();

    void setState(GameState newState);
    void setNextBall(BallEvent ev);
    void setNextWorldEvent(WorldEvent ev);

    void updateIdle();
    void updateGame();

    void updateWorldEvent();

    void drawPhysicsWorld();
    void drawIdle();
    void drawStart();
    void drawGame();
    void drawFinish();
    void drawSpecialEvents();

    void clearWorld();

    void onNextSpecialEvent();
    void refreshWorldEvents();
    void refreshBallEvents();

    void drawCrosshair(ofVec2f pos, ofColor color);

    ofTexture loadTexture(string path);

    vector<ofxQuadWarp> warper;
    ofJson settings;

    ofxUDPManager udpConnection;
    
    ofImage img;
    ofFbo screen;
    ofFbo overlay;

    ofPoint points[10];

    vector<PlayerData> player;

    int nElems = 0;


    // game
    GameState state = IDLE;
    map<BallEvent,string> ballEvMapping;
    map<WorldEvent,string> worldEvMapping;
    long tStateChanged = 0;

    WorldEvent currentWorldEvent = NORMAL_WORLD;
    long tWorldEvent = 0;
    ofTexture worldEventTexture;
    ofVec2f worldEventTexturePos;
    long tBallEvent = 0;
    vector<BallEvent> ballEventList;
    vector<WorldEvent> worldEventList;

    ofSoundPlayer  sound;
   

    // graphics
    map<string, shared_ptr<ofTrueTypeFont>> fonts;
    map<string, TextData> textData;

    
    ofTrueTypeFont font;    
    float textLen;
    int testNum = 0;

    // box 2d
    ofxBox2d                                    box2d;			  //	the box2d world
    long idCount = 0;

    vector		<shared_ptr<ElemAnchor>>	anchors;		// ball hangings
    vector		<shared_ptr<ElemBall>>	balls;		  //	christmas balls
    vector		<shared_ptr<ofxBox2dJoint>>	    joints;			  //	joints
    map<string, string> jointsToCreate;
    vector<string> jointsToDelete;


};
