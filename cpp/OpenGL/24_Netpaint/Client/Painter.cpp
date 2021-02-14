#include "Painter.h"

MyGLApp::MyGLApp() : GLApp(1024, 786, 4, "Network Painter") {}

Vec3 MyGLApp::convertPosToHSV(float x, float y) {
  return Vec3::hsvToRgb({360*x,y,value});
}

void MyGLApp::fillHSVImage() {
  for (uint32_t y = 0;y<hsvImage.height;++y) {
    for (uint32_t x = 0;x<hsvImage.width;++x) {
      const Vec3 rgb = convertPosToHSV(float(x)/hsvImage.width, float(y)/hsvImage.height);
      hsvImage.setNormalizedValue(x,y,0,rgb.x());
      hsvImage.setNormalizedValue(x,y,1,rgb.y());
      hsvImage.setNormalizedValue(x,y,2,rgb.z());
      hsvImage.setValue(x,y,3,255);
    }
  }
}

void MyGLApp::tryToLoadSettings() {
  std::ifstream settings ("settings.txt");
  std::string line;
  if (settings.is_open()) {
    if (getline(settings,line) ) {
      serverAddress = line;
      addressComplete = true;
    } 
    if (getline(settings,line) ) {
      userName = line;
      nameComplete = true;
    }
    settings.close();
  }
}

void MyGLApp::genMouseCursor() {
  cursorShape = Image(64,64,4);

  for (uint32_t y = 0;y<cursorShape.height;++y) {
    for (uint32_t x = 0;x<cursorShape.width;++x) {
              
      Vec2 nPos{x / float(cursorShape.width), y / float(cursorShape.width)};
      
      float val = std::max(0.0f,(0.5f-(Vec2(0.5f,0.5f)-nPos).length())*2.0f);
      
      const uint8_t r{val > 0.9 ? uint8_t(0) : uint8_t(val*255)};
      const uint8_t g{val > 0.9 ? uint8_t(0) : uint8_t(val*255)};
      const uint8_t b{val > 0.9 ? uint8_t(0) : uint8_t(val*255)};
      const uint8_t a{uint8_t(val*255)};
      
      cursorShape.setValue(x,y,0,r);
      cursorShape.setValue(x,y,1,g);
      cursorShape.setValue(x,y,2,b);
      cursorShape.setValue(x,y,3,a);
    }
  }
}

void MyGLApp::init() {
  tryToLoadSettings();

  hsvImage = Image(255,255);
  fillHSVImage();
  genMouseCursor();
  
  glEnv.setCursorMode(CursorMode::HIDDEN);
  GL(glEnable(GL_BLEND));
  GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL(glBlendEquation(GL_FUNC_ADD));
  GL(glClearColor(0.0f,0.0f,0.0f,0.0f));
}

void MyGLApp::updateMousePos() {
  Dimensions s = glEnv.getWindowSize();
  normPos = Vec2{float(xPositionMouse/s.width)-0.5f,float(1.0-yPositionMouse/s.height)-0.5f} * 2.0f;
  
  if (colorChooserMode)
    normPos = (Mat4::inverse(baseTransformation) * Vec4{normPos,0.0f,1.0f}).xy();
  else
    normPos = (Mat4::inverse(userTransformation*baseTransformation) * Vec4{normPos,0.0f,1.0f}).xy();
}

void MyGLApp::addTransformation(const Mat4& trafo) {
  userTransformation = trafo * userTransformation;
  updateMousePos();
}

Vec2i MyGLApp::computePixelPos() {
  return Vec2i{int((normPos.x()/2.0f+0.5f)*imageSize.x()),int((normPos.y()/2.0f+0.5f)*imageSize.y())};
}

void MyGLApp::dropPaint() {
  if (client) client->paint(computePixelPos());
}

Mat4 MyGLApp::computeBaseTransform(const Vec2ui& imageSize) {
  const Dimensions s = glEnv.getWindowSize();
  const float ax = imageSize.x()/float(s.width);
  const float ay = imageSize.y()/float(s.height);
  const float m = std::max(ax,ay);
  return Mat4::scaling({ax/m, ay/m, 1.0f});
}
  
