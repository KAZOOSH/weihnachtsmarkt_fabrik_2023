#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSeedRandom();
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofEnableSmoothing();

    settings = ofLoadJson("settings.json");


    //create the socket
    ofxUDPSettings udpSettings;
    udpSettings.receiveOn(settings["network"]["port"].get<int>());
    udpSettings.blocking = false;
    udpConnection.Setup(udpSettings);


    // init warper
    int w = 0;
    int h = 0;
    int i = 0;
    for (auto& s : settings["screens"])
    {
        w = max(w, s["size"][0].get<int>());
        h += s["size"][1].get<int>();

        int ws = s["size"][0].get<int>();
        int hs = s["size"][1].get<int>();

        warper.push_back(ofxQuadWarp());
        warper.back().setSourceRect(ofRectangle(0, 0, ws, hs));              // this is the source rectangle which is the size of the image and located at ( 0, 0 )
        warper.back().setTopLeftCornerPosition(ofPoint(0, 0));             // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setTopRightCornerPosition(ofPoint(ws, 0));        // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setBottomLeftCornerPosition(ofPoint(0, hs));      // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setBottomRightCornerPosition(ofPoint(ws, hs)); // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setup();
        warper.back().disableKeyboardShortcuts();
        warper.back().disableMouseControls();
        warper.back().hide();
        warper.back().load(ofToString(i), "settings.json");
        ++i;
    }


    screen.allocate(w, h);
    screen.begin();
    ofClear(0);
    screen.end();

    overlay.allocate(w, h);
    overlay.begin();
    ofClear(0);
    overlay.end();

    // load fonts
    fonts = Utils::loadFonts(settings["style"]["fonts"]);


    /*
    // add objects
    splash.minSize = settings["gameObjects"]["splash"]["sizeMin"];
    splash.maxSize = settings["gameObjects"]["splash"]["sizeMax"];
    for (auto& obj: settings["gameObjects"]["splash"]["src"])
    {
        splash.textures.push_back(loadTexture(obj));
    }
    for (auto& obj : settings["gameObjects"]["splash"]["sound"])
    {
        splash.sounds.push_back(ofSoundPlayer());
        splash.sounds.back().load(obj);
    }

    for (auto& obj: settings["gameObjects"]["specialObjects"])
    {
        specialObjects.push_back(GameObject());
        specialObjects.back().textures.push_back(loadTexture(obj[0]));
        specialObjects.back().sounds.push_back(ofSoundPlayer());
        specialObjects.back().sounds.back().load(obj[1]);
        specialObjects.back().minSize = obj[2];
        specialObjects.back().maxSize = obj[3];

    }


    // grafittiEx
    graffitiExTex = loadTexture(settings["graffitiEx"]["car"]);
    for (auto& p: settings["graffitiEx"]["person"])
    {
        cleaningPersonTex.push_back(loadTexture(p));
    }
    carSound.load(settings["graffitiEx"]["carSound"]);
    cleanerSound.load(settings["graffitiEx"]["cleanerSound"]);
    */


    // init box 2d
    box2d.init();
    box2d.setGravity(0, 9.81);
    box2d.setFPS(60.0);
    box2d.enableEvents();

    // register the listener so that we get the events
    ofAddListener(box2d.contactStartEvents, this, &ofApp::contactStart);
    ofAddListener(box2d.contactEndEvents, this, &ofApp::contactEnd);

    setState(IDLE);
}

