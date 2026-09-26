#include <GL/freeglut.h>
#include <algorithm>
#include <cstring>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "third_party/stb_image_write.h"
#pragma GCC diagnostic pop

const int WIDTH = 512;
const int HEIGHT = 412;

// Layout constants below are measured proportionally from the reference
// drawing (to-copy.png) so the render matches it closely.
const float CX = 135.0f;
const float FLAG_POLE_TOP = 6.0f;
const float FLAG_TIP_Y = 18.0f;
const float FLAG_LOWER_Y = 30.0f;
const float FLAG_TIP_DX = 48.0f;
const float ROOF_APEX_Y = 47.0f;
const float BOX_TOP_Y = 117.0f;
const float BOX_HALF = 32.0f;
const float WINDOW_HALF = 10.0f;
const float WINDOW_TOP = 146.0f;
const float WINDOW_BOTTOM = 171.0f;
const float PILLAR_HALF = 45.0f;
const float PILLAR_TOP_Y = 193.0f;
const float BASE_Y = 400.0f;
const float BELT_Y = 204.0f;
const float DOOR_HALF = 46.0f;
const float OUTER_WALL_TOP_Y = 235.0f;
const float FAR_LEFT_X = 20.0f;
const float FAR_RIGHT_X = 495.0f;

void setColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

void drawLine(float x1, float y1, float x2, float y2, float width = 2.0f) {
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void drawLoop3(float x1, float y1, float x2, float y2, float x3, float y3, float width = 2.0f) {
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

void drawLoop4(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float width = 2.0f) {
    glLineWidth(width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glVertex2f(x4, y4);
    glEnd();
}

float mx(float x, bool mirror) {
    return mirror ? (WIDTH - x) : x;
}

void drawTower(float cx, bool mirror, float &innerX, float &outerX) {
    float poleX = mx(cx, mirror);

    setColor(0.0f, 0.0f, 0.0f);
    drawLine(poleX, FLAG_POLE_TOP, poleX, ROOF_APEX_Y);

    float flagTip = poleX + FLAG_TIP_DX;
    drawLoop3(poleX, FLAG_POLE_TOP, flagTip, FLAG_TIP_Y, poleX, FLAG_LOWER_Y);

    float roofLeft = mx(cx - BOX_HALF, mirror);
    float roofRight = mx(cx + BOX_HALF, mirror);
    setColor(0.9f, 0.1f, 0.1f);
    drawLoop3(poleX, ROOF_APEX_Y, roofLeft, BOX_TOP_Y, roofRight, BOX_TOP_Y);

    float boxLeft = mx(cx - BOX_HALF, mirror);
    float boxRight = mx(cx + BOX_HALF, mirror);
    setColor(0.0f, 0.0f, 0.0f);
    drawLoop4(boxLeft, BOX_TOP_Y, boxRight, BOX_TOP_Y, boxRight, PILLAR_TOP_Y, boxLeft, PILLAR_TOP_Y);

    float winLeft = mx(cx - WINDOW_HALF, mirror);
    float winRight = mx(cx + WINDOW_HALF, mirror);
    drawLoop4(winLeft, WINDOW_TOP, winRight, WINDOW_TOP, winRight, WINDOW_BOTTOM, winLeft, WINDOW_BOTTOM);

    float pA = mx(cx - PILLAR_HALF, mirror);
    float pB = mx(cx + PILLAR_HALF, mirror);
    float screenLeft = std::min(pA, pB);
    float screenRight = std::max(pA, pB);

    // Shoulder cap: the wall pillars are wider than the tower box sitting on
    // top of them, so a horizontal line is needed to close that gap - without
    // it the tower box appears to float disconnected from the wall below.
    drawLine(screenLeft, PILLAR_TOP_Y, screenRight, PILLAR_TOP_Y);
    drawLine(screenLeft, PILLAR_TOP_Y, screenLeft, BASE_Y);
    drawLine(screenRight, PILLAR_TOP_Y, screenRight, BASE_Y);

    if (!mirror) {
        outerX = screenLeft;
        innerX = screenRight;
    } else {
        innerX = screenLeft;
        outerX = screenRight;
    }
}

void drawCastle() {
    float leftInner, leftOuter, rightInner, rightOuter;
    drawTower(CX, false, leftInner, leftOuter);
    drawTower(CX, true, rightInner, rightOuter);

    // Belt line: connects the two towers partway down. The door sits lower
    // still, decoupled from this line by a stretch of bare wall - it does
    // not touch or grow out of it.
    setColor(0.0f, 0.0f, 0.0f);
    drawLine(leftInner, BELT_Y, rightInner, BELT_Y);

    float doorLeft = WIDTH / 2.0f - DOOR_HALF;
    float doorRight = WIDTH / 2.0f + DOOR_HALF;
    float doorMid = (doorLeft + doorRight) / 2.0f;
    drawLoop4(doorLeft, OUTER_WALL_TOP_Y, doorRight, OUTER_WALL_TOP_Y, doorRight, BASE_Y, doorLeft, BASE_Y);
    drawLine(doorMid, OUTER_WALL_TOP_Y, doorMid, BASE_Y);

    setColor(0.9f, 0.1f, 0.1f);
    drawLine(FAR_LEFT_X, OUTER_WALL_TOP_Y, leftOuter, OUTER_WALL_TOP_Y);
    drawLine(rightOuter, OUTER_WALL_TOP_Y, FAR_RIGHT_X, OUTER_WALL_TOP_Y);

    setColor(0.0f, 0.0f, 0.0f);
    drawLine(FAR_LEFT_X, OUTER_WALL_TOP_Y, FAR_LEFT_X, BASE_Y);
    drawLine(FAR_RIGHT_X, OUTER_WALL_TOP_Y, FAR_RIGHT_X, BASE_Y);
    drawLine(FAR_LEFT_X, BASE_Y, FAR_RIGHT_X, BASE_Y);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawCastle();
    glutSwapBuffers();
}

void exportPng(const char *path) {
    glClear(GL_COLOR_BUFFER_BIT);
    drawCastle();
    glFinish();

    std::vector<unsigned char> pixels(WIDTH * HEIGHT * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL's origin is bottom-left; PNG rows go top-down, so flip vertically.
    std::vector<unsigned char> flipped(WIDTH * HEIGHT * 3);
    for (int y = 0; y < HEIGHT; ++y) {
        std::memcpy(&flipped[y * WIDTH * 3],
                    &pixels[(HEIGHT - 1 - y) * WIDTH * 3],
                    WIDTH * 3);
    }

    stbi_write_png(path, WIDTH, HEIGHT, 3, flipped.data(), WIDTH * 3);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Castle");
    glClearColor(0.96f, 0.94f, 0.90f, 1.0f);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    reshape(WIDTH, HEIGHT);

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
            exportPng(argv[i + 1]);
            return 0;
        }
    }

    glutMainLoop();
    return 0;
}
