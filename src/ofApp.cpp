#include "ofApp.h"
//--------------------------------------------------------------
void ofApp::setup(){
  ofSetVerticalSync(true);
  ofHideCursor();

  setupDevices();
  initGUI();
  setupVideoProcessing();
  initBox2d();
  initParticleSystem();
  initSound();
  initFlowTools();
}


//--------------------------------------------------------------
void ofApp::update(){
  float dt = 1.0 / max(ofGetFrameRate(), 1.f);
  eTimef = ofGetElapsedTimef();

  //Scene change
  int cond = (int)eTimef % (dSec * sceneNum);
  if (cond < dSec) {
    scene = 0;
  } else if (dSec < cond && cond < dSec * 2) {
    scene = 1;
  } else if (dSec * 2 < cond && cond < dSec * 3){
    scene = 2;
  } else if (dSec * 3 < cond) {
    scene = 3;
  }

  box2d.update();
  vidGrabber.update();

  if (vidGrabber.isFrameNew()) {
    renderImg.setFromPixels(vidGrabber.getPixels());
    renderImg.mirror(false, true);
    colorImg.setFromPixels(renderImg.getPixels());
    grayImg = colorImg;

    if (bLearnBackground) {
      grayBg = grayImg;
      bLearnBackground = false;
    }

    grayDiff.absDiff(grayBg, grayImg);
    grayDiff.threshold(threshold);
    contourFinder.findContours(grayDiff, 20, (colorImg.width*colorImg.height)/ 3, 10, false);

    for (int i = 0; i < contourCircles.size(); i++) {
      contourCircles[i]->destroy();
    }

    contourCircles.clear();
    edgeLines.clear();
    for (int i = 0; i < contourFinder.nBlobs; i++) {
      for (int j = 0; j < contourFinder.blobs[i].pts.size(); j+=4) {
        glm::vec2 pos = contourFinder.blobs[i].pts[j];
        auto circle = make_shared<ofxBox2dCircle>();
        circle->setup(box2d.getWorld(), pos.x, pos.y, 4);
        contourCircles.push_back(circle);
      }
    }

    cameraFbo.begin();
    //vidGrabber.draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
    renderImg.draw(0, 0);
    cameraFbo.end();

    opticalFlow.setInput(cameraFbo.getTexture());
  }

  for(int i = 0; i< contourFinder.nBlobs; i++){
    ofPolyline line;
    for(int j = 0; j < contourFinder.blobs[i].pts.size(); j++){
      line.addVertex(contourFinder.blobs[i].pts[j]);
    }
    edgeLines.push_back(line);
  }

	opticalFlow.update();
	
	velocityBridgeFlow.setVelocity(opticalFlow.getVelocity());
	velocityBridgeFlow.update(dt);
	densityBridgeFlow.setDensity(cameraFbo.getTexture());
	densityBridgeFlow.setVelocity(opticalFlow.getVelocity());
	densityBridgeFlow.update(dt);
	temperatureBridgeFlow.setDensity(cameraFbo.getTexture());
	temperatureBridgeFlow.setVelocity(opticalFlow.getVelocity());
	temperatureBridgeFlow.update(dt);

	fluidFlow.addVelocity(velocityBridgeFlow.getVelocity());
	fluidFlow.addDensity(densityBridgeFlow.getDensity());
	fluidFlow.addTemperature(temperatureBridgeFlow.getTemperature());
  fluidFlow.update(dt);

  float dSum = 0;
  int totalVertices = 1;
  for(int i = 0; i < edgeLines.size(); i++) {
    totalVertices += edgeLines[i].getVertices().size();
    for (int j = 0; j < edgeLines[i].getVertices().size(); j++) {
      float x = edgeLines[i].getVertices()[j].x;
      float y = edgeLines[i].getVertices()[j].y;
      float dist = sqrt(x * x + y * y);
      dSum += dist;
    }
  }
  momentum = (int)dSum / totalVertices;
  momDiff = abs(momentum - lastMomentum);
  lastMomentum = momentum;

  if (eTimef > 1.0 && momDiff > moveThreshold) {
    array<float, 5> a = {0.2, 0.4, 0.6, 0.8, 1.0};
    int idx = (int)ofRandom(0, a.size());
    if (scene == 0) {
      se_harmo.setSpeed(a[idx]);
      se_harmo.play();
    } else if (scene == 1 || scene == 2) {
      se_bass.setSpeed(a[idx]);
      se_bass.play();
    } else if (scene == 3) {
      glitch_bass_g.setSpeed(a[idx]);
      glitch_bass_g.play();
    }
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofBackground(bgCol);
  float hue = abs(sin(eTimef  * 0.2)) * 255;

  ofPushMatrix();
  ofScale((float)ofGetWidth() / (float)colorImg.width, (float)ofGetHeight() / (float)colorImg.height);

  if (showImg) colorImg.draw(0, 0);
  if (scene == 0) {

    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_SCREEN);
    fluidFlow.draw(0, 0, camWidth, camHeight);
    //fluidFlow.drawVelocity(0, 0, camWidth, camHeight);
    ofPopStyle();


  } else if (scene == 1) {
    ofClear(0, 0);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    fluidFlow.draw(0, 0, camWidth, camHeight);
    fluidFlow.drawPressure(0, 0, camWidth, camHeight);
    fluidFlow.drawVelocity(0, 0, camWidth, camHeight);
    ofPopStyle();

  } else if (scene == 2) {
    //fluidFlow.drawVelocity(0, 0, camWidth, camHeight);
    particleSystem.updateMesh();
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_SCREEN);

    ofColor sCol = ofColor(0);
    sCol.setHsb(hue, 255, 255);
    ofSetColor(sCol);

    particleSystem.draw();

    ofColor hCol = ofColor(0);
    hCol.setHsb(abs(int(hue) - 100) % 255, 255, 255);
    ofSetColor(hCol);
    ofSetLineWidth(4.0);
    //for(int i = 0; i < edgeLines.size(); i++) edgeLines[i].draw();
    ofPopStyle();
    /*
    contourFinder.draw();
    for (int i=0; i<contourCircles.size(); i++) contourCircles[i]->draw();
    ofPopMatrix();
    */
  } else if (scene == 3) {
    float cFac = abs(sin(ofGetElapsedTimef()) * 0.8) + 1;
    ofPixels pixels = colorImg.getPixels();
    for (int j = 0; j < camHeight; j+=pixSize) {
      for (int i = 0; i < camWidth; i+=pixSize) {
        float tFac = abs(tan(ofGetElapsedTimef() * (i + j) * 0.002));
        unsigned char r = pixels[(j * camWidth + i) * 3];
        unsigned char g = pixels[(j * camWidth + i) * 3 + 1];
        unsigned char b = pixels[(j * camWidth + i) * 3 + 2];

        ofPushStyle();
        float hue3 = (r + g + b) / 3.0 * cFac;
        ofColor rCol = ofColor(0);
        rCol.setHsb(hue3, hue3, 255);
        ofSetColor(rCol);
        if (rCol.r == 0) {
          ofSetColor(r, g, b);
        }
        ofDrawRectangle(i, j, pixSize*tFac, pixSize*tFac);
        ofPopStyle();

      }
    }
  }
  ofPopMatrix();

  if (debugMode) {
    ofShowCursor();
    ofDrawBitmapStringHighlight("windowWidth: " + ofToString(ofGetWidth()), 10, 20, ofColor::black, ofColor::white);
    ofDrawBitmapStringHighlight("windowHeight: " + ofToString(ofGetHeight()), 10, 40, ofColor::black, ofColor::white);
    ofDrawBitmapStringHighlight("vidWidth: " + ofToString(vidGrabber.getWidth()), 10, 60, ofColor::black, ofColor::white);
    ofDrawBitmapStringHighlight("vidHeight: " + ofToString(vidGrabber.getHeight()), 10, 80, ofColor::black, ofColor::white);
    ofDrawBitmapStringHighlight("momDiff: " + ofToString(momDiff), 10, 100, ofColor::black, ofColor::white);
    gui.draw();
  }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  switch (key) {
    case ' ':
      bLearnBackground = true;
      break;
    case 'd':
      debugMode = !debugMode;
      break;
    case 'g':
      if (hasGravity) {
        box2d.setGravity(0, 0);
      } else {
        box2d.setGravity(0, 2);
      }
      hasGravity = !hasGravity;
      break;
    case 'm':
      scene = (scene + 1) % sceneNum;
      break;
  }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