//--------------------------------------------------------------
void ofApp::update()
{

    // network
    char udpMessage[100000];
    udpConnection.Receive(udpMessage, 100000);
    string message = udpMessage;
    if (message != "") {
        if (message[0] == 'X') {
            catapultPos = ofToInt(message.substr(1, 3));
        }else if (message[0] == 'Y') {
            if (ofGetElapsedTimeMillis() - lastshot > 1000) {
                lastshot = ofGetElapsedTimeMillis();
                auto mSplit = ofSplitString(message,",");
                auto force = ofToInt(mSplit[0].substr(1, 3));
                cout << mSplit[1] << " " <<ofToInt(mSplit[1].substr(1, 3)) << endl;
                currentColor = ofColor::fromHsb(ofToInt(mSplit[1].substr(0, 3)), 255, 255);
                GameEventArgs ev;

                if (ofRandom(1) <= settings["gameObjects"]["probabilitySpecialObjects"]) {
                    ev.id = "special";
                }
                else {
                    ev.id = "splash";
                }
                

                ev.position = ofVec2f(
                    ofMap(catapultPos, 0, 100,0, screen.getWidth()- settings["gameObjects"]["splash"]["sizeMin"]* screen.getWidth() ),
                    ofMap(force, 55, 100, screen.getHeight(), 0));

                //cout << mSplit[1] << endl;
               // ev.position = ofVec2f(
                //    screen.getWidth()*0.5,screen.getHeight()*0.5);

                onGameEvent(ev);
            }
            else {
                //cout << "double" << endl;
            }
            
        }
    }

    // box 2d
    
    box2d.update();

    updateJoints();
    if (state == IDLE) {
        updateIdle();
    }
    else if (state == GAME) {
        updateGame();
    }
    

    ofRemove(balls, ofxBox2dBaseShape::shouldRemoveOffScreen);
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(0);
    ofSetColor(255);

    screen.begin();
    ofClear(0);

    switch (state)
    {
    case IDLE:
        drawIdle();
        break;
    case GAME:
        drawGame();
        break;
    case FINISH:
        break;
    default:
        break;
    }

    

    screen.end();


    drawScreen(0);
}

void ofApp::drawWindow2(ofEventArgs& args)
{
    ofBackground(0);
    drawScreen(1);
}

void ofApp::keyPressedWindow2(ofKeyEventArgs& args)
{
    processKeyPressedEvent(args.key, 1);
}

void ofApp::keyPressedWindow3(ofKeyEventArgs& args)
{
    processKeyPressedEvent(args.key, 2);
}

void ofApp::keyPressedWindow4(ofKeyEventArgs& args)
{
    processKeyPressedEvent(args.key, 3);
}

void ofApp::drawWindow3(ofEventArgs& args)
{
    ofBackground(0);
    drawScreen(2);
}

void ofApp::drawWindow4(ofEventArgs& args)
{
    ofBackground(0);
    drawScreen(3);
}

void ofApp::exit() {
    for (size_t i = 0; i < warper.size(); i++)
    {
        warper[i].save(ofToString(i), "settings.json");
    }
    
}

void ofApp::drawScreen(int screenId)
{
    auto settingsScreen = settings["screens"][screenId];
    int dy = 0;
    for (size_t i = 0; i < screenId; i++)
    {
        dy += settings["screens"][i]["size"][1];
    }

    
    //======================== get our quad warp matrix.

    ofMatrix4x4 mat = warper[screenId].getMatrix();

    //======================== use the matrix to transform our fbo.

    ofPushMatrix();
    ofMultMatrix(mat);
    
    ofSetColor(255);
    screen.getTexture().drawSubsection(
        ofRectangle(0, 0, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()),
        ofRectangle(0, dy, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()));
    overlay.getTexture().drawSubsection(
        ofRectangle(0, 0, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()),
        ofRectangle(0, dy, settingsScreen["size"][0].get<int>(), settingsScreen["size"][1].get<int>()));
    ofPopMatrix();

    //======================== use the matrix to transform points.

    ofSetLineWidth(2);
    ofSetColor(ofColor::cyan);

    for (int i = 0; i < 9; i++) {
        int j = i + 1;

        ofVec3f p1 = mat.preMult(ofVec3f(points[i].x, points[i].y, 0));
        ofVec3f p2 = mat.preMult(ofVec3f(points[j].x, points[j].y, 0));

        ofDrawLine(p1.x, p1.y, p2.x, p2.y);
    }

    //======================== draw quad warp ui.

    ofSetColor(ofColor::magenta);
    warper[screenId].drawQuadOutline();

    ofSetColor(ofColor::yellow);
    warper[screenId].drawCorners();

    ofSetColor(ofColor::magenta);
    warper[screenId].drawHighlightedCorner();

    ofSetColor(ofColor::red);
    warper[screenId].drawSelectedCorner();
}

