#include <sgct.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
    sgct::Engine* gEngine;

    const glm::vec4 lightPosition = glm::vec4(5.f, 5.f, 10.f, 1.f);
    const glm::vec4 lightAmbient = glm::vec4(0.1f, 0.1f, 0.1f, 1.f);
    const glm::vec4 lightDiffuse = glm::vec4(0.6f, 0.6f, 0.6f, 1.f);
    const glm::vec4 lightSpecular = glm::vec4(1.f, 1.f, 1.f, 1.f);

    const glm::vec4 materialAmbient = glm::vec4(1.f, 0.f, 0.f, 1.f);
    const glm::vec4 materialDiffuse = glm::vec4(1.f, 1.f, 0.f, 1.f);
    const glm::vec4 materialSpecular = glm::vec4(0.f, 1.f, 1.f, 1.f);
    const float materialShininess = 64.f;

    std::unique_ptr<sgct_utils::Sphere> sphere;
} // namespace

using namespace sgct;

void drawFun() {
    glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(lightPosition));
    glTranslatef(0.f, 0.f, -3.f);
    
    sphere->draw();
}

void initOGLFun() {
    sphere = std::make_unique<sgct_utils::Sphere>(1.f, 32);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);

    // Set up light 0
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, glm::value_ptr(lightAmbient));
    glLightfv(GL_LIGHT0, GL_DIFFUSE, glm::value_ptr(lightDiffuse));
    glLightfv(GL_LIGHT0, GL_SPECULAR, glm::value_ptr(lightSpecular));

    // Set up material
    glMaterialfv(GL_FRONT, GL_AMBIENT, glm::value_ptr(materialAmbient));
    glMaterialfv(GL_FRONT, GL_DIFFUSE, glm::value_ptr(materialDiffuse));
    glMaterialfv(GL_FRONT, GL_SPECULAR, glm::value_ptr(materialSpecular));
    glMaterialf(GL_FRONT, GL_SHININESS, materialShininess);

    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    gEngine->setInitOGLFunction(initOGLFun);
    gEngine->setDrawFunction(drawFun);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    sphere = nullptr;
    exit(EXIT_SUCCESS);
}
