#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>

// --------------------------------------------------
// GLOBAL FLAGS & CONSTANTS
// --------------------------------------------------
bool showDoor2D    = true;
bool showWindows2D = true;
bool showTable2D   = true;
bool is3DMode      = false;

int gWindowWidth  = 1000;
int gWindowHeight = 900;

// Room dimensions (shared between camera & drawing)
const float ROOM_HALF_WIDTH  = 6.0f; // x
const float ROOM_HALF_DEPTH  = 8.0f; // z
const float ROOM_HEIGHT      = 3.0f; // y

// 3D camera (FPS style)
float camX = 0.0f;
float camY = 1.7f;
float camZ = ROOM_HALF_DEPTH - 1.0f;   // near front wall, inside room
float camYawDeg   = 180.0f;            // facing towards center
float camPitchDeg = -10.0f;

// Movement keys
bool keyW = false, keyA = false, keyS = false, keyD = false;
bool keyQ = false, keyE = false;

// Mouse look
int  lastMouseX = 0, lastMouseY = 0;
bool firstMouse = true;

// Fan + door animation
float fanAngleDeg  = 0.0f;
float fanSpeedDeg  = 4.0f;  // degrees per frame, adjustable

float doorAngleDeg = 0.0f;
bool  doorOpen     = false;   // logical state (target)
const float DOOR_MAX_ANGLE = 90.0f;