void MyGLApp::mouseMove(double xPosition, double yPosition) {
  if (!client) return;
  
  Dimensions s = glEnv.getWindowSize();
  if (xPosition < 0 || xPosition > s.width || yPosition < 0 || yPosition > s.height) return;
  
  xPositionMouse = xPosition;
  yPositionMouse = yPosition;
  updateMousePos();

  if (colorChooserMode) {
    if (rightMouseDown) client->setColor( Vec4{Vec3::hsvToRgb({360*(normPos.x()+1.0f)/2.0f,(normPos.y()+1.0f)/2.0f,value}), 1.0f} );
  } else {
  
    if (rightMouseDown) dropPaint();
    
    if (leftMouseDown) {
      const Vec2 trans = normPos - startDragPos;
      addTransformation(Mat4::translation(trans.x(), trans.y(), 0));
      startDragPos = normPos;
    }

    const Vec2i iPos = computePixelPos();
    if (iPos != lastMousePos) {
      client->setMousePos(normPos);
    }
    lastMousePos = iPos;
  }
}

void MyGLApp::mouseButton(int button, int state, int mods, double xPosition, double yPosition) {
  if (!client) return;
  
  if (colorChooserMode) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      rightMouseDown = (state == GLFW_PRESS);
      if (rightMouseDown) client->setColor( Vec4{Vec3::hsvToRgb({360*(normPos.x()+1.0f)/2.0f,(normPos.y()+1.0f)/2.0f,value}), 1.0f} );
    }
  } else {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      rightMouseDown = (state == GLFW_PRESS);
      if (rightMouseDown) dropPaint();
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      leftMouseDown = (state == GLFW_PRESS);
      if (state == GLFW_PRESS) {
        startDragPos = normPos;
      }
    }
  }
}

void MyGLApp::mouseWheel(double x_offset, double y_offset, double xPosition, double yPosition) {
  if (!client) return;
  
  if (colorChooserMode) {
    value = std::clamp<float>(value + float(y_offset)/wheelScale, 0.0f, 1.0);
    fillHSVImage();
  } else {
    addTransformation(Mat4::translation(-normPos.x(), -normPos.y(), 0) *
                      Mat4::scaling(1.0f+float(y_offset)/wheelScale) *
                      Mat4::translation(normPos.x(), normPos.y(), 0));
  }
}

void MyGLApp::keyboardChar(unsigned int codepoint) {
  if (!client) {
    if (!addressComplete) {
      serverAddress += char(codepoint);
      responseImage = MyClient::fr.render(serverAddress);
    } else {
      userName += char(codepoint);
      responseImage = MyClient::fr.render(userName);
    }
  }
}
  
void MyGLApp::keyboard(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    
    if (key == GLFW_KEY_ESCAPE) {
      closeWindow();
      return;
    }

    if (!client) {
      std::string& str = addressComplete ? userName : serverAddress;
      bool& complete = addressComplete ? nameComplete : addressComplete;
      switch (key) {
        case GLFW_KEY_BACKSPACE :
          if (str.size() > 0) str.erase(str.size() - 1);
          responseImage = MyClient::fr.render(str);
          break;
        case GLFW_KEY_ENTER :
          if (str.size() > 0) complete = true;
          break;
      }
      return;
    }

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
      if (colorChooserMode) {
        quickColors[key-GLFW_KEY_0] = Vec4{Vec3::hsvToRgb({360*(normPos.x()+1.0f)/2.0f,(normPos.y()+1.0f)/2.0f,value}), 1.0f};
      } else {
        client->setColor(quickColors[key-GLFW_KEY_0]);
      }
    }
    
    switch (key) {
      case GLFW_KEY_ESCAPE:
        closeWindow();
        break;
      case GLFW_KEY_Q:
        wheelScale /= 1.5f;
        break;
      case GLFW_KEY_W:
        wheelScale *= 1.5f;
        break;
      case GLFW_KEY_P:
        client->lockData();
        BMP::save("artwork.bmp", client->getImage());
        client->unlockData();
        break;
      case GLFW_KEY_R:
        userTransformation = Mat4{};
        updateMousePos();
        break;
      case GLFW_KEY_C:
        colorChooserMode = !colorChooserMode;
        break;
    }
  }
}