void ofApp::processKeyPressedEvent(int key, int screenId)
{
    if (key == 'h' || key == 'H') {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].hide();
        }
    }
    if (key == 'd' || key == 'D') {
        for (size_t i = 0; i < warper.size(); i++)
        {
            if (i == screenId) {
                warper[i].enableKeyboardShortcuts();
                warper[i].enableMouseControls();
                warper[i].show();
            }
            else {
                warper[i].disableKeyboardShortcuts();
                warper[i].disableMouseControls();
                warper[i].hide();
            }
            
        }
    }

    if (key == 'l' || key == 'L') {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].load(ofToString(i), "settings.json");
        }
    }

    if (key == 's' || key == 'S') {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].save(ofToString(i), "settings.json");
        }
    }

    if (key == 'p') {
        currentColor.setHsb(ofRandom(255), 255, 255);
        GameEventArgs e;
        e.id = "splash";
        e.position = ofVec2f(ofRandom(screen.getWidth()), ofRandom(screen.getHeight()));
        onGameEvent(e);
    }
    if (key == 'b') {
        currentColor.setHsb(ofRandom(255), 255, 255);
        GameEventArgs e;
        e.id = "splash";
        e.position = ofVec2f(screen.getWidth()*0.5, 0.5*screen.getHeight());
        onGameEvent(e);
    }

    if (key == 'o') {
        GameEventArgs e;
        e.id = "special";
        e.position = ofVec2f(ofRandom(screen.getWidth()), ofRandom(screen.getHeight()));
        onGameEvent(e);
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    
   
  
}

void ofApp::mousePressed(int x, int y, int button)
{
    createBall(x, y, button);
}

void ofApp::onGameEvent(GameEventArgs& ev)
{
   
}


ofTexture ofApp::loadTexture(string path)
{
    ofTexture ret;

    // windows path replace
    ofStringReplace(path, "\\", "/");

    ofLoadImage(ret, path);
    return ret;
}

void ofApp::contactStart(ofxBox2dContactArgs& e)
{
    if (e.a != NULL && e.b != NULL) {
        // get contacts data
        EntityData* a = (EntityData*)e.a->GetBody()->GetUserData();
        EntityData* b = (EntityData*)e.b->GetBody()->GetUserData();

        // check if both balls from the list
        if (ofIsStringInString(a->id, "b") && ofIsStringInString(b->id, "b")) {

            // check if one is anchored
            bool isAnchoredA = false;
            bool isAnchoredB = false;

            JointData* jDataA;
            JointData* jDataB;

            for (auto& j : joints) {
                auto* jd = (JointData*)j->joint->GetUserData();
                if (jd->idBall == a->id) {
                    isAnchoredA = true;
                    jDataA = jd;
                }
                else if (jd->idBall == b->id) {
                    isAnchoredB = true;
                    jDataB = jd;
                }
            }

            // apply forces
            if (isAnchoredA) {
                jDataA->strenght -= e.b->GetBody()->GetMass() * e.b->GetBody()->GetLinearVelocity().Length();
                if (jDataA->strenght <= 0) {
                    jointsToDelete.push_back(jDataA->id);
                }
            }
            if (isAnchoredB) {
                jDataB->strenght -= e.a->GetBody()->GetMass() * e.a->GetBody()->GetLinearVelocity().Length();
                if (jDataB->strenght <= 0) {
                    jointsToDelete.push_back(jDataB->id);
                }
            }

        }
    }
}

void ofApp::contactEnd(ofxBox2dContactArgs& e)
{
    if (e.a != NULL && e.b != NULL) {
        // get contacts data
        EntityData* a = (EntityData*)e.a->GetBody()->GetUserData();
        EntityData* b = (EntityData*)e.b->GetBody()->GetUserData();

        // check if anchor, and set anchor to a
        bool isAnchor = false;
        if (a->type == "anchor")
        {
            isAnchor = true;
        }
        else if (b->type == "anchor")
        {
            isAnchor = true;
            EntityData* t = a;
            a = b;
            b = t;
        }

        // if anchor check if anchor empty
        if (isAnchor) {
            bool isEmpty = true;
            for (auto& j : joints) {
                auto* jd = (JointData*)j->joint->GetUserData();
                if (jd->idAnchor == a->id) {
                    isEmpty = false;
                }
            }

            // if empty create joint
            if (isEmpty) {
                jointsToCreate.insert(make_pair(a->id, b->id));
            }
        }
    }
}

void ofApp::createBall(int x, int y, int owner)
{
    auto c = std::make_shared<ofxBox2dCircle>();
    c->setPhysics(1, 0.5, 0.9);
    c->setup(box2d.getWorld(), x, y, ofRandom(20, 50));
    c->setData(new EntityData());
    auto* sd = (EntityData*)c->getData();
    sd->id = "b" + ofToString(idCount);
    sd->type = "ball";
    sd->owner = owner;

balls.push_back(c);
++idCount;
}

