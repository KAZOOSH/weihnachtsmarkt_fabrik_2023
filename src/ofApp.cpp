#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSeedRandom();
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    ofSetCircleResolution(200);
    ofTrueTypeFont::setGlobalDpi(72);

    settings = ofLoadJson("settings.json");

    // create the socket
    ofxUDPSettings udpSettings;
    udpSettings.receiveOn(settings["network"]["port"].get<int>());
    udpSettings.blocking = false;
    udpConnection.Setup(udpSettings);

    // init warper
    int w = 0;
    int h = 0;
    int i = 0;
    for (auto &s : settings["screens"])
    {
        w = max(w, s["size"][0].get<int>());
        h += s["size"][1].get<int>();

        int ws = s["size"][0].get<int>();
        int hs = s["size"][1].get<int>();

        warper.push_back(ofxQuadWarp());
        warper.back().setSourceRect(ofRectangle(0, 0, ws, hs));      // this is the source rectangle which is the size of the image and located at ( 0, 0 )
        warper.back().setTopLeftCornerPosition(ofPoint(0, 0));       // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setTopRightCornerPosition(ofPoint(ws, 0));     // this is position of the quad warp corners, centering the image on the screen.
        warper.back().setBottomLeftCornerPosition(ofPoint(0, hs));   // this is position of the quad warp corners, centering the image on the screen.
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
    setupTexts();

    // init player
    player.push_back(PlayerData());
    player.push_back(PlayerData());
    player[0].color = ofColor(settings["style"]["player"]["colors"][0][0].get<int>(),
            settings["style"]["player"]["colors"][0][1].get<int>(),
            settings["style"]["player"]["colors"][0][2].get<int>(),
            settings["style"]["player"]["colors"][0][3].get<int>());
    player[1].color = ofColor(settings["style"]["player"]["colors"][1][0].get<int>(),
            settings["style"]["player"]["colors"][1][1].get<int>(),
            settings["style"]["player"]["colors"][1][2].get<int>(),
            settings["style"]["player"]["colors"][1][3].get<int>());
    player[0].posTexNextBall = ofVec2f(30,30);
    player[1].posTexNextBall = ofVec2f(screen.getWidth()- 30 - 128,30);

    ballEvMapping.insert(make_pair(NORMAL,"normal"));
    ballEvMapping.insert(make_pair(HUGE_BALL,"hugeBall"));
    ballEvMapping.insert(make_pair(TINY_BALL,"tinyBall"));
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
    if (message != "")
    {
        if (message[0] == 'X')
        {
            player[ofToInt(message.substr(2, 1))-1].catapultPos.x = ofToInt(message.substr(4, 3));
        }
        else if (message[0] == 'Y')
        {
            player[ofToInt(message.substr(2, 1))-1].catapultPos.x = ofToInt(message.substr(4, 3));
        }
        else if (message[0] == 'S'){
            int id = ofToInt(message.substr(2, 1))-1;
            if (ofGetElapsedTimeMillis() - player[id].lastShot > 200)
            {
                player[id].lastShot = ofGetElapsedTimeMillis();
                
                GameEventArgs ev;
                ev.id = id;
                ev.position = Utils::convertCatapultToScreenCoords(player[id].catapultPos,ofVec2f(screen.getWidth(),screen.getHeight()));
                onGameEvent(ev);
            }
        }
            
    }

    // box 2d

    box2d.update();

    updateJoints();
    if (state == IDLE)
    {
        updateIdle();
    }
    else if (state == GAME)
    {
        updateGame();
    }

    ofRemove(balls, ofxBox2dBaseShape::shouldRemoveOffScreen);
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofBackground(0);
    ofSetColor(255);

    screen.begin();
    ofClear(0);

    switch (state)
    {
    case IDLE:
        drawIdle();
        break;
    case START:
        drawStart();
        break;
    case GAME:
        drawGame();
        break;
    case FINISH:
        drawFinish();
        break;
    default:
        break;
    }

    ofVec2f screenSize = ofVec2f(screen.getWidth(),screen.getHeight());
    drawCrosshair(Utils::convertCatapultToScreenCoords(player[0].catapultPos,screenSize),
        player[0].color);
    drawCrosshair(Utils::convertCatapultToScreenCoords(player[1].catapultPos,screenSize),
        player[1].color);

    screen.end();

    drawScreen(0);
}

void ofApp::drawWindow2(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(1);
}

void ofApp::keyPressedWindow2(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 1);
}

void ofApp::keyPressedWindow3(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 2);
}

void ofApp::keyPressedWindow4(ofKeyEventArgs &args)
{
    processKeyPressedEvent(args.key, 3);
}

void ofApp::drawWindow3(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(2);
}

void ofApp::drawWindow4(ofEventArgs &args)
{
    ofBackground(0);
    drawScreen(3);
}

