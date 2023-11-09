#include "ElemAnchor.h"

ElemAnchor::ElemAnchor()
{
}

void ElemAnchor::draw()
{
	if (!isBody()) return;



	ofPushMatrix();
	ofPushStyle();
	ofTranslate(getPosition().x, getPosition().y, 0);
	ofRotateDeg(getRotation(), 0, 0, 1);
	ofEnableAlphaBlending();

	ofSetColor(color.r, color.g, color.b, 100);
	ofDrawCircle(0, 0, getRadius());

	ofSetColor(color);
	float rd = 2 * PI / nDots;
	for (size_t i = 0; i < nDots; i++)
	{
		ofDrawCircle(getRadius() * cos(rd * i), getRadius() * sin(rd * i), rDots);
	}
	

	ofPopStyle();

	ofPopMatrix();
}