void ofApp::createAnchor(int x, int y)
{
    auto c = std::make_shared<ElemAnchor>();
    c->setup(box2d.getWorld(), x, y, 30, true);

    c->setData(new EntityData());
    auto* sd = (EntityData*)c->getData();
    sd->id = "a" + ofToString(idCount);
    sd->type = "anchor";

    anchors.push_back(c);
    ++idCount;
}

void ofApp::updateJoints()
{
    // delete destrye joints
    vector<int> toDel;
    for (auto& id : jointsToDelete)
    {
        for (int i = joints.size() - 1; i >= 0; i--)
        {

            auto* sd = (JointData*)joints[i]->joint->GetUserData();
            if (id == sd->id) {
                toDel.push_back(i);
            }
        }
    }
    std::reverse(toDel.begin(), toDel.end());
    for (auto& index : toDel)
    {
        joints[index]->destroy();
        joints.erase(joints.begin() + index);
    }


    // create new joints
    for (auto& p : jointsToCreate)
    {
        shared_ptr<ofxBox2dCircle> bAnchor;
        shared_ptr<ofxBox2dCircle> bBall;

        for (auto& anchor : anchors)
        {
            auto* sd = (EntityData*)anchor->getData();
            if (sd->id == p.first) {
                bAnchor = anchor;
            }
        }

        for (auto& ball : balls)
        {
            auto* sd = (EntityData*)ball->getData();
            if (sd->id == p.second) {
                bBall = ball;
            }
        }

        auto j = make_shared<ofxBox2dJoint>(box2d.getWorld(), bAnchor->body, bBall->body);
        j->joint->SetUserData(new JointData());
        auto* sd = (JointData*)j->joint->GetUserData();
        sd->idAnchor = p.first;
        sd->idBall = p.second;
        sd->id = "j" + ofToString(idCount);
        ++idCount;

        j.get()->jointType = e_ropeJoint;
        j.get()->setLength(50);
        joints.push_back(j);
    }
    jointsToCreate.clear();
}

void ofApp::setState(GameState newState)
{
    state = newState;

    switch (state) {
    case IDLE: {

        clearWorld();
        createAnchor(ofGetWidth() * 0.35, ofGetHeight() * 0.5);
        createAnchor(ofGetWidth() * 0.7, ofGetHeight() * 0.5);
        break;
        }
    case GAME: {
        clearWorld();
        for (auto& pos : settings["gameObjects"]["anchors"]) {
            createAnchor(screen.getWidth() * pos[0].get<float>(), screen.getHeight() * pos[1].get<float>());
        }
        break;
    }
   }
}


void ofApp::updateIdle()
{
    if (joints.size() == 2) {
        int nA = 0;
        int nB = 0;
        for (auto& joint : joints)
        {
            string idBall = ((JointData*)joint->joint->GetUserData())->idBall;
            for (auto& ball : balls)
            {
                if (((EntityData*)ball->getData())->id == idBall){
                    int owner = ((EntityData*)ball->getData())->owner;
                    if (owner == 0) {
                        nA++;
                    }
                    else {
                        nB++;
                    }
                }
                
            }
        }
        if (nA > 0 && nB > 0) {
            setState(GAME);
        }
    }
}

void ofApp::updateGame()
{
    
}

void ofApp::drawPhysicsWorld()
{
    ofSetHexColor(0xf2ab01);
    for (auto& anchor : anchors)
    {
        anchor->draw();
    }


    for (auto& circle : balls) {
        ofFill();
        auto* sd = (EntityData*)circle->getData();

        if (sd->owner == 0) {
            ofSetHexColor(0x01b1f2);
        }
        else {
            ofSetColor(235, 51, 35);
        }


        circle->draw();
    }

    for (auto& joint : joints) {
        ofSetHexColor(0x444342);
        joint->draw();
    }
}

void ofApp::drawIdle()
{
    ofSetColor(255);
    fonts["Header"].drawString("Schmücke die Gestecke", 155, 92);
    
    drawPhysicsWorld();
}

void ofApp::drawGame()
{
    drawPhysicsWorld();
}

void ofApp::clearWorld()
{
    for (auto& ball:balls)
    {
        ball->destroy();
    }
    for (auto& anchor : anchors) {
        anchor->destroy();
    }
    /*for (auto& joint : joints) {
        joint->destroy();
    }*/
    

    balls.clear();
    joints.clear();
    anchors.clear();
}