void ofApp::exit()
{
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
        dy += settings["screens"][i]["size"][1].get<int>();
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

    for (int i = 0; i < 9; i++)
    {
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
    if (key == 'h' || key == 'H')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].hide();
        }
    }
    if (key == 'd' || key == 'D')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            if (i == screenId)
            {
                warper[i].enableKeyboardShortcuts();
                warper[i].enableMouseControls();
                warper[i].show();
            }
            else
            {
                warper[i].disableKeyboardShortcuts();
                warper[i].disableMouseControls();
                warper[i].hide();
            }
        }
    }

    if (key == 'l' || key == 'L')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].load(ofToString(i), "settings.json");
        }
    }

    if (key == 's' || key == 'S')
    {
        for (size_t i = 0; i < warper.size(); i++)
        {
            warper[i].save(ofToString(i), "settings.json");
        }
    }
    if (key == 'b')
    {
        setNextBall(HUGE_BALL);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    processKeyPressedEvent(key,0);
}

void ofApp::mousePressed(int x, int y, int button)
{
    if (button > 1)
        button = 1;
    if (state != START && state != FINISH){
        createBall(x, y, button);
    }
    
}

void ofApp::mouseMoved(int x, int y)
{
    player[0].catapultPos = ofVec2f(ofMap(x,0,screen.getWidth(),0,100),ofMap(y,0,screen.getHeight(),100,55));
}


void ofApp::onGameEvent(GameEventArgs &ev)
{
    if (state != START && state != FINISH){
        createBall(ev.position.x,ev.position.y,ev.id);
    };
}

ofTexture ofApp::loadTexture(string path)
{
    ofTexture ret;

    // windows path replace
    ofStringReplace(path, "\\", "/");

    ofLoadImage(ret, path);
    return ret;
}

void ofApp::contactStart(ofxBox2dContactArgs &e)
{
    if (e.a != NULL && e.b != NULL)
    {
        // get contacts data
        EntityData *a = (EntityData *)e.a->GetBody()->GetUserData();
        EntityData *b = (EntityData *)e.b->GetBody()->GetUserData();

        // check if both balls from the list
        if (ofIsStringInString(a->id, "b") && ofIsStringInString(b->id, "b"))
        {

            // check if one is anchored
            bool isAnchoredA = false;
            bool isAnchoredB = false;

            JointData *jDataA;
            JointData *jDataB;

            for (auto &j : joints)
            {
                auto *jd = (JointData *)j->joint->GetUserData();
                if (jd->idBall == a->id)
                {
                    isAnchoredA = true;
                    jDataA = jd;
                }
                else if (jd->idBall == b->id)
                {
                    isAnchoredB = true;
                    jDataB = jd;
                }
            }

            // apply forces
            if (isAnchoredA)
            {
                jDataA->strenght -= e.b->GetBody()->GetMass() * e.b->GetBody()->GetLinearVelocity().Length();
                if (jDataA->strenght <= 0)
                {
                    jointsToDelete.push_back(jDataA->id);
                }
            }
            if (isAnchoredB)
            {
                jDataB->strenght -= e.a->GetBody()->GetMass() * e.a->GetBody()->GetLinearVelocity().Length();
                if (jDataB->strenght <= 0)
                {
                    jointsToDelete.push_back(jDataB->id);
                }
            }
        }
    }
}

void ofApp::contactEnd(ofxBox2dContactArgs &e)
{
    if (e.a != NULL && e.b != NULL)
    {
        // get contacts data
        EntityData *a = (EntityData *)e.a->GetBody()->GetUserData();
        EntityData *b = (EntityData *)e.b->GetBody()->GetUserData();

        // check if anchor, and set anchor to a
        bool isAnchor = false;
        if (a->type == "anchor")
        {
            isAnchor = true;
        }
        else if (b->type == "anchor")
        {
            isAnchor = true;
            EntityData *t = a;
            a = b;
            b = t;
        }

        // if anchor check if anchor empty
        if (isAnchor)
        {
            bool isEmpty = true;
            for (auto &j : joints)
            {
                auto *jd = (JointData *)j->joint->GetUserData();
                if (jd->idAnchor == a->id)
                {
                    isEmpty = false;
                }
            }

            // if empty create joint
            if (isEmpty)
            {
                jointsToCreate.insert(make_pair(a->id, b->id));
            }
        }
    }
}