// --------------------------------------------------
// 2D HELPERS (Bresenham + Midpoint Circle)
// --------------------------------------------------
void drawPixel(int x, int y)
{
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawLineBresenham(int x1, int y1, int x2, int y2)
{
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        drawPixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void drawCircleMidpoint(int xc, int yc, int r)
{
    int x = 0, y = r;
    int d = 1 - r;

    while (x <= y)
    {
        drawPixel(xc + x, yc + y);
        drawPixel(xc - x, yc + y);
        drawPixel(xc + x, yc - y);
        drawPixel(xc - x, yc - y);
        drawPixel(xc + y, yc + x);
        drawPixel(xc - y, yc + x);
        drawPixel(xc + y, yc - x);
        drawPixel(xc - y, yc - x);

        if (d < 0)
            d += 2 * x + 3;
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

// --------------------------------------------------
// 2D FLOOR PLAN (TOP VIEW)
// --------------------------------------------------
void drawOfficePlan2D()
{
    int left   = 100;
    int right  = 900;
    int bottom = 100;
    int top    = 700;

    // Room outline
    glColor3f(1.0f, 1.0f, 1.0f);
    drawLineBresenham(left, bottom, right, bottom);
    drawLineBresenham(right, bottom, right, top);
    drawLineBresenham(right, top, left, top);
    drawLineBresenham(left, top, left, bottom);

    // Round table (center)
    if (showTable2D)
    {
        glColor3f(1.0f, 0.8f, 0.0f);
        int cx = 500;
        int cy = 420;
        int radius = 120;
        drawCircleMidpoint(cx, cy, radius);
    }

    // Desk (left)
    glColor3f(1.0f, 0.5f, 0.0f);
    int dx1 = 220, dy1 = 450;
    int dx2 = 420, dy2 = 520;
    drawLineBresenham(dx1, dy1, dx2, dy1);
    drawLineBresenham(dx2, dy1, dx2, dy2);
    drawLineBresenham(dx2, dy2, dx1, dy2);
    drawLineBresenham(dx1, dy2, dx1, dy1);

    // Chair
    glColor3f(0.0f, 0.7f, 1.0f);
    int cx1 = 440, cy1 = 450;
    int cx2 = 490, cy2 = 500;
    drawLineBresenham(cx1, cy1, cx2, cy1);
    drawLineBresenham(cx2, cy1, cx2, cy2);
    drawLineBresenham(cx2, cy2, cx1, cy2);
    drawLineBresenham(cx1, cy2, cx1, cy1);

    // Door (bottom)
    if (showDoor2D)
    {
        glColor3f(0.0f, 1.0f, 0.0f);
        int doorLeft  = 440;
        int doorRight = 560;
        int doorHeight = 60;
        drawLineBresenham(doorLeft,  bottom, doorLeft,  bottom + doorHeight);
        drawLineBresenham(doorRight, bottom, doorRight, bottom + doorHeight);
    }

    // Windows (top)
    if (showWindows2D)
    {
        glColor3f(0.2f, 0.8f, 1.0f);
        drawLineBresenham(150, top, 300, top);
        drawLineBresenham(700, top, 850, top);
    }
}

// --------------------------------------------------
// 3D HELPERS
// --------------------------------------------------

// Unit cube centered at origin (1x1x1)
void drawUnitBox()
{
    float h = 0.5f;
    glBegin(GL_QUADS);

    // Top
    glNormal3f(0, 1, 0);
    glVertex3f(-h, h, -h);
    glVertex3f( h, h, -h);
    glVertex3f( h, h,  h);
    glVertex3f(-h, h,  h);

    // Bottom
    glNormal3f(0, -1, 0);
    glVertex3f(-h, -h, -h);
    glVertex3f(-h, -h,  h);
    glVertex3f( h, -h,  h);
    glVertex3f( h, -h, -h);

    // Front
    glNormal3f(0, 0, 1);
    glVertex3f(-h, -h, h);
    glVertex3f( h, -h, h);
    glVertex3f( h,  h, h);
    glVertex3f(-h,  h, h);

    // Back
    glNormal3f(0, 0, -1);
    glVertex3f(-h, -h, -h);
    glVertex3f(-h,  h, -h);
    glVertex3f( h,  h, -h);
    glVertex3f( h, -h, -h);

    // Left
    glNormal3f(-1, 0, 0);
    glVertex3f(-h, -h, -h);
    glVertex3f(-h, -h,  h);
    glVertex3f(-h,  h,  h);
    glVertex3f(-h,  h, -h);

    // Right
    glNormal3f(1, 0, 0);
    glVertex3f(h, -h, -h);
    glVertex3f(h,  h, -h);
    glVertex3f(h,  h,  h);
    glVertex3f(h, -h,  h);

    glEnd();
}

// Cylinder for table / plant pot / lamp
void drawCylinder(float radius, float height, int segments = 32)
{
    float halfH = height * 0.5f;

    // Side
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (2.0f * 3.1415926f * i) / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        glNormal3f(x, 0.0f, z);
        glVertex3f(x, -halfH, z);
        glVertex3f(x,  halfH, z);
    }
    glEnd();

    // Top
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, halfH, 0);
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (2.0f * 3.1415926f * i) / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        glVertex3f(x, halfH, z);
    }
    glEnd();

    // Bottom
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(0, -halfH, 0);
    for (int i = 0; i <= segments; ++i)
    {
        float theta = (2.0f * 3.1415926f * i) / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        glVertex3f(x, -halfH, z);
    }
    glEnd();
}

// --------------------------------------------------
// DRAW SIMPLE PERSON
// --------------------------------------------------
void drawSeatedPerson()
{
    // Very simple blocky character sitting at the chair

    // Legs
    glColor3f(0.1f, 0.1f, 0.3f); // dark pants
    glPushMatrix();
    glTranslatef(-0.18f, 0.4f, 0.1f);  // left leg
    glScalef(0.12f, 0.8f, 0.12f);
    drawUnitBox();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.18f, 0.4f, 0.1f);   // right leg
    glScalef(0.12f, 0.8f, 0.12f);
    drawUnitBox();
    glPopMatrix();

    // Torso
    glColor3f(0.0f, 0.4f, 0.8f); // shirt
    glPushMatrix();
    glTranslatef(0.0f, 0.9f, -0.05f);
    glScalef(0.45f, 0.7f, 0.25f);
    drawUnitBox();
    glPopMatrix();

    // Head
    glColor3f(1.0f, 0.8f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 1.4f, -0.05f);
    glScalef(0.30f, 0.35f, 0.30f);
    drawUnitBox();
    glPopMatrix();

    // Left arm reaching to keyboard
    glColor3f(0.0f, 0.4f, 0.8f);
    glPushMatrix();
    glTranslatef(-0.32f, 0.95f, -0.25f);
    glRotatef(-20.0f, 1, 0, 0);
    glScalef(0.12f, 0.4f, 0.12f);
    drawUnitBox();
    glPopMatrix();

    // Right arm
    glPushMatrix();
    glTranslatef(0.32f, 0.95f, -0.25f);
    glRotatef(-15.0f, 1, 0, 0);
    glScalef(0.12f, 0.4f, 0.12f);
    drawUnitBox();
    glPopMatrix();
}