void ofApp::paramChangedEvent(ofAbstractParameter &e) {
  if (e.getName() == "camID") {
    vidGrabber.close();
    vidGrabber.setDeviceID(camId);
    vidGrabber.initGrabber(640, 480);
  }
}

void ofApp::setupDevices() {
  //Setup Camera Devices
  devices = vidGrabber.listDevices();
  for (int i = 0; i < devices.size(); i++) {
    if (devices[i].bAvailable) {
      ofLogNotice() << devices[i].id << ": " << devices[i].deviceName;
    } else {
      ofLogNotice() << devices[i].id << ": " << devices[i].deviceName << " - unavailable";
    }
  }
  vidGrabber.setDeviceID(1);
  vidGrabber.setup(camWidth, camHeight, true);
}

void ofApp::initGUI() {
    // Init GUI
    gui.setup();
    gui.add(threshold.setup("threshold", 40, 10, 255));
    gui.add(moveThreshold.setup("move threshold", 20, 0, 100));
    gui.add(bgCol.setup("background", 0, 0, 255));
    gui.add(camId.setup("camID", 1, 0, devices.size()));

    ofAddListener(gui.getParameter().castGroup().parameterChangedE(), this, &ofApp::paramChangedEvent);
}

void ofApp::setupVideoProcessing() {
  //Init VideoGrabber
  colorImg.allocate(camWidth ,camHeight);
  grayImg.allocate(camWidth, camHeight);
  grayBg.allocate(camWidth, camHeight);
  grayDiff.allocate(camWidth, camHeight);
  renderImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
  bLearnBackground = true;
  cameraFbo.allocate(camWidth, camHeight);
  ftUtil::zero(cameraFbo);
}