void MyGLApp::animate(double animationTime) {
  currentImage = size_t(animationTime*2) % connectingImage.size();
}

void MyGLApp::draw() {
  GL(glClearColor(0.0f,0.0f,0.0f,0.0f));
  GL(glClear(GL_COLOR_BUFFER_BIT));
  std::vector<float> glShape;

  if (!client) {
    
    if (nameComplete && addressComplete) {
      connectingImage[0] = MyClient::fr.render("Connecting to " + serverAddress);
      connectingImage[1] = MyClient::fr.render("Connecting to " + serverAddress + " .");
      connectingImage[2] = MyClient::fr.render("Connecting to " + serverAddress + " ..");
      connectingImage[3] = MyClient::fr.render("Connecting to " + serverAddress + " ...");

      client = std::make_shared<MyClient>(serverAddress, serverPort, userName);
    } else {
      if (serverAddress.empty()) {
        responseImage = MyClient::fr.render("Type in server address:");
      } else if (addressComplete && userName.empty()) {
        responseImage = MyClient::fr.render("Type in your name:");
      }
      setDrawTransform(Mat4::scaling(1.0f/3.0f) * computeBaseTransform({responseImage.width, responseImage.height}) );
      drawImage(responseImage);
      return;
    }
  }
  
  if (!client->isValid()) {
    setDrawTransform(Mat4::scaling(connectingImage[currentImage].width / (connectingImage[0].width * 2.0f)) * computeBaseTransform({connectingImage[currentImage].width, connectingImage[currentImage].height}) );
    drawImage(connectingImage[currentImage]);
    return;
  }
  
  if (colorChooserMode) {
    setDrawTransform(Mat4{});
    drawImage(hsvImage);
    
    glShape.clear();
    glShape.push_back(normPos.x()); glShape.push_back(normPos.y()); glShape.push_back(0.0f);
    glShape.push_back(0.0f); glShape.push_back(0.0f); glShape.push_back(0.0f);  glShape.push_back(1.0f);
    drawPoints(glShape, 10, false);

    return;
  }
  
  
  client->lockData();
  imageSize = Vec2ui{client->getImage().width, client->getImage().height};
  client->unlockData();
  baseTransformation = computeBaseTransform(imageSize);
  
  setDrawTransform(userTransformation*baseTransformation);
  setImageFilter(GL_NEAREST,GL_NEAREST);
  client->lockData();
  drawImage(client->getImage());
  const std::vector<ClientInfoClientSide> others = client->getClientInfos();
  for (const ClientInfoClientSide& m : others) {
    glShape.push_back(m.pos.x()); glShape.push_back(m.pos.y()); glShape.push_back(0.0f);
    glShape.push_back(m.color.x()); glShape.push_back(m.color.y()); glShape.push_back(m.color.z());  glShape.push_back(m.color.w());
  }
  Vec4 color{client->getColor()};
  drawPoints(glShape, 10, true);
  
  for (const ClientInfoClientSide& m : others) {
    setDrawTransform( Mat4::translation(1.0f, 1.0f, 0.0f) * Mat4::scaling(1/10.0f) * computeBaseTransform({m.image.width, m.image.height}) * Mat4::translation(m.pos.x(), m.pos.y(), 0.0f) * userTransformation * baseTransformation );
    drawImage(m.image);
  }
  
  client->unlockData();
  
  glShape.clear();
  glShape.push_back(normPos.x()); glShape.push_back(normPos.y()); glShape.push_back(0.0f);
  glShape.push_back(color.r()); glShape.push_back(color.y()); glShape.push_back(color.z());  glShape.push_back(color.w());
  setDrawTransform(userTransformation*baseTransformation);
  
  setPointTexture(cursorShape);
  drawPoints(glShape, 40, true);
  resetPointTexture();
}