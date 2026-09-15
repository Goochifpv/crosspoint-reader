#pragma once

void addPagesRead(int pages);

void startExpedition();
void finishExpedition();
void abandonCurrentAdventure();

bool isBossMilestone(int stepNumber);
bool generateBoss(int stepNumber);

void generateBattle();
void generateEvent();

void performAdventureStep(int direction);
