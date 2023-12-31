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

    screen.allocate(w, h, GL_RGBA32F_ARB);
    screen.begin();
    ofClear(0);
    screen.end();

    treeBg.allocate(w, h, GL_RGBA32F_ARB);
    treeBg.begin();
    ofClear(0);
    treeBg.end();

    // load fonts & colors
    fonts = Utils::loadFonts(settings["style"]["fonts"]);
    setupColors();
    setupTexts();

    setupTreeData(settings);
    treeGraphic.load("img/xmasTree.png");

    // setup score    
    int gap = settings["gameObjects"]["score"]["gap"];
    float scorePosX = settings["gameObjects"]["score"]["pos"][0];
    float scorePosY = settings["gameObjects"]["score"]["pos"][1];
    float scoreSize = settings["gameObjects"]["score"]["size"];

    for (int i = 0; i < 10 ; i++) {
        ScoreData scorePoint;
        scorePoint.size = scoreSize;
        scorePoint.pos = ofVec2f(scorePosX + (i * (gap + (scoreSize * 2))), scorePosY);
        scorePoint.color = appColors["scorebase"];
        scoreData.push_back(scorePoint);
    }


    
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
    player[0].posTexNextBall = ofVec2f(130,1450);
    player[1].posTexNextBall = ofVec2f(screen.getWidth()- 80 - 128,1450);

    player[0].soundWin.loadSound(settings["gameObjects"]["sounds"]["p1Win"]);
    player[1].soundWin.loadSound(settings["gameObjects"]["sounds"]["p2Win"]);

    ballEvMapping.insert(make_pair(NORMAL,"normal"));
    ballEvMapping.insert(make_pair(HUGE_BALL,"hugeBall"));
    ballEvMapping.insert(make_pair(TINY_BALL,"tinyBall"));
    ballEvMapping.insert(make_pair(MULTI_BALL,"multiBall"));

    worldEvMapping.insert(make_pair(NORMAL_WORLD,"normal"));
    worldEvMapping.insert(make_pair(INVERSE_GRAVITY,"inverseGravity"));
    //worldEvMapping.insert(make_pair(WIND,"wind"));
    
   // sounds.insert(make_pair("contact",ofSoundPlayer()));
    //sounds.insert(make_pair("start",ofSoundPlayer()));
    sounds["contact"].loadSound(settings["gameObjects"]["sounds"]["ball"]);
    sounds["contact"].setMultiPlay(true);
    sounds["start"].loadSound(settings["gameObjects"]["sounds"]["start"]);
    sounds["anchor"].loadSound(settings["gameObjects"]["sounds"]["anchor"]);
    sounds["anchor"].setMultiPlay(true);

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
        if (isDebug) {
            cout << message << endl;
        }
        if (message[0] == 's' || message[0] == 'S' ){
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
        else if (message[0] == 'X')
        {
            player[ofToInt(message.substr(2, 1))-1].catapultPos.x = ofToInt(message.substr(4, 3));
        }
        else if (message[0] == 'Y')
        {
            player[ofToInt(message.substr(2, 1))-1].catapultPos.y = ofToInt(message.substr(4, 3));
        }
        else if (message[0] == 'b' || message[0] == 'B')
        {
            onNextSpecialEvent();
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
    //cout << player[0].catapultPos << " " << player[1].catapultPos <<endl;
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
            isDebug = false;
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
                isDebug = true;
            }
            else
            {
                warper[i].disableKeyboardShortcuts();
                warper[i].disableMouseControls();
                warper[i].hide();
                isDebug = false;
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
        if (state == GAME){
            onNextSpecialEvent();
        }
        
    }
    if (key == 'r')
    {
        settings = ofLoadJson("settings.json");
        setupColors();
        setupTexts();
        setupTreeData(settings);
    }
    if (key == 'f') {
        ofToggleFullscreen();
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
   //player[0].catapultPos = ofVec2f(ofMap(x,0,screen.getWidth(),0,100),ofMap(y,0,screen.getHeight(),100,35));
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
            sounds["contact"].play();

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
            sounds["anchor"].play();
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

    if (player[owner].nextBall == MULTI_BALL){
        for (size_t i = 0; i < 2; i++)
        {
            auto c = std::make_shared<ElemBall>();
            auto vals = settings["gameObjects"]["balls"][ballEvMapping[player[owner].nextBall]];
            c->setPhysics(vals["density"], vals["bounce"], vals["friction"]);
            c->setup(box2d.getWorld(), x +((i+1)%2)* vals["radius"][1].get<float>()*1.3, y+(i%2)* vals["radius"][1].get<float>()*1.3, ofRandom(vals["radius"][0], vals["radius"][1]));
            c->color = player[owner].color;
            c->setData(new EntityData());
            auto *sd = (EntityData *)c->getData();
            sd->id = "b" + ofToString(idCount);
            sd->type = "ball";
            sd->owner = owner;
            balls.push_back(c);
            ++idCount;
        }
        for (size_t i = balls.size()-2; i < balls.size(); i++)
        {
            int t = i+1;
            if(t == balls.size()){
                t = balls.size()-2;
            }
            auto j = make_shared<ofxBox2dJoint>(box2d.getWorld(), balls[i]->body, balls[t]->body);
            j->joint->SetUserData(new JointData());
            /*auto *sd = (JointData *)j->joint->GetUserData();
            sd->idAnchor = p.first;
            sd->idBall = p.second;
            sd->id = "j" + ofToString(idCount);
            ++idCount;*/
            j.get()->jointType = e_ropeJoint;
            j.get()->setLength(50);
        }
        
        

        
    }

    

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
        createAnchor(ofGetWidth() * 0.35, ofGetHeight() * 0.65);
        createAnchor(ofGetWidth() * 0.65, ofGetHeight() * 0.65);
        break;
    }
    case START:
    {
        clearWorld();
        for (auto &pos : settings["gameObjects"]["anchors"])
        {
            createAnchor(screen.getWidth() * pos[0].get<float>(), screen.getHeight() * pos[1].get<float>());
        }
        sounds["start"].play();
        break;
        
    }
    case FINISH:
    {
         if (player[1].score < player[0].score) {
            player[0].soundWin.play();
        }
        else if (player[1].score > player[0].score ) {
            player[1].soundWin.play();
        }
        else {
            //draw
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
        sound.load(settings["gameObjects"]["balls"][ballEvMapping[ev]]["sound"].get<string>());
        sound.play();
    }
    tBallEvent = ofGetElapsedTimeMillis();
}

void ofApp::setNextWorldEvent(WorldEvent ev)
{
    currentWorldEvent = ev;
    tWorldEvent = ofGetElapsedTimeMillis();

    switch (currentWorldEvent)
    {
    case NORMAL_WORLD:
        box2d.setGravity(0, 9.81);
        break;
    case INVERSE_GRAVITY:
        box2d.setGravity(0, -9.81);
        break;
    default:
        break;
    }

    sound.load(settings["gameObjects"]["worldEvents"][worldEvMapping[ev]]["sound"].get<string>());
    sound.play();
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

   updateWorldEvent();

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

void ofApp::updateWorldEvent()
{
    if(currentWorldEvent != NORMAL_WORLD){

        switch (currentWorldEvent)
        {
       
        
        default:
            break;
        }
        if (ofGetElapsedTimeMillis() - tWorldEvent > settings["gameObjects"]["worldEventTime"].get<int>()){
            setNextWorldEvent(NORMAL_WORLD);
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
            data.color = appColors[text.value()["color"]];
        }
               
        textData.emplace(text.key(), data);        
    }    
}

void ofApp::setupColors()
{
    ofColor color;

    for (auto c : settings["style"]["colors"].items())
    {
        color.r = c.value()[0];
        color.g = c.value()[1];
        color.b = c.value()[2];
        color.a = c.value()[3];
        appColors[c.key()] = color;
        //appColors.emplace(c.key(), color);
    }
}

void ofApp::setupTreeData(ofJson s)
{
    // setup tree
    xmasTree.color = ofColor(appColors["tree"]);
    xmasTree.top = ofVec2f(settings["tree"]["pos"]["top"][0], settings["tree"]["pos"]["top"][1]);
    xmasTree.right = ofVec2f(settings["tree"]["pos"]["right"][0], settings["tree"]["pos"]["right"][1]);
    xmasTree.left = ofVec2f(settings["tree"]["pos"]["left"][0], settings["tree"]["pos"]["left"][1]);
    xmasTree.trunk = ofColor(appColors["trunk"]);
    xmasTree.topTrunk = ofVec2f(settings["tree"]["trunkPos"]["top"][0], settings["tree"]["trunkPos"]["top"][1]);
    xmasTree.rightTrunk = ofVec2f(settings["tree"]["trunkPos"]["right"][0], settings["tree"]["trunkPos"]["right"][1]);
    xmasTree.leftTrunk = ofVec2f(settings["tree"]["trunkPos"]["left"][0], settings["tree"]["trunkPos"]["left"][1]);
    xmasTree.triangles = settings["tree"]["triangles"];
}

void ofApp::drawText(string textID) 
{
    ofSetColor(textData[textID].color);
    fonts[textData[textID].font]->drawString(textData[textID].content, textData[textID].x, textData[textID].y);
}

void ofApp::drawTree(TreeData tree)
{   
    ofSetColor(tree.trunk);
    ofDrawTriangle(tree.topTrunk[0], tree.topTrunk[1], tree.rightTrunk[0], tree.rightTrunk[1], tree.leftTrunk[0], tree.leftTrunk[1]);

    
    int hTree = tree.left[1] - tree.top[1];
    int wTree = tree.right[0] - tree.left[0];
    int xCenter = wTree*0.5 + tree.left[0];

    treeBg.begin();
    ofClear(0);
    ofPushStyle();
    ofFill();
    //ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    for (auto& tr:tree.triangles)
    {// fix draw function
        
        ofSetColor(appColors[tr["color"]]);
        //ofSetColor(255);
        int py = tr["pos"][1].get<float>() * hTree +tree.top[1] ;
        int height = tr["pos"][2].get<float>() * hTree;
        int wMax = tr["pos"][1].get<float>()*wTree;
        int pxMin = xCenter - wMax*0.5;
        int pxMax = xCenter + wMax*0.5;
        int px = ofMap(tr["pos"][0].get<float>(),0,1,pxMin,pxMax);
        int wSide = height*2/sqrt(3)/2;
        ofDrawTriangle(px,py,px-wSide,py+height,px+wSide,py+height);
    }
    

    ofPopStyle();
    treeBg.end();
    

    ofSetColor(255);
    ofFill();
    //ofDrawTriangle(tree.top[0], tree.top[1], tree.right[0], tree.right[1], tree.left[0], tree.left[1]);
    treeBg.draw(0,0);    

    //treeGraphic.draw(xmasTree.left[0] - 60, xmasTree.top[1] - 10, 1020, 1224);    
}

void ofApp::drawIdle()
{
    drawTree(xmasTree);

    drawPhysicsWorld(); 

    drawText("0_idleTitle1");
    drawText("0_idleTitle2");
    
}

void ofApp::drawStart()
{
    drawTree(xmasTree);

    drawPhysicsWorld();
        
    drawText("1_startTitle1");
    drawText("1_startTitle2");

    int t = 5000 - (ofGetElapsedTimeMillis() - tStateChanged);

    if (t<1000){
        drawText("1_startLos");
    }
    else if (t< 4000){
        //ofSetColor(textData[textID].r, textData[textID].g, textData[textID].b, textData[textID].a);
        //fonts[textData[textID].font]->drawString(textData[textID].content, textData[textID].x, textData[textID].y);
        fonts["timer"]->drawString(ofToString((t/1000)), 510, 1635);
    }

    if (ofGetElapsedTimeMillis() - tStateChanged > 5000)
    {
        setState(GAME);
    }

}

void ofApp::drawGame()
{    
    drawTree(xmasTree);
    
    drawPhysicsWorld();

    // draw timer
    ofSetColor(appColors[settings["gameObjects"]["timerColor"]]);
    int tLeft = (settings["gameObjects"]["gameTime"].get<int>() * 1000 - (ofGetElapsedTimeMillis() - tStateChanged)) / 1000;
    int t1 = tLeft / 60;
    int t2 = tLeft % 60;
    string timer = t1 < 10 ? "0" + ofToString(t1) : ofToString(t1);
    timer += ":";
    timer += t2 < 10 ? "0" + ofToString(t2) : ofToString(t2);

    int w = fonts["timer"]->getStringBoundingBox(timer, 0, 0).width;
    fonts["timer"]->drawString(timer, 0.5 * (screen.getWidth() - w), 1635);

    // draw score
    /*
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
    */

    for (int i = 0; i < scoreData.size(); i++) {
        int noScore = scoreData.size() - player[0].score - player[1].score;
        if (i < player[0].score) {
            scoreData[i].color = player[0].color;
        }
        else if (i < player[0].score + noScore) {
            scoreData[i].color = appColors["scoreBase"];
        }
        else {
            scoreData[i].color = player[1].color;
        }        
    }
    
    for (auto& sp : scoreData) {
        ofSetColor(sp.color);
        ofDrawCircle(sp.pos[0], sp.pos[1], sp.size);
    }

   for(auto& p:player){
    ofSetColor(p.color);
    if(p.texNextBall.isAllocated()){
        p.texNextBall.draw(p.posTexNextBall);
    }
   }

   ofSetColor(255);
   if(worldEventTexture.isAllocated()){
    worldEventTexture.draw(worldEventTexturePos);
   }

   drawSpecialEvents();

}

void ofApp::drawFinish()
{
    if(player[0].score > player[1].score){
        drawText("2_finishPlayer");
        drawText("2_finishRot");
        drawText("2_finishWins");
                 
    }else if(player[0].score < player[1].score){
        drawText("2_finishPlayer");
        drawText("2_finishGold");
        drawText("2_finishWins");
    }else{
        drawText("2_finishDraw");
    }
    
     if (ofGetElapsedTimeMillis() - tStateChanged > 5000)
    {
        setState(IDLE);
    }
}

void ofApp::drawSpecialEvents()
{
    

    if (ofGetElapsedTimeMillis()-tWorldEvent < settings["style"]["tEvent"].get<float>()*1000){
        float p = float(ofGetElapsedTimeMillis()-tWorldEvent) /float(settings["style"]["tEvent"].get<float>()*1000);
        ///currentWorldEvent -> draw
        textData["HELIUM"].color[3] = alpha;
        drawText("HELIUM");
        
        alpha = 255 * sin(p * PI);
        /*
        
        if (alpha < 55) {
            alpha += 50;
        }
        else if (alpha > 200) {
            alpha -= 50;
        };*/

        ofPushMatrix();
        //ofScale(scaleSize);
        ofRotateXDeg(90);
        
        ofPopMatrix();
    }
    if (ofGetElapsedTimeMillis()-tBallEvent < settings["style"]["tEvent"].get<float>()*1000){
        float p = float(ofGetElapsedTimeMillis()-tBallEvent) /float(settings["style"]["tEvent"].get<float>()*1000);
        //player[0].nextBall
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

void ofApp::onNextSpecialEvent()
{
    if(ballEventList.size() == 0){
        refreshBallEvents();
    }
    if(worldEventList.size() == 0){
        refreshWorldEvents();
    }

    bool isWorldNormal = false;
    if(currentWorldEvent == NORMAL_WORLD){
        isWorldNormal = true;
    };
    bool isBallNormal = false;
    if( player[0].nextBall == NORMAL &&
    player[1].nextBall == NORMAL){
        isBallNormal = true;
    }

    if (isWorldNormal && isBallNormal){
        if(ofRandom(1.0)> 0.5){
            setNextBall(ballEventList.back());
            ballEventList.pop_back();
        }else{
            setNextWorldEvent(worldEventList.back());
            worldEventList.pop_back();
        }
    }else if (isBallNormal){
        setNextBall(ballEventList.back());
        ballEventList.pop_back();
    }else if (isWorldNormal){
        setNextWorldEvent(worldEventList.back());
            worldEventList.pop_back();
    }else{
        if(ofRandom(1.0)> 0.5){
            setNextBall(ballEventList.back());
            ballEventList.pop_back();
        }else{
            setNextWorldEvent(worldEventList.back());
            worldEventList.pop_back();
        }
    }
}

void ofApp::refreshWorldEvents()
{
    for (auto& p:worldEvMapping){
        if(p.first!=NORMAL_WORLD){
            worldEventList.push_back(p.first);
        }
        
    }
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(worldEventList), std::end(worldEventList), rng);
}

void ofApp::refreshBallEvents()
{
    for (auto& p:ballEvMapping){
        if(p.first!=NORMAL){
            ballEventList.push_back(p.first);
        }
        
    }
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(ballEventList), std::end(ballEventList), rng);
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
