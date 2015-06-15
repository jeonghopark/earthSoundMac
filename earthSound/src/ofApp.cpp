
#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
    ofEnableAlphaBlending();
    
	ofBackground(ofColor::fromHsb(0, 0, 30, 255));
    
    gui.setup();
    gui.add(mainFq.setup("MainFQ", 0.1, 0, 100));
    gui.add(amountModGui.setup("amount Mod", 1, 0, 20));
    gui.add(amountFQGui.setup("amount FQ", 3.048, 0, 10));
    gui.add(thresholdInput.setup("amount FQ", 17.5, 0, 512));
    gui.add(frameRate.setup("fps", ""));
    
    buttonState = "digital pin:";
    potValue = "analog pin:";
    
	ard.connect("/dev/cu.usbmodem1421", 57600);
	
	// listen for EInitialized notification. this indicates that
	// the arduino is ready to receive commands and it is safe to
	// call setupArduino()
	ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
	bSetupArduino	= false;	// flag so we setup arduino when its ready, you don't need to touch this :)
    
    analogInputA0 = 0;
    
    soundStream.setup(this, 2, 0, 44100, 512, 4);
    
    // Parameter
    ControlParameter triggerPitch = synth.addParameter("triggerPitch");
    ControlParameter amountMod = synth.addParameter("amountMod");
    ControlParameter amountFQ = synth.addParameter("amountFQ");
    ControlParameter envelopTrigger = synth.addParameter("trigger");
    
    // Main Fq
    Generator mainFq = ControlMidiToFreq().input(triggerPitch).smoothed();
    
    // Modulation Fq
    Generator rModFq     = mainFq * amountFQ;
    Generator modulation = SineWave().freq( rModFq ) * rModFq * amountMod;
    
    // Tone Generator
    Generator tone = SineWave().freq(mainFq + modulation);
    
    // Envelop Generator
    Generator env = ADSR().attack(0.001).decay(0.5).sustain(0).release(0).trigger(envelopTrigger).legato(false);
    
    // Output
    synth.setOutputGen( tone * env * 0.75 );

    fullScreenOnOff = false;
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
	updateArduino();
    
    analogInputA0 = ofMap(ard.getAnalog(0), 0, 1023, -ofGetHeight()*0.5, ofGetHeight()*0.5);
    bufferAnalogInputA0.push_back(analogInputA0);
    if(bufferAnalogInputA0.size()>ofGetWidth()*0.5) {
        bufferAnalogInputA0.erase(bufferAnalogInputA0.begin());
    }
    
    float _amountModGuiSensor = abs(ofMap(ard.getAnalog(0), 0, 1023, -30, 30));
    synth.setParameter( "amountMod", _amountModGuiSensor );
    float _amountFQGuiSensor = abs(ofMap(ard.getAnalog(0), 0, 1023, -30, 30));
    synth.setParameter( "amountFQ", _amountFQGuiSensor );
    
    if (bSetupArduino){
        float _inputMiddleValue = abs(ard.getAnalog(0)-512);
        if (_inputMiddleValue>thresholdInput) {
            synth.setParameter("trigger", 1);
            synth.setParameter("triggerPitch", mainFq);
        } else {
            synth.setParameter("trigger", 0);
            synth.setParameter("triggerPitch", mainFq);
        }
    }
    
    frameRate = ofToString(ofGetFrameRate(),1);
}


//--------------------------------------------------------------
void ofApp::draw(){
    
    ofPushMatrix();
    ofPushStyle();
    ofSetColor(255);
    ofFill();
	if (!bSetupArduino){
		font.drawString("arduino not ready...\n", 515, 40);
	}
    
    ofRect(ofGetWidth()*0.5, ofGetHeight()*0.5, 10, analogInputA0*0.5);
    ofPopStyle();
    ofPopMatrix();
    
    ofPushMatrix();
    ofPushStyle();
    
    ofTranslate(0, ofGetHeight()*0.5);
    ofNoFill();
    
	ofSetColor(255);
	ofBeginShape();
	for (int i=0; i<(int)ofGetWidth()*0.5; i++){
		float px = ofMap( i, 0, (ofGetWidth()*0.5-1), 0, ofGetWidth()*0.5 );
		float py = (bufferAnalogInputA0[i])*0.9;
		ofVertex( px, py );
	}
	ofEndShape(false);
    ofPopStyle();
    ofPopMatrix();
    
    gui.draw();
    
}


