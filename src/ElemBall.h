#pragma once
#include "ofxBox2dCircle.h"
class ElemBall :
    public ofxBox2dCircle
{
public:
    ElemBall();

    void draw();

    ofColor color = ofColor(200);
    int nDots = 20;
    int rDots = 2;
};

