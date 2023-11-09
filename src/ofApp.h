#pragma once

#include "ofMain.h"
#include "ofxQuadWarp.h"
#include "ofxNetwork.h"
#include "ofxBox2d.h"
#include "ElemAnchor.h"
#include "Utils.h"

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
    string id;
    ofVec2f position;
};

enum GameState {
    IDLE,
    GAME,
    FINISH
};


class ofApp : public ofBaseApp {
  
public:
		
    void setup();
    void update();
    void draw();
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


    void updateIdle();
    void updateGame();

    void drawPhysicsWorld();
    void drawIdle();
    void drawGame();

    void clearWorld();



    ofTexture loadTexture(string path);

    vector<ofxQuadWarp> warper;
    ofJson settings;

    ofxUDPManager udpConnection;
    
    ofImage img;
    ofFbo screen;
    ofFbo overlay;

    ofPoint points[10];


    //catapult 
    int catapultPos = 0;
    ofColor currentColor;

    int nElems = 0;

    long lastshot = 0;


    // game
    GameState state = IDLE;

    // graphics
    map<string, ofTrueTypeFont> fonts;

    // box 2d
    ofxBox2d                                    box2d;			  //	the box2d world
    long idCount = 0;

    vector		<shared_ptr<ElemAnchor>>	anchors;		// ball hangings
    vector		<shared_ptr<ofxBox2dCircle>>	balls;		  //	christmas balls
    vector		<shared_ptr<ofxBox2dJoint>>	    joints;			  //	joints
    map<string, string> jointsToCreate;
    vector<string> jointsToDelete;
};