//--------------------------------------------------------------
void ofApp::setupArduino(const int & version) {
	
	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &ofApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;
    
    // print firmware name and version to the console
    ofLogNotice() << ard.getFirmwareName();
    ofLogNotice() << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion();
    
    // Note: pins A0 - A5 can be used as digital input and output.
    // Refer to them as pins 14 - 19 if using StandardFirmata from Arduino 1.0.
    // If using Arduino 0022 or older, then use 16 - 21.
    // Firmata pin numbering changed in version 2.3 (which is included in Arduino 1.0)
    
    // set pins D2 and A5 to digital input
    ard.sendDigitalPinMode(2, ARD_INPUT);
    ard.sendDigitalPinMode(19, ARD_INPUT);  // pin 21 if using StandardFirmata from Arduino 0022 or older
    
    // set pin A0 to analog input
    ard.sendAnalogPinReporting(0, ARD_ANALOG);
    
    // set pin D13 as digital output
	ard.sendDigitalPinMode(13, ARD_OUTPUT);
    // set pin A4 as digital output
    ard.sendDigitalPinMode(18, ARD_OUTPUT);  // pin 20 if using StandardFirmata from Arduino 0022 or older
    
    // set pin D11 as PWM (analog output)
	ard.sendDigitalPinMode(11, ARD_PWM);
    
    // attach a servo to pin D9
    // servo motors can only be attached to pin D3, D5, D6, D9, D10, or D11
    ard.sendServoAttach(9);
	
    // Listen for changes on the digital and analog pins
    ofAddListener(ard.EDigitalPinChanged, this, &ofApp::digitalPinChanged);
    ofAddListener(ard.EAnalogPinChanged, this, &ofApp::analogPinChanged);
}

//--------------------------------------------------------------
void ofApp::updateArduino(){
    
	// update the arduino, get any data or messages.
    // the call to ard.update() is required
	ard.update();
	
	// do not send anything until the arduino has been set up
	if (bSetupArduino) {
        // fade the led connected to pin D11
		ard.sendPwm(11, (int)(128 + 128 * sin(ofGetElapsedTimef())));   // pwm...
	}
    
}

// digital pin event handler, called whenever a digital pin value has changed
// note: if an analog pin has been set as a digital pin, it will be handled
// by the digitalPinChanged function rather than the analogPinChanged function.

//--------------------------------------------------------------
void ofApp::digitalPinChanged(const int & pinNum) {
    // do something with the digital input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    buttonState = "digital pin: " + ofToString(pinNum) + " = " + ofToString(ard.getDigital(pinNum));
}

// analog pin event handler, called whenever an analog pin value has changed

//--------------------------------------------------------------
void ofApp::analogPinChanged(const int & pinNum) {
    // do something with the analog input. here we're simply going to print the pin number and
    // value to the screen each time it changes
    potValue = "analog pin: " + ofToString(pinNum) + " = " + ofToString(ard.getAnalog(pinNum));
}


//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
    
    if(key=='f') fullScreenOnOff = !fullScreenOnOff;
        
    ofSetFullscreen(fullScreenOnOff);
    
    //    switch (key) {
    //        case OF_KEY_RIGHT:
    //            // rotate servo head to 180 degrees
    //            ard.sendServo(9, 180, false);
    //            ard.sendDigital(18, ARD_HIGH);  // pin 20 if using StandardFirmata from Arduino 0022 or older
    //            break;
    //        case OF_KEY_LEFT:
    //            // rotate servo head to 0 degrees
    //            ard.sendServo(9, 0, false);
    //            ard.sendDigital(18, ARD_LOW);  // pin 20 if using StandardFirmata from Arduino 0022 or older
    //            break;
    //        default:
    //            break;
    //    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    // turn on the onboard LED when the application window is clicked
    //	ard.sendDigital(13, ARD_HIGH);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    // turn off the onboard LED when the application window is clicked
    //	ard.sendDigital(13, ARD_LOW);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::audioRequested (float * output, int bufferSize, int nChannels){

//    for (int i = 0; i < bufferSize; i++){
//        output[i*nChannels    ] = ofMap(ard.getAnalog(0),0,1023,0,1) * 0.9;
//        output[i*nChannels + 1] = ofMap(ard.getAnalog(0),0,1023,0,1) * 0.9;
//    }

    synth.fillBufferOfFloats(output, bufferSize, nChannels);
    
}


//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}