// --------------------------------------------------
// 3D ROOM, FURNITURE, DOOR, FAN, LIGHTS
// --------------------------------------------------
void drawRoomAndObjects3D()
{
    // FLOOR
    glColor3f(0.12f, 0.12f, 0.16f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, 0.0f,  ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f,  ROOM_HALF_DEPTH);
    glEnd();

    // CEILING
    glColor3f(0.20f, 0.20f, 0.25f);
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT, -ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT,  ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, ROOM_HEIGHT,  ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, ROOM_HEIGHT, -ROOM_HALF_DEPTH);
    glEnd();

    // Ceiling light panels (2 big white rectangles)
    glColor3f(0.95f, 0.95f, 1.0f);
    glPushMatrix();
    glTranslatef(-1.5f, ROOM_HEIGHT - 0.02f, -1.0f);
    glScalef(3.0f, 0.05f, 0.8f);
    drawUnitBox();
    glPopMatrix();

    glPushMatrix();
    glTranslatef( 1.5f, ROOM_HEIGHT - 0.02f, -1.0f);
    glScalef(3.0f, 0.05f, 0.8f);
    drawUnitBox();
    glPopMatrix();

    // WALLS (inside facing)
    glColor3f(0.80f, 0.80f, 0.86f);

    // Back wall (z -)
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT, -ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, ROOM_HEIGHT, -ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glEnd();

    // Front wall (z +) with door gap
    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);

    // Left segment
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f,  ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f(-1.5f,           ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f(-1.5f,           0.0f,        ROOM_HALF_DEPTH);

    // Right segment
    glVertex3f( 1.5f,           0.0f,        ROOM_HALF_DEPTH);
    glVertex3f( 1.5f,           ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH,ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f( ROOM_HALF_WIDTH,0.0f,        ROOM_HALF_DEPTH);
    glEnd();

    // Left wall (x -)
    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, 0.0f,  ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f(-ROOM_HALF_WIDTH, ROOM_HEIGHT,-ROOM_HALF_DEPTH);
    glEnd();

    // Right wall (x +)
    glBegin(GL_QUADS);
    glNormal3f(-1, 0, 0);
    glVertex3f(ROOM_HALF_WIDTH, 0.0f, -ROOM_HALF_DEPTH);
    glVertex3f(ROOM_HALF_WIDTH, ROOM_HEIGHT,-ROOM_HALF_DEPTH);
    glVertex3f(ROOM_HALF_WIDTH, ROOM_HEIGHT, ROOM_HALF_DEPTH);
    glVertex3f(ROOM_HALF_WIDTH, 0.0f,  ROOM_HALF_DEPTH);
    glEnd();

    // ------------------ Furniture ------------------

    // Meeting table (cylinder)
    glColor3f(1.0f, 0.8f, 0.2f);
    glPushMatrix();
    glTranslatef(0.0f, 0.75f, 0.0f);
    glScalef(1.0f, 0.5f, 1.0f);
    drawCylinder(1.5f, 1.0f, 40);
    glPopMatrix();

    // Desk (left)
    glColor3f(0.90f, 0.55f, 0.25f);
    glPushMatrix();
    glTranslatef(-3.5f, 0.8f, -1.2f);
    glScalef(2.6f, 0.2f, 1.2f);
    drawUnitBox();
    glPopMatrix();

    // Desk legs
    glColor3f(0.4f, 0.25f, 0.18f);
    float legHeight = 0.8f;
    float legSize   = 0.1f;
    // front-left leg
    glPushMatrix();
    glTranslatef(-4.6f, legHeight * 0.5f, -1.8f);
    glScalef(legSize, legHeight, legSize);
    drawUnitBox();
    glPopMatrix();
    // front-right leg
    glPushMatrix();
    glTranslatef(-2.4f, legHeight * 0.5f, -1.8f);
    glScalef(legSize, legHeight, legSize);
    drawUnitBox();
    glPopMatrix();

    // Chair (seat + back)
    glColor3f(0.2f, 0.6f, 1.0f);
    // seat
    glPushMatrix();
    glTranslatef(-1.5f, 0.5f, -1.0f);
    glScalef(0.9f, 0.18f, 0.9f);
    drawUnitBox();
    glPopMatrix();
    // backrest
    glPushMatrix();
    glTranslatef(-1.5f, 1.0f, -1.6f);
    glScalef(0.9f, 0.7f, 0.15f);
    drawUnitBox();
    glPopMatrix();

    // Person sitting on the chair using the computer
    glPushMatrix();
    glTranslatef(-1.5f, 0.0f, -1.0f);
    drawSeatedPerson();
    glPopMatrix();

    // Cabinet (right side)
    glColor3f(0.7f, 0.7f, 0.75f);
    glPushMatrix();
    glTranslatef(ROOM_HALF_WIDTH - 1.0f, 1.1f, -ROOM_HALF_DEPTH + 2.0f);
    glScalef(1.0f, 2.2f, 0.7f);
    drawUnitBox();
    glPopMatrix();

    // Whiteboard (back wall)
    glColor3f(0.95f, 0.95f, 1.0f);
    glPushMatrix();
    glTranslatef(0.0f, 1.6f, -ROOM_HALF_DEPTH + 0.02f);
    glScalef(3.0f, 1.4f, 0.05f);
    drawUnitBox();
    glPopMatrix();

    // Monitor on desk
    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(-3.4f, 1.15f, -1.2f);
    glScalef(0.9f, 0.6f, 0.1f);
    drawUnitBox(); // screen
    glPopMatrix();
    // stand
    glPushMatrix();
    glTranslatef(-3.4f, 0.95f, -1.25f);
    glScalef(0.1f, 0.4f, 0.1f);
    drawUnitBox();
    glPopMatrix();

    // Keyboard (simple thin box)
    glColor3f(0.15f, 0.15f, 0.18f);
    glPushMatrix();
    glTranslatef(-2.9f, 0.9f, -1.2f);
    glScalef(0.9f, 0.05f, 0.25f);
    drawUnitBox();
    glPopMatrix();

    // Table Lamp on desk
    // Base
    glPushMatrix();
    glTranslatef(-3.0f, 0.9f, -0.9f);
    glColor3f(0.3f, 0.2f, 0.1f);
    glScalef(1.0f, 0.3f, 1.0f);
    drawCylinder(0.12f, 0.08f, 20);
    glPopMatrix();

    // Neck
    glPushMatrix();
    glTranslatef(-3.0f, 1.05f, -0.9f);
    glColor3f(0.7f, 0.7f, 0.7f);
    glScalef(1.0f, 1.0f, 1.0f);
    drawCylinder(0.05f, 0.35f, 16);
    glPopMatrix();

    // Shade
    glPushMatrix();
    glTranslatef(-3.0f, 1.3f, -0.9f);
    glColor3f(1.0f, 0.95f, 0.75f); // warm light
    drawCylinder(0.18f, 0.30f, 24);
    glPopMatrix();

    // Plant (front-left corner)
    glPushMatrix();
    // pot
    glColor3f(0.6f, 0.3f, 0.15f);
    glTranslatef(-ROOM_HALF_WIDTH + 1.0f, 0.4f, ROOM_HALF_DEPTH - 1.0f);
    glScalef(1.0f, 0.8f, 1.0f);
    drawCylinder(0.3f, 0.6f, 24);
    glPopMatrix();

    glPushMatrix();
    // leaves (simple box)
    glColor3f(0.1f, 0.6f, 0.2f);
    glTranslatef(-ROOM_HALF_WIDTH + 1.0f, 1.1f, ROOM_HALF_DEPTH - 1.0f);
    glScalef(0.6f, 1.0f, 0.6f);
    drawUnitBox();
    glPopMatrix();

    // --------------- Door (animated single panel) ---------------

    const float DOOR_WIDTH  = 3.0f;
    const float DOOR_HEIGHT = 2.2f;
    const float DOOR_THICK  = 0.08f;

    glPushMatrix();
    // Hinge at x=-1.5, z=ROOM_HALF_DEPTH
    glTranslatef(-1.5f, DOOR_HEIGHT * 0.5f, ROOM_HALF_DEPTH + 0.01f);
    glRotatef(doorAngleDeg, 0.0f, 1.0f, 0.0f);
    // Move door so hinge is at its left edge
    glTranslatef(DOOR_WIDTH * 0.5f, 0.0f, 0.0f);

    glColor3f(0.95f, 0.95f, 0.98f);
    glScalef(DOOR_WIDTH, DOOR_HEIGHT, DOOR_THICK);
    drawUnitBox();
    glPopMatrix();

    // Door handle
    glPushMatrix();
    glTranslatef(-1.5f, DOOR_HEIGHT * 0.7f, ROOM_HALF_DEPTH + 0.12f);
    glRotatef(doorAngleDeg, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.9f, 0.0f, 0.15f);
    glColor3f(0.9f, 0.75f, 0.25f);
    glScalef(0.25f, 0.12f, 0.12f);
    drawUnitBox();
    glPopMatrix();

    // --------------- Ceiling Fan -------------------

    // Hub
    glPushMatrix();
    glTranslatef(0.0f, ROOM_HEIGHT - 0.2f, 0.0f);
    glRotatef(fanAngleDeg, 0.0f, 1.0f, 0.0f);

    glColor3f(0.85f, 0.85f, 0.85f);
    glScalef(0.3f, 0.1f, 0.3f);
    drawUnitBox();
    glPopMatrix();

    // Blades
    glPushMatrix();
    glTranslatef(0.0f, ROOM_HEIGHT - 0.25f, 0.0f);
    glRotatef(fanAngleDeg, 0.0f, 1.0f, 0.0f);
    glColor3f(0.9f, 0.9f, 0.9f);

    for (int i = 0; i < 4; ++i)
    {
        glPushMatrix();
        glRotatef(i * 90.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(1.4f, 0.0f, 0.0f);
        glScalef(2.8f, 0.05f, 0.3f);
        drawUnitBox();
        glPopMatrix();
    }
    glPopMatrix();
}

// --------------------------------------------------
// LIGHTING
// --------------------------------------------------
void setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 0.0f, ROOM_HEIGHT - 0.1f, -2.0f, 1.0f };
    GLfloat amb[]      = { 0.25f, 0.25f, 0.30f, 1.0f };
    GLfloat diff[]     = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat spec[]     = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
}

