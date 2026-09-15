#pragma once

#include <Arduino.h>


// ==================================================
// DISPLAY TEXT FOR ONE EVENT
// ==================================================

struct EventText {
  String title;
  String line1;
  String line2;
};


// ==================================================
// BOSS DISPLAY TEXT
// ==================================================

struct BossText {
  String name;
  String encounter;
};


// ==================================================
// LOCATION EVENT CONTENT
// ==================================================

EventText contentTravel(
  int location,
  const String &actorName
);

EventText contentBattle(
  int location
);

EventText contentFriendly(
  int location
);

EventText contentTreasure(
  int location
);

EventText contentNPC(
  int location
);

EventText contentDiscovery(
  int location
);

EventText contentRest(
  int location
);

EventText contentRare(
  int location
);


// ==================================================
// CONTEXTUAL ART KEY
//
// Returns a stable scene/subject key for authored
// events. Empty string means use normal art fallbacks.
// ==================================================

String contentArtKeyFor(
  int location,
  const String &eventTitle,
  int eventType
);


// ==================================================
// BOSS CONTENT
// ==================================================

BossText contentBoss(
  int location,
  int bossIndex
);


// ==================================================
// CONTEXTUAL CHARACTER OUTCOMES
// ==================================================

String contentBattleVictory(
  const String &name
);

String contentBattleRetreat();

String contentFriendlyResult(
  const String &name
);

String contentTreasureResult(
  const String &name
);

String contentNPCResult(
  const String &name
);

String contentDiscoveryResult(
  const String &name
);

String contentRestResult(
  const String &name
);

String contentRareResult(
  const String &name
);