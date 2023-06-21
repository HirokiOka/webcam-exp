#pragma once

#include "ofMain.h"
#include "ofxBox2d.h"
#include "ofxBox2dParticleSystem.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "ofxFlowTools.h"

using namespace ofxBox2dParticleSystem;
using namespace flowTools;

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		ofxBox2d box2d;
    vector <shared_ptr<ofxBox2dCircle>> contourCircles;
    ParticleSystem particleSystem;

    ofVideoGrabber vidGrabber;
    vector<ofVideoDevice> devices;
    ofxCvColorImage colorImg;
    ofxCvGrayscaleImage grayImg;
    ofxCvGrayscaleImage grayBg;
    ofxCvGrayscaleImage grayDiff;
    ofImage renderImg;
    ofxCvContourFinder contourFinder;

    bool hasGravity = false;
    bool debugMode = false;
    bool bLearnBackground;
    float eTimef = 0.0;

    ofxFloatSlider threshold;
    ofxFloatSlider moveThreshold;
    ofxFloatSlider bgCol;
    ofxIntSlider pixSize;
    ofxPanel gui;
    ofxButton showImg;

    vector		<shared_ptr<ofxBox2dCircle> >	circles;
    vector <ofPolyline> edgeLines;

    int camWidth = 640;
    int camHeight = 480;
    //int cropW = 270;  //if camWidth is 320, 135
    int cropW = 270;

    int maxParticles = 45000;
    int particleRadius = 2;

    int intervalSec = 30;

    //sound
    ofSoundPlayer bgm;
    ofSoundPlayer se_harmo;
    ofSoundPlayer se_bass;
    ofSoundPlayer glitch_bass_g;

    //FlowToolsConfig
    ofFbo cameraFbo;

    vector< ftFlow* >		flows;
    ftOpticalFlow			opticalFlow;
    ftVelocityBridgeFlow	velocityBridgeFlow;
    ftDensityBridgeFlow		densityBridgeFlow;
    ftTemperatureBridgeFlow temperatureBridgeFlow;
    ftFluidFlow				fluidFlow;
    ftParticleFlow			particleFlow;

    float deltaTime;

    int windowWidth, windowHeight, densityWidth, densityHeight, simulationWidth, simulationHeight;
    int sceneNum = 4;
    int scene = 0;
    int dSec = 30;
    int momentum = 0;
    int lastMomentum = 0;
    int momDiff = 0;
};