// --------------------------------------------------
// DISPLAY
// --------------------------------------------------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!is3DMode)
    {
        // 2D MODE
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, gWindowWidth, 0, gWindowHeight);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        drawOfficePlan2D();
    }
    else
    {
        // 3D MODE
        glEnable(GL_DEPTH_TEST);
        setupLighting();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = (double)gWindowWidth / (double)gWindowHeight;
        gluPerspective(60.0, aspect, 0.1, 100.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Camera direction from yaw/pitch
        const float DEG2RAD = 3.1415926f / 180.0f;
        float yawRad   = camYawDeg   * DEG2RAD;
        float pitchRad = camPitchDeg * DEG2RAD;

        float dirX = std::cos(pitchRad) * std::sin(yawRad);
        float dirY = std::sin(pitchRad);
        float dirZ = std::cos(pitchRad) * std::cos(yawRad);

        gluLookAt(camX, camY, camZ,
                  camX + dirX, camY + dirY, camZ + dirZ,
                  0.0f, 1.0f, 0.0f);

        drawRoomAndObjects3D();
    }

    glutSwapBuffers();
}

// --------------------------------------------------
// KEYBOARD (PRESS)
// --------------------------------------------------
void keyboard(unsigned char key, int, int)
{
    switch (key)
    {
    case 27: // ESC
        std::exit(0);

    case 'v': case 'V':
        is3DMode = !is3DMode;
        firstMouse = true; // reset mouse delta
        break;

    // ---------- 2D toggles ----------
    case '1': showDoor2D    = !showDoor2D;    break;
    case '2': showWindows2D = !showWindows2D; break;
    case '3': showTable2D   = !showTable2D;   break;

    // ---------- 3D door toggle ----------
    case 'o': case 'O':
        doorOpen = !doorOpen;
        break;

    // ---------- Fan speed presets ----------
    // 7 = slow, 8 = normal, 9 = fast
    case '7': fanSpeedDeg = 1.5f; break;
    case '8': fanSpeedDeg = 4.0f; break;
    case '9': fanSpeedDeg = 8.0f; break;

    // ---------- Movement keys (set flags) ----------
    case 'w': case 'W': keyW = true; break;
    case 's': case 'S': keyS = true; break;
    case 'a': case 'A': keyA = true; break;
    case 'd': case 'D': keyD = true; break;
    case 'q': case 'Q': keyQ = true; break;
    case 'e': case 'E': keyE = true; break;
    }

    glutPostRedisplay();
}