void ofApp::createBall(int x, int y, int owner)
{
    auto c = std::make_shared<ElemBall>();
    auto vals = settings["gameObjects"]["balls"][ballEvMapping[player[owner].nextBall]];
    c->setPhysics(vals["density"], vals["bounce"], vals["friction"]);
    c->setup(box2d.getWorld(), x, y, ofRandom(vals["radius"][0], vals["radius"][1]));
    c->color = player[owner].color;
    c->setData(new EntityData());
    auto *sd = (EntityData *)c->getData();
    sd->id = "b" + ofToString(idCount);
    sd->type = "ball";
    sd->owner = owner;

    balls.push_back(c);
    ++idCount;

    player[owner].nextBall = NORMAL;
    player[owner].texNextBall = ofTexture();
}

void ofApp::createAnchor(int x, int y)
{
    auto c = std::make_shared<ElemAnchor>();
    c->setup(box2d.getWorld(), x, y, 30, true);

    c->setData(new EntityData());
    auto *sd = (EntityData *)c->getData();
    sd->id = "a" + ofToString(idCount);
    sd->type = "anchor";

    anchors.push_back(c);
    ++idCount;
}

void ofApp::updateJoints()
{
    // delete destrye joints
    vector<int> toDel;
    for (auto &id : jointsToDelete)
    {
        for (int i = joints.size() - 1; i >= 0; i--)
        {

            auto *sd = (JointData *)joints[i]->joint->GetUserData();
            if (id == sd->id)
            {
                toDel.push_back(i);
            }
        }
    }
    std::reverse(toDel.begin(), toDel.end());
    for (auto &index : toDel)
    {
        joints[index]->destroy();
        joints.erase(joints.begin() + index);
    }

    // create new joints
    for (auto &p : jointsToCreate)
    {
        shared_ptr<ofxBox2dCircle> bAnchor;
        shared_ptr<ofxBox2dCircle> bBall;

        for (auto &anchor : anchors)
        {
            auto *sd = (EntityData *)anchor->getData();
            if (sd->id == p.first)
            {
                bAnchor = anchor;
            }
        }

        for (auto &ball : balls)
        {
            auto *sd = (EntityData *)ball->getData();
            if (sd->id == p.second)
            {
                bBall = ball;
            }
        }

        auto j = make_shared<ofxBox2dJoint>(box2d.getWorld(), bAnchor->body, bBall->body);
        j->joint->SetUserData(new JointData());
        auto *sd = (JointData *)j->joint->GetUserData();
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
    tStateChanged = ofGetElapsedTimeMillis();

    switch (state)
    {
    case IDLE:
    {

        clearWorld();
        createAnchor(ofGetWidth() * 0.35, ofGetHeight() * 0.5);
        createAnchor(ofGetWidth() * 0.7, ofGetHeight() * 0.5);
        break;
    }
    case START:
    {
        clearWorld();
        for (auto &pos : settings["gameObjects"]["anchors"])
        {
            createAnchor(screen.getWidth() * pos[0].get<float>(), screen.getHeight() * pos[1].get<float>());
        }
        break;
    }
    default:
        break;
    }
}

void ofApp::setNextBall(BallEvent ev)
{
    for(auto& p:player){
        p.nextBall = ev;
        p.texNextBall = loadTexture(settings["gameObjects"]["balls"][ballEvMapping[ev]]["icon"].get<string>());
    }
}

void ofApp::updateIdle()
{
    if (joints.size() == 2)
    {
        int nA = 0;
        int nB = 0;
        for (auto &joint : joints)
        {
            string idBall = ((JointData *)joint->joint->GetUserData())->idBall;
            for (auto &ball : balls)
            {
                if (((EntityData *)ball->getData())->id == idBall)
                {
                    int owner = ((EntityData *)ball->getData())->owner;
                    if (owner == 0)
                    {
                        nA++;
                    }
                    else
                    {
                        nB++;
                    }
                }
            }
        }
        if (nA > 0 && nB > 0)
        {
            setState(START);
        }
    }
}

void ofApp::updateGame()
{
    if (ofGetElapsedTimeMillis() - tStateChanged > settings["gameObjects"]["gameTime"].get<int>() * 1000)
    {
        // end game
        setState(FINISH);
    }

    // update score
    player[0].score = 0;
    player[1].score = 0;
    for (auto &joint : joints)
    {
        string idBall = ((JointData *)joint->joint->GetUserData())->idBall;
        for (auto &ball : balls)
        {
            auto *sd = (EntityData *)ball->getData();
            if (idBall == sd->id)
            {
                sd->owner == 0 ? player[0].score++ : player[1].score++;
            }
        }
    }
}

void ofApp::drawPhysicsWorld()
{
    for (auto &joint : joints)
    {
        ofSetHexColor(0x444342);
        joint->draw();
    }

    for (auto &anchor : anchors)
    {
        bool isUsed = false;
        string id = ((EntityData *)anchor->getData())->id;
        for (auto &joint : joints)
        {
            string idBall = ((JointData *)joint->joint->GetUserData())->idAnchor;
            if (id == idBall)
            {
                isUsed = true;
            }
        }
        if (!isUsed)
        {
            anchor->draw();
        }
    }

    for (auto &circle : balls)
    {
        auto *sd = (EntityData *)circle->getData();
        circle->draw();
    }
}

void ofApp::setupTexts()
{

    TextData data;

    for (auto &text : settings["style"]["text"].items())
    {
        
        font.load(settings["style"]["fonts"][text.value()["font"]]["font"], settings["style"]["fonts"][text.value()["font"]]["size"]);
        textLen = font.stringWidth(text.value()["text"]);

        data.content = text.value()["text"];
        data.font = text.value()["font"];
        data.textLen = textLen;
        
        if (text.value()["align"] == "center") {
            data.x = (screen.getWidth() - textLen) / 2;
        }
        else {
            data.x = text.value()["pos"][0];
        }        
        data.y = text.value()["pos"][1];

        
        if (text.value().contains("color")) {
            data.r = text.value()["color"][0];
            data.g = text.value()["color"][1];
            data.b = text.value()["color"][2];
            data.a = text.value()["color"][3];
        }
               
        textData.emplace(text.key(), data);

        //cout << text << endl;
        
    }    
}

void ofApp::drawText(string textID) 
{
    ofSetColor(textData[textID].r, textData[textID].g, textData[textID].b, textData[textID].a);
    fonts[textData[textID].font]->drawString(textData[textID].content, textData[textID].x, textData[textID].y);
}

void ofApp::drawIdle()
{
    drawPhysicsWorld();  
    drawText("idleTitle");    
}

void ofApp::drawStart()
{
     drawPhysicsWorld();
    
     drawText("startTitle1");
     drawText("startTitle2");

     //fonts["head"]->drawString("Wer am meisten schmückt,", 155, 92);
    //fonts["head"]->drawString("gewinnt!", 155, 150);

    int t = 5000 - (ofGetElapsedTimeMillis() - tStateChanged);

    if (t<1000){
        drawText("startLos");
    }
    else if (t< 4000){
        fonts["timer"]->drawString(ofToString((t/1000)), 500, 700);
    }

    if (ofGetElapsedTimeMillis() - tStateChanged > 5000)
    {
        setState(GAME);
    }

}

void ofApp::drawGame()
{
    drawPhysicsWorld();

    // draw timer
    int tLeft = (settings["gameObjects"]["gameTime"].get<int>() * 1000 - (ofGetElapsedTimeMillis() - tStateChanged)) / 1000;
    int t1 = tLeft / 60;
    int t2 = tLeft % 60;
    string timer = t1 < 10 ? "0" + ofToString(t1) : ofToString(t1);
    timer += ":";
    timer += t2 < 10 ? "0" + ofToString(t2) : ofToString(t2);

    int w = fonts["timer"]->getStringBoundingBox(timer, 0, 0).width;
    fonts["timer"]->drawString(timer, 0.5 * (screen.getWidth() - w), 92);

    // draw score
    int scoreMax = anchors.size();

    int hScore = 45;
    int wMax = (0.9 * screen.getWidth() - w) * 0.5;
    int absScore = player[0].score-player[1].score;
    int y = 75;
    
    if (absScore > 0){
    ofSetColor(player[0].color);

        int w0 = wMax / scoreMax * absScore;
        ofDrawRectangle((screen.getWidth() - w) * 0.5 - w0, y, w0, hScore);
    }else{
    ofSetColor(player[1].color);
        int w1 = wMax / scoreMax * abs(absScore);
        ofDrawRectangle((screen.getWidth() + w) * 0.5, y, w1, hScore);
    }

   for(auto& p:player){
    p.texNextBall.draw(p.posTexNextBall);
   }
}

void ofApp::drawFinish()
{
    if(player[0].score > player[1].score){
        drawText("finishPlayer");
        drawText("finishRot");
        drawText("finishWins");
                 
    }else if(player[0].score < player[1].score){
        drawText("finishPlayer");
        drawText("finishGold");
        drawText("finishWins");
    }else{
        drawText("finishDraw");
    }
    
     if (ofGetElapsedTimeMillis() - tStateChanged > 5000)
    {
        setState(IDLE);
    }
}

void ofApp::clearWorld()
{
    for (auto &ball : balls)
    {
        ball->destroy();
    }
    for (auto &anchor : anchors)
    {
        anchor->destroy();
    }
    /*for (auto& joint : joints) {
        joint->destroy();
    }*/

    balls.clear();
    joints.clear();
    anchors.clear();
}

void ofApp::drawCrosshair(ofVec2f pos, ofColor color)
{
    ofPushStyle();
    ofPushMatrix();
    ofSetColor(color);
    ofTranslate(pos);
    ofDrawCircle(0,0,5);
    ofPopMatrix();
    ofPopStyle();
}
