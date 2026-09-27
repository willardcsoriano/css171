/*
================================================================================
COURSE:     CSS171 - Computer Graphics and Visual Computing
TASK:       Machine Problem #2 (2D Model with Primitives & Blinking Color)
FILENAME:   CSS171_MP2_Soriano.cpp
AUTHOR:     Willard Soriano
INSTRUCTOR: Polycarpio V. Cabalag II
DATE:       September 2026

DESCRIPTION:
This program renders a 2D alpine mountain landscape using OpenGL primitives.
The scene is constructed with triangle meshes (GL_TRIANGLES and GL_TRIANGLE_FAN),
demonstrating efficient vertex sharing, light and shadow facet division,
proportional centering, and active animation with blinking color effects.

KEY FEATURES:
1. Centered Massif: The primary summit is centered at X = 500.0 (in a 1000x700
   canvas) with balanced sub-peaks and cascading lateral ridges.
2. Efficient Triangular Facets: All mountain faces and snowcaps are tessellated
   using GL_TRIANGLES, sharing common ridge vertices between lit and shaded faces.
3. Color Selection: Atmospheric twilight palette featuring deep indigo sky,
   cool slate rock, warm lantern amber, and alpine snow highlights.
4. Animated Blinking Elements:
   - Summit Warning Beacon: High-visibility aviation warning beacon atop the
     central communication mast that flashes between radiant crimson and dark red.
   - Alpine Research Hut Lantern: Warm amber window light that flickers.
   - Twinkling Stars: Multi-phase star field cycling across brightness states.
================================================================================
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <fstream>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

// Canvas and Window Specifications
const int WINDOW_WIDTH  = 1000;
const int WINDOW_HEIGHT = 700;
const char *WINDOW_TITLE = "CSS171 MP2 - Alpine Mountain (Willard Soriano)";

// Animation Timing Constants (in milliseconds)
const int TIMER_INTERVAL_MS = 250; // 4 ticks per second

// Global Animation State Variables
int  g_timerTick     = 0;
bool g_beaconActive  = true;
int  g_lanternPhase  = 0;

// Structure representing a single star in the night sky
struct Star {
    float x;
    float y;
    float size;
    int   phaseOffset;
};

// Fixed star field data
const Star STARS[] = {
    {  80.0f, 620.0f, 2.5f, 0 },
    { 140.0f, 660.0f, 2.0f, 1 },
    { 220.0f, 630.0f, 3.0f, 2 },
    { 290.0f, 670.0f, 1.8f, 3 },
    { 360.0f, 610.0f, 2.2f, 1 },
    { 440.0f, 650.0f, 2.8f, 0 },
    { 580.0f, 660.0f, 2.0f, 2 },
    { 640.0f, 620.0f, 3.0f, 3 },
    { 720.0f, 670.0f, 2.4f, 1 },
    { 800.0f, 630.0f, 1.8f, 0 },
    { 880.0f, 660.0f, 2.6f, 2 },
    { 930.0f, 610.0f, 2.2f, 3 },
    { 170.0f, 550.0f, 1.8f, 2 },
    { 830.0f, 540.0f, 2.0f, 1 }
};
const int NUM_STARS = sizeof(STARS) / sizeof(STARS[0]);

// Utility helper to assign RGB color
inline void setColor(float r, float g, float b) {
    glColor3f(r, g, b);
}

// Utility helper to assign RGBA color (supports alpha blending)
inline void setColorA(float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
}

// Draw smooth sky gradient using triangle primitives
void drawSky() {
    glBegin(GL_TRIANGLES);

    // Upper gradient quad (split into two triangles)
    // Top: Deep twilight indigo
    // Mid: Dusky lavender horizon transition
    setColor(0.08f, 0.09f, 0.20f);
    glVertex2f(0.0f, (float)WINDOW_HEIGHT);
    glVertex2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
    setColor(0.24f, 0.20f, 0.34f);
    glVertex2f((float)WINDOW_WIDTH, 260.0f);

    setColor(0.08f, 0.09f, 0.20f);
    glVertex2f(0.0f, (float)WINDOW_HEIGHT);
    setColor(0.24f, 0.20f, 0.34f);
    glVertex2f((float)WINDOW_WIDTH, 260.0f);
    glVertex2f(0.0f, 260.0f);

    // Lower gradient quad down to base
    // Mid to warm twilight horizon amber glow
    setColor(0.24f, 0.20f, 0.34f);
    glVertex2f(0.0f, 260.0f);
    glVertex2f((float)WINDOW_WIDTH, 260.0f);
    setColor(0.38f, 0.26f, 0.34f);
    glVertex2f((float)WINDOW_WIDTH, 140.0f);

    setColor(0.24f, 0.20f, 0.34f);
    glVertex2f(0.0f, 260.0f);
    setColor(0.38f, 0.26f, 0.34f);
    glVertex2f((float)WINDOW_WIDTH, 140.0f);
    glVertex2f(0.0f, 140.0f);

    glEnd();
}

// Draw twinkling stars with dynamic brightness based on timer ticks
void drawStars() {
    for (int i = 0; i < NUM_STARS; ++i) {
        const Star &s = STARS[i];
        int cycle = (g_timerTick + s.phaseOffset) % 4;

        float brightness = 0.0f;
        switch (cycle) {
            case 0: brightness = 0.35f; break; // Dim
            case 1: brightness = 0.70f; break; // Medium
            case 2: brightness = 1.00f; break; // Peak brightness
            case 3: brightness = 0.50f; break; // Fading
        }

        setColor(brightness * 0.95f, brightness * 0.95f, brightness * 1.0f);

        // Render each star as a small 4-point diamond using 2 triangles
        glBegin(GL_TRIANGLES);
        glVertex2f(s.x, s.y + s.size);
        glVertex2f(s.x - s.size * 0.5f, s.y);
        glVertex2f(s.x + s.size * 0.5f, s.y);

        glVertex2f(s.x, s.y - s.size);
        glVertex2f(s.x + s.size * 0.5f, s.y);
        glVertex2f(s.x - s.size * 0.5f, s.y);
        glEnd();
    }
}

// Draw crescent moon in upper sky
void drawMoon() {
    const float cx = 160.0f;
    const float cy = 580.0f;
    const float radius = 32.0f;
    const int segments = 24;

    // Glowing base disc (pale crescent gold)
    setColor(0.96f, 0.92f, 0.75f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i * 2.0f * 3.14159265f / segments;
        glVertex2f(cx + cosf(angle) * radius, cy + sinf(angle) * radius);
    }
    glEnd();

    // Shadow cutout disc offset to create a clean crescent curve
    // Color matches surrounding deep sky
    setColor(0.11f, 0.12f, 0.23f);
    const float shadowOffsetX = 12.0f;
    const float shadowOffsetY = 6.0f;
    const float shadowRadius  = 30.0f;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx + shadowOffsetX, cy + shadowOffsetY);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i * 2.0f * 3.14159265f / segments;
        glVertex2f(cx + shadowOffsetX + cosf(angle) * shadowRadius,
                   cy + shadowOffsetY + sinf(angle) * shadowRadius);
    }
    glEnd();
}

// Draw distant background mountain peaks for atmospheric depth
void drawDistantMountainRange() {
    glBegin(GL_TRIANGLES);

    // Distant Peak Left
    // Lit facet (cool purple-slate)
    setColor(0.25f, 0.23f, 0.36f);
    glVertex2f(220.0f, 440.0f);
    glVertex2f(30.0f,  150.0f);
    glVertex2f(235.0f, 150.0f);

    // Shadow facet (darker purple-slate)
    setColor(0.18f, 0.16f, 0.28f);
    glVertex2f(220.0f, 440.0f);
    glVertex2f(235.0f, 150.0f);
    glVertex2f(410.0f, 150.0f);

    // Distant Peak Right
    // Lit facet
    setColor(0.24f, 0.22f, 0.35f);
    glVertex2f(760.0f, 430.0f);
    glVertex2f(570.0f, 150.0f);
    glVertex2f(770.0f, 150.0f);

    // Shadow facet
    setColor(0.17f, 0.15f, 0.27f);
    glVertex2f(760.0f, 430.0f);
    glVertex2f(770.0f, 150.0f);
    glVertex2f(970.0f, 150.0f);

    // Distant Rear Saddle Peak
    setColor(0.21f, 0.19f, 0.31f);
    glVertex2f(440.0f, 470.0f);
    glVertex2f(310.0f, 160.0f);
    glVertex2f(450.0f, 160.0f);

    setColor(0.16f, 0.14f, 0.24f);
    glVertex2f(440.0f, 470.0f);
    glVertex2f(450.0f, 160.0f);
    glVertex2f(590.0f, 160.0f);

    glEnd();
}

// Draw the prominent main mountain massif with faceted triangular tessellation
void drawMainMountainMassif() {
    glBegin(GL_TRIANGLES);

    // Master Palette for Faceted Alpine Stone:
    // Lit faces (receiving ambient western moonlight / sky glow)
    const float litHigh[3] = { 0.44f, 0.48f, 0.56f };
    const float litMid[3]  = { 0.35f, 0.39f, 0.47f };
    const float litLow[3]  = { 0.28f, 0.32f, 0.40f };

    // Shadow faces (facing eastward in deep shadow)
    const float shdHigh[3] = { 0.22f, 0.25f, 0.34f };
    const float shdMid[3]  = { 0.17f, 0.19f, 0.27f };
    const float shdLow[3]  = { 0.13f, 0.15f, 0.22f };

    // Common Ridge Vertices (efficient vertex sharing):
    // Central Peak Apex: (500.0, 540.0)
    // Left Sub-peak:     (320.0, 420.0)
    // Right Sub-peak:    (690.0, 430.0)
    // Outer Shoulders:   Left (130.0, 270.0), Right (870.0, 280.0)
    // Central Spine:     S1(490.0, 390.0), S2(515.0, 270.0), S3(500.0, 150.0)
    // Left Knolls:       K1(260.0, 290.0), K2(370.0, 240.0)
    // Right Knolls:      K3(620.0, 250.0), K4(760.0, 280.0)

    // --- SECTION 1: Central Peak Facets ---
    // Facet 1: Central Apex to Left Sub-peak (Lit upper face)
    setColor(litHigh[0], litHigh[1], litHigh[2]);
    glVertex2f(500.0f, 540.0f);
    glVertex2f(320.0f, 420.0f);
    glVertex2f(490.0f, 390.0f);

    // Facet 2: Central Apex to Right Sub-peak (Shadow upper face)
    setColor(shdHigh[0], shdHigh[1], shdHigh[2]);
    glVertex2f(500.0f, 540.0f);
    glVertex2f(490.0f, 390.0f);
    glVertex2f(690.0f, 430.0f);

    // --- SECTION 2: Central Spine & Gully Facets ---
    // Facet 3: Spine Upper-Left (Lit mid face)
    setColor(litMid[0], litMid[1], litMid[2]);
    glVertex2f(490.0f, 390.0f);
    glVertex2f(370.0f, 240.0f);
    glVertex2f(515.0f, 270.0f);

    // Facet 4: Spine Upper-Right (Shadow mid face)
    setColor(shdMid[0], shdMid[1], shdMid[2]);
    glVertex2f(490.0f, 390.0f);
    glVertex2f(515.0f, 270.0f);
    glVertex2f(620.0f, 250.0f);

    // Facet 5: Spine Lower-Left
    setColor(litLow[0], litLow[1], litLow[2]);
    glVertex2f(515.0f, 270.0f);
    glVertex2f(370.0f, 240.0f);
    glVertex2f(500.0f, 150.0f);

    // Facet 6: Spine Lower-Right
    setColor(shdLow[0], shdLow[1], shdLow[2]);
    glVertex2f(515.0f, 270.0f);
    glVertex2f(500.0f, 150.0f);
    glVertex2f(620.0f, 250.0f);

    // --- SECTION 3: Left Wing / Sub-Peak Facets ---
    // Facet 7: Left Sub-peak Upper Face
    setColor(litHigh[0] * 0.95f, litHigh[1] * 0.95f, litHigh[2] * 0.95f);
    glVertex2f(320.0f, 420.0f);
    glVertex2f(260.0f, 290.0f);
    glVertex2f(490.0f, 390.0f);

    // Facet 8: Left Intermediate Couloir
    setColor(litMid[0] * 0.92f, litMid[1] * 0.92f, litMid[2] * 0.92f);
    glVertex2f(490.0f, 390.0f);
    glVertex2f(260.0f, 290.0f);
    glVertex2f(370.0f, 240.0f);

    // Facet 9: Left Outer Shoulder Upper
    setColor(litMid[0], litMid[1], litMid[2]);
    glVertex2f(320.0f, 420.0f);
    glVertex2f(130.0f, 270.0f);
    glVertex2f(260.0f, 290.0f);

    // Facet 10: Left Outer Flank to Base
    setColor(litLow[0], litLow[1], litLow[2]);
    glVertex2f(130.0f, 270.0f);
    glVertex2f(60.0f,  150.0f);
    glVertex2f(220.0f, 150.0f);

    // Facet 11: Left Shoulder Infill
    setColor(litLow[0] * 1.05f, litLow[1] * 1.05f, litLow[2] * 1.05f);
    glVertex2f(130.0f, 270.0f);
    glVertex2f(220.0f, 150.0f);
    glVertex2f(260.0f, 290.0f);

    // Facet 12: Left Base Infill to Center
    setColor(litLow[0], litLow[1], litLow[2]);
    glVertex2f(260.0f, 290.0f);
    glVertex2f(220.0f, 150.0f);
    glVertex2f(370.0f, 240.0f);

    setColor(litLow[0] * 0.9f, litLow[1] * 0.9f, litLow[2] * 0.9f);
    glVertex2f(370.0f, 240.0f);
    glVertex2f(220.0f, 150.0f);
    glVertex2f(500.0f, 150.0f);

    // --- SECTION 4: Right Wing / Sub-Peak Facets ---
    // Facet 13: Right Sub-peak Couloir Face
    setColor(shdHigh[0] * 1.05f, shdHigh[1] * 1.05f, shdHigh[2] * 1.05f);
    glVertex2f(490.0f, 390.0f);
    glVertex2f(690.0f, 430.0f);
    glVertex2f(620.0f, 250.0f);

    // Facet 14: Right Sub-peak Outer Face
    setColor(shdMid[0], shdMid[1], shdMid[2]);
    glVertex2f(690.0f, 430.0f);
    glVertex2f(760.0f, 280.0f);
    glVertex2f(620.0f, 250.0f);

    // Facet 15: Right Outer Shoulder Upper
    setColor(shdMid[0] * 0.95f, shdMid[1] * 0.95f, shdMid[2] * 0.95f);
    glVertex2f(690.0f, 430.0f);
    glVertex2f(870.0f, 280.0f);
    glVertex2f(760.0f, 280.0f);

    // Facet 16: Right Outer Flank to Base
    setColor(shdLow[0], shdLow[1], shdLow[2]);
    glVertex2f(870.0f, 280.0f);
    glVertex2f(780.0f, 150.0f);
    glVertex2f(940.0f, 150.0f);

    // Facet 17: Right Shoulder Infill
    setColor(shdLow[0] * 1.08f, shdLow[1] * 1.08f, shdLow[2] * 1.08f);
    glVertex2f(870.0f, 280.0f);
    glVertex2f(760.0f, 280.0f);
    glVertex2f(780.0f, 150.0f);

    // Facet 18: Right Base Infill to Center
    setColor(shdLow[0], shdLow[1], shdLow[2]);
    glVertex2f(760.0f, 280.0f);
    glVertex2f(620.0f, 250.0f);
    glVertex2f(780.0f, 150.0f);

    setColor(shdLow[0] * 0.92f, shdLow[1] * 0.92f, shdLow[2] * 0.92f);
    glVertex2f(620.0f, 250.0f);
    glVertex2f(500.0f, 150.0f);
    glVertex2f(780.0f, 150.0f);

    glEnd();
}

// Draw detailed snowcaps fitted cleanly over peak summits using triangles
void drawSnowcaps() {
    glBegin(GL_TRIANGLES);

    // Snow Colors:
    // Lit Snow (sunward face): Pure alpine white
    const float snowLit[3] = { 0.94f, 0.97f, 1.00f };
    // Shaded Snow (shadow face): Powder periwinkle
    const float snowShd[3] = { 0.73f, 0.79f, 0.89f };

    // --- 1. Central Summit Snowcap (Apex at 500.0, 540.0) ---
    // Lit Face (Left of ridge)
    setColor(snowLit[0], snowLit[1], snowLit[2]);
    // Main upper lit triangle
    glVertex2f(500.0f, 540.0f);
    glVertex2f(430.0f, 493.0f);
    glVertex2f(495.0f, 465.0f);

    // Left lower tongue 1
    glVertex2f(430.0f, 493.0f);
    glVertex2f(410.0f, 465.0f);
    glVertex2f(450.0f, 475.0f);

    // Infill body between tongue 1 and ridge
    glVertex2f(430.0f, 493.0f);
    glVertex2f(450.0f, 475.0f);
    glVertex2f(495.0f, 465.0f);

    // Lower tongue 2 extending down ridge spine
    glVertex2f(450.0f, 475.0f);
    glVertex2f(470.0f, 440.0f);
    glVertex2f(495.0f, 465.0f);

    // Shadow Face (Right of ridge)
    setColor(snowShd[0], snowShd[1], snowShd[2]);
    // Main upper shadow triangle
    glVertex2f(500.0f, 540.0f);
    glVertex2f(495.0f, 465.0f);
    glVertex2f(570.0f, 499.0f);

    // Right lower tongue 1
    glVertex2f(570.0f, 499.0f);
    glVertex2f(545.0f, 475.0f);
    glVertex2f(590.0f, 470.0f);

    // Infill body between tongue 1 and ridge
    glVertex2f(570.0f, 499.0f);
    glVertex2f(495.0f, 465.0f);
    glVertex2f(545.0f, 475.0f);

    // Lower tongue 2 extending down shadow couloir
    glVertex2f(545.0f, 475.0f);
    glVertex2f(495.0f, 465.0f);
    glVertex2f(520.0f, 445.0f);

    // --- 2. Left Sub-Peak Snowcap (Apex at 320.0, 420.0) ---
    // Lit Face
    setColor(snowLit[0] * 0.96f, snowLit[1] * 0.96f, snowLit[2] * 0.96f);
    glVertex2f(320.0f, 420.0f);
    glVertex2f(265.0f, 380.0f);
    glVertex2f(310.0f, 370.0f);

    glVertex2f(265.0f, 380.0f);
    glVertex2f(245.0f, 355.0f);
    glVertex2f(285.0f, 360.0f);

    glVertex2f(265.0f, 380.0f);
    glVertex2f(285.0f, 360.0f);
    glVertex2f(310.0f, 370.0f);

    // Shadow Face
    setColor(snowShd[0] * 0.96f, snowShd[1] * 0.96f, snowShd[2] * 0.96f);
    glVertex2f(320.0f, 420.0f);
    glVertex2f(310.0f, 370.0f);
    glVertex2f(375.0f, 385.0f);

    glVertex2f(310.0f, 370.0f);
    glVertex2f(340.0f, 350.0f);
    glVertex2f(375.0f, 385.0f);

    // --- 3. Right Sub-Peak Snowcap (Apex at 690.0, 430.0) ---
    // Lit Face
    setColor(snowLit[0] * 0.94f, snowLit[1] * 0.94f, snowLit[2] * 0.94f);
    glVertex2f(690.0f, 430.0f);
    glVertex2f(635.0f, 385.0f);
    glVertex2f(680.0f, 375.0f);

    glVertex2f(635.0f, 385.0f);
    glVertex2f(615.0f, 360.0f);
    glVertex2f(655.0f, 365.0f);

    glVertex2f(635.0f, 385.0f);
    glVertex2f(655.0f, 365.0f);
    glVertex2f(680.0f, 375.0f);

    // Shadow Face
    setColor(snowShd[0] * 0.94f, snowShd[1] * 0.94f, snowShd[2] * 0.94f);
    glVertex2f(690.0f, 430.0f);
    glVertex2f(680.0f, 375.0f);
    glVertex2f(750.0f, 385.0f);

    glVertex2f(680.0f, 375.0f);
    glVertex2f(715.0f, 350.0f);
    glVertex2f(750.0f, 385.0f);

    glEnd();
}

// Draw Alpine Cabin with blinking/flickering lantern window on mountain shelf
void drawAlpineCabin() {
    const float cx = 350.0f;
    const float cy = 240.0f;

    // Cabin Walls (2 triangles forming quad)
    setColor(0.20f, 0.14f, 0.10f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx, cy);
    glVertex2f(cx + 34.0f, cy);
    glVertex2f(cx + 34.0f, cy + 22.0f);

    glVertex2f(cx, cy);
    glVertex2f(cx + 34.0f, cy + 22.0f);
    glVertex2f(cx, cy + 22.0f);

    // Triangular Roof
    setColor(0.12f, 0.08f, 0.06f);
    glVertex2f(cx - 4.0f, cy + 22.0f);
    glVertex2f(cx + 38.0f, cy + 22.0f);
    glVertex2f(cx + 17.0f, cy + 36.0f);
    glEnd();

    // Blinking / Flickering Lantern Window:
    // Color toggles between bright golden amber and warmer dim ember
    if (g_lanternPhase == 0) {
        setColor(1.00f, 0.88f, 0.35f); // Bright lantern glow
    } else if (g_lanternPhase == 1) {
        setColor(0.88f, 0.65f, 0.22f); // Warm mid ember
    } else {
        setColor(0.72f, 0.48f, 0.15f); // Dim flicker
    }

    const float wx = cx + 11.0f;
    const float wy = cy + 6.0f;
    const float ww = 12.0f;
    const float wh = 10.0f;

    glBegin(GL_TRIANGLES);
    glVertex2f(wx, wy);
    glVertex2f(wx + ww, wy);
    glVertex2f(wx + ww, wy + wh);

    glVertex2f(wx, wy);
    glVertex2f(wx + ww, wy + wh);
    glVertex2f(wx, wy + wh);
    glEnd();
}

// Draw Summit Communications Mast and Active Blinking Aviation Warning Beacon
void drawSummitBeacon() {
    const float peakX = 500.0f;
    const float peakY = 540.0f;
    const float mastH = 55.0f;
    const float beaconY = peakY + mastH;

    // Structural Mast Truss (lines and triangular bracing)
    glLineWidth(2.0f);
    setColor(0.12f, 0.12f, 0.15f);
    glBegin(GL_LINES);
    // Left and right vertical leg posts
    glVertex2f(peakX - 4.0f, peakY);
    glVertex2f(peakX - 1.5f, beaconY);

    glVertex2f(peakX + 4.0f, peakY);
    glVertex2f(peakX + 1.5f, beaconY);

    // Cross horizontal struts
    glVertex2f(peakX - 3.2f, peakY + 18.0f);
    glVertex2f(peakX + 3.2f, peakY + 18.0f);

    glVertex2f(peakX - 2.2f, peakY + 36.0f);
    glVertex2f(peakX + 2.2f, peakY + 36.0f);

    // Diagonal lattice cross bracing
    glVertex2f(peakX - 4.0f, peakY);
    glVertex2f(peakX + 3.2f, peakY + 18.0f);

    glVertex2f(peakX + 4.0f, peakY);
    glVertex2f(peakX - 3.2f, peakY + 18.0f);

    glVertex2f(peakX - 3.2f, peakY + 18.0f);
    glVertex2f(peakX + 2.2f, peakY + 36.0f);

    glVertex2f(peakX + 3.2f, peakY + 18.0f);
    glVertex2f(peakX - 2.2f, peakY + 36.0f);
    glEnd();

    // --- BLINKING AVIATION WARNING BEACON ---
    const float beaconRadius = 5.0f;
    const int segments = 16;

    if (g_beaconActive) {
        // ACTIVE STATE: Radiant Aviation Warning Crimson Red + Radial Glow Halo

        // 1. Radial Diffuse Halo (semi-transparent blended fan)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const float haloRadius = 24.0f;
        glBegin(GL_TRIANGLE_FAN);
        setColorA(1.0f, 0.20f, 0.20f, 0.45f); // Intense red core
        glVertex2f(peakX, beaconY);
        setColorA(1.0f, 0.20f, 0.20f, 0.00f); // Fully transparent edge
        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i * 2.0f * 3.14159265f / segments;
            glVertex2f(peakX + cosf(angle) * haloRadius,
                       beaconY + sinf(angle) * haloRadius);
        }
        glEnd();
        glDisable(GL_BLEND);

        // 2. Beacon Core Lamp: High-intensity aviation red
        setColor(1.00f, 0.10f, 0.10f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(peakX, beaconY);
        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i * 2.0f * 3.14159265f / segments;
            glVertex2f(peakX + cosf(angle) * beaconRadius,
                       beaconY + sinf(angle) * beaconRadius);
        }
        glEnd();

        // 3. Central Hotspot: Radiant white-amber filament
        setColor(1.00f, 0.90f, 0.85f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(peakX, beaconY);
        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i * 2.0f * 3.14159265f / segments;
            glVertex2f(peakX + cosf(angle) * (beaconRadius * 0.45f),
                       beaconY + sinf(angle) * (beaconRadius * 0.45f));
        }
        glEnd();

    } else {
        // INACTIVE / DORMANT STATE: Dark dormant ruby-red lamp
        setColor(0.35f, 0.05f, 0.05f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(peakX, beaconY);
        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i * 2.0f * 3.14159265f / segments;
            glVertex2f(peakX + cosf(angle) * beaconRadius,
                       beaconY + sinf(angle) * beaconRadius);
        }
        glEnd();
    }
}

// Draw foothills and foreground terrain along the base of the mountains.
// Every hill's base sits on the ground line so it rises out of the meadow
// (the meadow band below covers the bases), and the outer hills extend past
// the window edges so they roll off-screen instead of ending at the frame.
void drawFoothills() {
    const float GROUND_Y = 110.0f;

    glBegin(GL_TRIANGLES);

    // Left rolling foothills (back, then front)
    setColor(0.12f, 0.18f, 0.16f);
    glVertex2f(-150.0f, GROUND_Y);
    glVertex2f(300.0f,  GROUND_Y);
    glVertex2f(120.0f,  215.0f);

    setColor(0.09f, 0.14f, 0.12f);
    glVertex2f(-100.0f, GROUND_Y);
    glVertex2f(340.0f,  GROUND_Y);
    glVertex2f(170.0f,  178.0f);

    // Right rolling foothills (back, then front)
    setColor(0.10f, 0.16f, 0.14f);
    glVertex2f(680.0f,  GROUND_Y);
    glVertex2f(1150.0f, GROUND_Y);
    glVertex2f(850.0f,  218.0f);

    setColor(0.08f, 0.13f, 0.11f);
    glVertex2f(640.0f,  GROUND_Y);
    glVertex2f(1100.0f, GROUND_Y);
    glVertex2f(820.0f,  172.0f);

    // Central meadow band the forest stands on
    setColor(0.08f, 0.13f, 0.12f);
    glVertex2f(0.0f, GROUND_Y);
    glVertex2f((float)WINDOW_WIDTH, GROUND_Y);
    glVertex2f((float)WINDOW_WIDTH, 150.0f);

    glVertex2f(0.0f, GROUND_Y);
    glVertex2f((float)WINDOW_WIDTH, 150.0f);
    glVertex2f(0.0f, 150.0f);

    glEnd();
}

// Draw a single stylized evergreen pine tree using stacked triangle tiers
void drawPineTree(float x, float baseY, float width, float height) {
    // Tree Trunk (2 triangles)
    float trunkW = width * 0.20f;
    float trunkH = height * 0.22f;
    setColor(0.15f, 0.10f, 0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - trunkW * 0.5f, baseY);
    glVertex2f(x + trunkW * 0.5f, baseY);
    glVertex2f(x + trunkW * 0.5f, baseY + trunkH);

    glVertex2f(x - trunkW * 0.5f, baseY);
    glVertex2f(x + trunkW * 0.5f, baseY + trunkH);
    glVertex2f(x - trunkW * 0.5f, baseY + trunkH);
    glEnd();

    // 3 Stacked Foliage Tiers using Triangles (split into lit/shaded half for volume)
    const float tierBottomY[3] = {
        baseY + trunkH * 0.7f,
        baseY + height * 0.42f,
        baseY + height * 0.68f
    };
    const float tierWidth[3] = {
        width,
        width * 0.78f,
        width * 0.56f
    };
    const float tierHeight[3] = {
        height * 0.45f,
        height * 0.40f,
        height * 0.38f
    };

    for (int t = 0; t < 3; ++t) {
        float bY = tierBottomY[t];
        float tW = tierWidth[t];
        float tH = tierHeight[t];
        float apexY = bY + tH;

        glBegin(GL_TRIANGLES);
        // Left half (lit forest green)
        setColor(0.08f, 0.22f, 0.14f);
        glVertex2f(x, apexY);
        glVertex2f(x - tW * 0.5f, bY);
        glVertex2f(x, bY + tH * 0.15f);

        // Right half (shaded deep forest green)
        setColor(0.04f, 0.14f, 0.09f);
        glVertex2f(x, apexY);
        glVertex2f(x, bY + tH * 0.15f);
        glVertex2f(x + tW * 0.5f, bY);
        glEnd();
    }
}

// Draw foreground forest belt adding scale, proportion, and depth
void drawForest() {
    // Array of tree placements along the base of the mountains
    struct TreeDef {
        float x;
        float y;
        float w;
        float h;
    };

    const TreeDef trees[] = {
        {  45.0f, 130.0f, 26.0f, 65.0f },
        {  90.0f, 125.0f, 30.0f, 75.0f },
        { 145.0f, 120.0f, 28.0f, 70.0f },
        { 190.0f, 128.0f, 24.0f, 58.0f },
        { 240.0f, 122.0f, 26.0f, 62.0f },
        { 290.0f, 118.0f, 32.0f, 80.0f },
        { 340.0f, 125.0f, 22.0f, 55.0f },
        { 410.0f, 115.0f, 30.0f, 72.0f },
        { 460.0f, 120.0f, 26.0f, 64.0f },
        { 530.0f, 118.0f, 28.0f, 68.0f },
        { 590.0f, 124.0f, 24.0f, 60.0f },
        { 650.0f, 116.0f, 32.0f, 78.0f },
        { 710.0f, 122.0f, 26.0f, 65.0f },
        { 765.0f, 128.0f, 24.0f, 58.0f },
        { 820.0f, 118.0f, 30.0f, 74.0f },
        { 875.0f, 124.0f, 28.0f, 68.0f },
        { 930.0f, 128.0f, 26.0f, 62.0f },
        { 970.0f, 120.0f, 28.0f, 70.0f }
    };
    const int count = sizeof(trees) / sizeof(trees[0]);

    for (int i = 0; i < count; ++i) {
        drawPineTree(trees[i].x, trees[i].y, trees[i].w, trees[i].h);
    }
}

// Draw solid foreground meadow from the ground line to the bottom edge:
// a gradient that continues the meadow band's color and darkens toward the
// viewer, with two low rolling mounds for depth.
void drawForegroundMeadow() {
    const float GROUND_Y = 110.0f;

    glBegin(GL_TRIANGLES);

    // Ground gradient (2 triangles): meadow green at the ground line fading
    // to deep shadow at the bottom of the window
    setColor(0.08f, 0.13f, 0.12f);
    glVertex2f(0.0f, GROUND_Y);
    glVertex2f((float)WINDOW_WIDTH, GROUND_Y);
    setColor(0.03f, 0.06f, 0.05f);
    glVertex2f((float)WINDOW_WIDTH, 0.0f);

    setColor(0.08f, 0.13f, 0.12f);
    glVertex2f(0.0f, GROUND_Y);
    setColor(0.03f, 0.06f, 0.05f);
    glVertex2f((float)WINDOW_WIDTH, 0.0f);
    glVertex2f(0.0f, 0.0f);

    // Low rolling mounds in the foreground (extend past the window edges)
    setColor(0.06f, 0.11f, 0.09f);
    glVertex2f(-100.0f, 0.0f);
    glVertex2f(420.0f,  0.0f);
    glVertex2f(160.0f,  55.0f);

    setColor(0.05f, 0.10f, 0.08f);
    glVertex2f(560.0f,  0.0f);
    glVertex2f(1100.0f, 0.0f);
    glVertex2f(820.0f,  48.0f);

    glEnd();
}

// Master scene render assembly
void renderScene() {
    drawSky();
    drawStars();
    drawMoon();
    drawDistantMountainRange();
    drawMainMountainMassif();
    drawSnowcaps();
    drawAlpineCabin();
    drawSummitBeacon();
    drawFoothills();
    drawForegroundMeadow();
    drawForest();
}

// FreeGLUT Display Callback
void displayCallback() {
    glClear(GL_COLOR_BUFFER_BIT);
    renderScene();
    glutSwapBuffers();
}

// FreeGLUT Reshape Callback for 2D Orthographic Projection
void reshapeCallback(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 2D orthographic projection: (0,0) at bottom-left, (1000, 700) at top-right
    gluOrtho2D(0.0, (double)WINDOW_WIDTH, 0.0, (double)WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// FreeGLUT Timer Callback for Periodic Blinking Animation
void timerCallback(int value) {
    (void)value;
    g_timerTick++;

    // Beacon blinks every 2 ticks (500 ms cycle: on for 500ms, off for 500ms)
    if (g_timerTick % 2 == 0) {
        g_beaconActive = !g_beaconActive;
    }

    // Cabin lantern phase advances every tick
    g_lanternPhase = (g_lanternPhase + 1) % 3;

    // Request immediate window redraw with updated state
    glutPostRedisplay();

    // Re-register timer for perpetual animation loop
    glutTimerFunc(TIMER_INTERVAL_MS, timerCallback, 0);
}

// Keyboard Callback (ESC or 'q' to exit)
void keyboardCallback(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    if (key == 27 || key == 'q' || key == 'Q') {
        exit(0);
    }
}

// Self-contained BMP image exporter (for verification and test capture)
void exportBmp(const char *filename) {
    glClear(GL_COLOR_BUFFER_BIT);
    renderScene();
    glFinish();

    std::vector<unsigned char> pixels(WINDOW_WIDTH * WINDOW_HEIGHT * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << " for writing.\n";
        return;
    }

    const int padding = (4 - (WINDOW_WIDTH * 3) % 4) % 4;
    const int rowSize = WINDOW_WIDTH * 3 + padding;
    const int imageSize = rowSize * WINDOW_HEIGHT;
    const int fileSize = 54 + imageSize;

    unsigned char bmpHeader[54] = {
        'B', 'M',
        (unsigned char)(fileSize), (unsigned char)(fileSize >> 8), (unsigned char)(fileSize >> 16), (unsigned char)(fileSize >> 24),
        0, 0, 0, 0,
        54, 0, 0, 0, // Header size
        40, 0, 0, 0, // Info header size
        (unsigned char)(WINDOW_WIDTH), (unsigned char)(WINDOW_WIDTH >> 8), (unsigned char)(WINDOW_WIDTH >> 16), (unsigned char)(WINDOW_WIDTH >> 24),
        (unsigned char)(WINDOW_HEIGHT), (unsigned char)(WINDOW_HEIGHT >> 8), (unsigned char)(WINDOW_HEIGHT >> 16), (unsigned char)(WINDOW_HEIGHT >> 24),
        1, 0, // Color planes
        24, 0, // Bits per pixel
        0, 0, 0, 0, // Compression
        (unsigned char)(imageSize), (unsigned char)(imageSize >> 8), (unsigned char)(imageSize >> 16), (unsigned char)(imageSize >> 24),
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    file.write((char*)bmpHeader, 54);
    std::vector<unsigned char> pad(padding, 0);
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        file.write((char*)&pixels[y * WINDOW_WIDTH * 3], WINDOW_WIDTH * 3);
        if (padding > 0) {
            file.write((char*)pad.data(), padding);
        }
    }

    std::cout << "Render successfully exported to " << filename << "\n";
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(WINDOW_TITLE);

    // Initial background clear
    glClearColor(0.08f, 0.09f, 0.20f, 1.0f);

    // Register Callbacks
    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    reshapeCallback(WINDOW_WIDTH, WINDOW_HEIGHT);

    // Optional command line flag for exporting single frame
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--export") == 0 && i + 1 < argc) {
            exportBmp(argv[i + 1]);
            return 0;
        }
    }

    // Register 250ms periodic timer callback for animations
    glutTimerFunc(TIMER_INTERVAL_MS, timerCallback, 0);

    // Enter FreeGLUT Event Dispatching Loop
    glutMainLoop();
    return 0;
}
