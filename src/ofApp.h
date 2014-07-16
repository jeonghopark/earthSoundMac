#pragma once

#include "ofMain.h"
#include "ofEvents.h"

#include "ofxTonic.h"
#include "ofxGui.h"


using namespace Tonic;

class ofApp : public ofBaseApp{

    ofxTonicSynth synth;
    
public:

	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);

	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

    void audioRequested(float * output, int bufferSize, int nChannels);
    ofSoundStream soundStream;
    
	ofImage				bgImage;
	ofTrueTypeFont		font;
    ofTrueTypeFont      smallFont;
	ofArduino	ard;
	bool		bSetupArduino;			// flag variable for setting up arduino once
    
    float analogInputA0;
    vector<float> bufferAnalogInputA0;
    int counter;
    
    ofxPanel gui;
    ofxFloatSlider amountModGui;
    ofxFloatSlider amountFQGui;
    ofxFloatSlider thresholdInput;
    ofxLabel viewText;


private:
    
    void setupArduino(const int & version);
    void digitalPinChanged(const int & pinNum);
    void analogPinChanged(const int & pinNum);
	void updateArduino();
    
    string buttonState;
    string potValue;

};

