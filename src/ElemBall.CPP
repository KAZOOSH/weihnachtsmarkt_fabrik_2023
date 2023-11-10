#include "ElemBall.h"

ElemBall::ElemBall()
{
}

void ElemBall::draw()
{
	if (!isBody()) return;

	ofFill();

	// ball
	ofPushMatrix();
	ofPushStyle();
	ofTranslate(getPosition().x, getPosition().y, 0);
	ofRotateDeg(getRotation(), 0, 0, 1);
	ofEnableAlphaBlending();
	ofSetColor(color);
	ofDrawCircle(0, 0, getRadius());
	ofPopStyle();
	ofPopMatrix();

	// light point
	ofPushMatrix();
	ofPushStyle();
	ofTranslate(getPosition().x+getRadius()*0.3, getPosition().y - getRadius() * 0.3, 0);
	ofEnableAlphaBlending();
	
	ofSetColor(255,250);
	ofDrawCircle(0, 0, getRadius()*0.08);
	ofPopStyle();
	ofPopMatrix();
}
