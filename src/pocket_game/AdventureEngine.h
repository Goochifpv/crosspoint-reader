#pragma once

// Canonical TFT v19 gameplay engine, now UI-agnostic for CrossPoint/X4.

void addPagesRead(int pages);

void startExpedition();
void finishExpedition();
void abandonCurrentAdventure();

bool isBossMilestone(int stepNumber);
bool generateBoss(int stepNumber);

void generateEvent();

// Spend one banked Adventure Step and generate the resulting event.
// The caller is responsible for rendering currentEvent afterwards.
void performAdventureStep(int direction);
