#pragma once
#include "ofxBox2dCircle.h"
class ElemAnchor :
    public ofxBox2dCircle
{
public:
    ElemAnchor();

    void draw();

    ofColor color = ofColor(200);
    int nDots = 20;
    int rDots = 2;
};