// --------------------------------------------------
// KEYBOARD (RELEASE)
// --------------------------------------------------
void keyboardUp(unsigned char key, int, int)
{
    switch (key)
    {
    case 'w': case 'W': keyW = false; break;
    case 's': case 'S': keyS = false; break;
    case 'a': case 'A': keyA = false; break;
    case 'd': case 'D': keyD = false; break;
    case 'q': case 'Q': keyQ = false; break;
    case 'e': case 'E': keyE = false; break;
    }
}

// --------------------------------------------------
// MOUSE LOOK
// --------------------------------------------------
void passiveMouseMotion(int x, int y)
{
    if (!is3DMode) { firstMouse = true; return; }

    if (firstMouse)
    {
        lastMouseX = x;
        lastMouseY = y;
        firstMouse = false;
        return;
    }

    float dx = (float)(x - lastMouseX);
    float dy = (float)(y - lastMouseY);
    lastMouseX = x;
    lastMouseY = y;

    const float sensitivity = 0.15f;
    camYawDeg   += dx * sensitivity;
    camPitchDeg -= dy * sensitivity;

    // Clamp pitch so we don't flip
    if (camPitchDeg > 80.0f)  camPitchDeg = 80.0f;
    if (camPitchDeg < -80.0f) camPitchDeg = -80.0f;

    glutPostRedisplay();
}