void ofApp::initBox2d() {
  //Init box2d
  box2d.init();
  box2d.setGravity(0, 0);
  box2d.enableEvents();
  box2d.createBounds(0, 0, colorImg.width, colorImg.height);
  box2d.setFPS(30);
  box2d.checkBounds(true);
}

void ofApp::initParticleSystem() {
  //Init particleSystem
  particleSystem.init(box2d.getWorld());
  particleSystem.setMaxParticles(maxParticles);
  particleSystem.setRadius(particleRadius);

  for(int i = 0; i < maxParticles; i++) {
    particleSystem.addParticle(int(colorImg.width / 2) + ofRandom(-200, 200), ofRandom(-50, 150));
  }
  particleSystem.setParticleType(12);
}

void ofApp::initSound() {
  //Init sound
  bgm.load("ambient_bgm_t.wav");
  bgm.setLoop(true);
  bgm.setMultiPlay(true);
  bgm.setVolume(0.8);
  bgm.play();

  se_harmo.load("b_harmo.mp3");
  se_harmo.setVolume(1.2);
  se_harmo.setMultiPlay(true);

  se_bass.load("b_high.mp3");
  se_bass.setVolume(2.0);
  se_bass.setMultiPlay(true);

  glitch_bass_g.load("glitch_bass_g.flac");
  glitch_bass_g.setVolume(0.2);
  glitch_bass_g.setMultiPlay(true);
}

void ofApp::initFlowTools() {
  //Init flowTools
  densityWidth = 640;
  densityHeight = 480;

  simulationWidth = densityWidth / 2;
  simulationHeight= densityHeight / 2;

  windowWidth = ofGetWindowWidth();
  windowHeight = ofGetWindowHeight();

	opticalFlow.setup(simulationWidth, simulationHeight);
	velocityBridgeFlow.setup(simulationWidth, simulationHeight);
	densityBridgeFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);
	temperatureBridgeFlow.setup(simulationWidth, simulationHeight);

	fluidFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);
  fluidFlow.setSpeed(0.02);
  fluidFlow.setDissipationVel(0.2);
  fluidFlow.setDissipationDen(0.8);
  //fluidFlow.setVorticity(0.1);
  fluidFlow.setVorticity(1.0);

	particleFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);

	flows.push_back(&velocityBridgeFlow);
	flows.push_back(&densityBridgeFlow);
	flows.push_back(&temperatureBridgeFlow);
	flows.push_back(&fluidFlow);
	flows.push_back(&particleFlow);

  for (auto flow : flows) {
    flow->setVisualizationFieldSize(glm::vec2(simulationWidth / 2, simulationHeight / 2));
    flow->setVisualizationToggleScalar(true);
  }
}