// --------------------------------------------------
// RESHAPE
// --------------------------------------------------
void reshape(int w, int h)
{
    gWindowWidth  = (w > 1) ? w : 1;
    gWindowHeight = (h > 1) ? h : 1;
    glViewport(0, 0, gWindowWidth, gWindowHeight);
}

// --------------------------------------------------
// UPDATE CAMERA MOVEMENT
// --------------------------------------------------
void updateCamera()
{
    if (!is3DMode) return;

    const float MOVE_SPEED = 0.10f;
    const float DEG2RAD = 3.1415926f / 180.0f;

    float yawRad = camYawDeg * DEG2RAD;

    // Forward & right vectors in XZ plane
    float forwardX = std::sin(yawRad);
    float forwardZ = std::cos(yawRad);
    float rightX   =  forwardZ;
    float rightZ   = -forwardX;

    if (keyW)
    {
        camX += forwardX * MOVE_SPEED;
        camZ += forwardZ * MOVE_SPEED;
    }
    if (keyS)
    {
        camX -= forwardX * MOVE_SPEED;
        camZ -= forwardZ * MOVE_SPEED;
    }
    if (keyA)
    {
        camX -= rightX * MOVE_SPEED;
        camZ -= rightZ * MOVE_SPEED;
    }
    if (keyD)
    {
        camX += rightX * MOVE_SPEED;
        camZ += rightZ * MOVE_SPEED;
    }
    if (keyQ) camY -= MOVE_SPEED;
    if (keyE) camY += MOVE_SPEED;

    // Clamp to room bounds (stay inside)
    float margin = 0.6f;
    if (camX < -ROOM_HALF_WIDTH  + margin) camX = -ROOM_HALF_WIDTH  + margin;
    if (camX >  ROOM_HALF_WIDTH  - margin) camX =  ROOM_HALF_WIDTH  - margin;
    if (camZ < -ROOM_HALF_DEPTH  + margin) camZ = -ROOM_HALF_DEPTH  + margin;
    if (camZ >  ROOM_HALF_DEPTH  - margin) camZ =  ROOM_HALF_DEPTH  - margin;
    if (camY < 0.7f)             camY = 0.7f;
    if (camY > ROOM_HEIGHT - 0.3f) camY = ROOM_HEIGHT - 0.3f;
}

// --------------------------------------------------
// TIMER – FAN + DOOR + CAMERA
// --------------------------------------------------
void timer(int)
{
    // Fan spin (uses adjustable speed)
    fanAngleDeg += fanSpeedDeg;
    if (fanAngleDeg >= 360.0f) fanAngleDeg -= 360.0f;

    // Door animation
    float targetAngle = doorOpen ? DOOR_MAX_ANGLE : 0.0f;
    float diff = targetAngle - doorAngleDeg;
    float step = 3.0f;

    if (std::fabs(diff) > 0.1f)
        doorAngleDeg += (diff > 0 ? step : -step);

    // Update camera movement
    updateCamera();

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// --------------------------------------------------
// INIT
// --------------------------------------------------
void initGL()
{
    glClearColor(0.05f, 0.05f, 0.10f, 1.0f);
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Office Designer - Part 1 (2D + Full 3D FPS Preview)");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
