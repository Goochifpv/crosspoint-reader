#pragma once

#include <Arduino.h>

#define POCKET_READER_COLLECTION_API_VERSION 19

// ==================================================
// VERSION
// ==================================================

constexpr int SAVE_VERSION = 19;


// ==================================================
// READING
// ==================================================

constexpr int PAGES_PER_STEP = 10;


// ==================================================
// PARTY / ROSTER
// ==================================================

constexpr int TEAM_SIZE = 3;
constexpr int ROSTER_SIZE = 32;

constexpr int NORMAL_COMPANION_COUNT = 23;
constexpr int SPECIAL_COMPANION_COUNT = 9;
constexpr int STARTING_COMPANION_COUNT = 3;

// Stable roster indices. Nany and Buba remain at their
// historic indices so existing saves keep the correct
// character progression and party membership.
constexpr int NANY_ROSTER_INDEX = 11;
constexpr int BUBA_ROSTER_INDEX = 3;
constexpr int JEM_ROSTER_INDEX = 25;
constexpr int GRAN_ROSTER_INDEX = 26;
constexpr int POPS_ROSTER_INDEX = 27;
constexpr int JAKEY_ROSTER_INDEX = 28;
constexpr int BOOBOO_ROSTER_INDEX = 29;
constexpr int SIAN_ROSTER_INDEX = 30;
constexpr int GOOCHI_ROSTER_INDEX = 31;

// First earned level in a role needs 50 XP.
// Second needs 100, third 150, and so on.
constexpr int ROLE_XP_BASE = 50;


// ==================================================
// LOCATIONS
// ==================================================

constexpr int LOCATION_COUNT = 8;


// ==================================================
// AUTO TIMING
// ==================================================

constexpr unsigned long AUTO_EVENT_TIME = 3600;
constexpr unsigned long AUTO_BETWEEN_TIME = 1000;


// ==================================================
// PARTY ROLES
// ==================================================

enum PartyRole {
  ROLE_HEART = 0,
  ROLE_STRENGTH = 1,
  ROLE_LUCK = 2
};


// ==================================================
// LOCATIONS
// ==================================================

// Stable internal IDs. These deliberately do NOT match
// player progression order. Keeping them stable preserves
// saves, event-content arrays and SD-card artwork paths.
//
// Canonical progression order is exposed through
// locationForProgressionPosition().
enum LocationId {

  LOC_WHISPERWOOD,

  LOC_OPEN_PLAINS,

  LOC_DUSTY_DESERT,

  LOC_STORM_COAST,

  LOC_MISTY_MARSH,

  LOC_FROSTPEAK,

  LOC_VOLCANIC_HIGHLANDS,

  LOC_SKYREACH
};


// ==================================================
// TRAVEL DIRECTIONS
// ==================================================

enum TravelDirection {
  DIR_NORTH = 0,
  DIR_EAST = 1,
  DIR_SOUTH = 2,
  DIR_WEST = 3,
  DIR_NONE = -1
};


// ==================================================
// EVENT TYPES
// ==================================================

enum EventType {
  BATTLE,
  FRIENDLY_ENCOUNTER,
  TREASURE,
  POTION_MAKER,
  DISCOVERY,
  REST_EVENT,
  RARE_ENCOUNTER,
  TRAVEL_EVENT,
  BOSS_EVENT
};


// ==================================================
// COMPANION
// ==================================================

struct Companion {
  String name;

  // Innate card aptitude.
  // These values never increase and match the
  // Strength / Heart / Luck numbers printed on the
  // collectible card.
  int baseStrength;
  int baseHeart;
  int baseLuck;

  // Earned training is separate from innate aptitude.
  //
  // Effective role value =
  // card aptitude + earned role levels.
  //
  // Overall Level =
  // earnedStrength + earnedHeart + earnedLuck.
  int earnedStrength;
  int strengthXP;

  int earnedHeart;
  int heartXP;

  int earnedLuck;
  int luckXP;

  int restSteps;
  int knockoutCount;
};


// ==================================================
// ADVENTURE EVENT
// ==================================================

struct AdventureEvent {
  EventType type;

  String title;
  String line1;
  String line2;
  String result;

  int xpDelta[TEAM_SIZE];

  bool pauseAuto;
};


// ==================================================
// GLOBAL GAME DATA
// ==================================================

extern Companion roster[ROSTER_SIZE];

extern int teamSlots[TEAM_SIZE];

extern AdventureEvent currentEvent;


// ==================================================
// COMPANION COLLECTION
//
// unlockOrder contains the complete 32-character
// acquisition order.
//
// Fresh v18 saves begin with 23 normal companions in
// shuffled order followed by the nine special entries:
//
// Nany, Buba, Jem, Gran, Pops, Jakey, BooBoo, Sian,
// Goochi.
//
// Fixed boss rewards may move a special into the owned
// prefix later; roster indices themselves never change.
//
// unlockedCompanionCount tells us how much of that
// order the player currently owns.
// ==================================================

extern int companionUnlockOrder[ROSTER_SIZE];

extern int unlockedCompanionCount;


// ==================================================
// PENDING COMPANION REVEAL
//
// Ephemeral UI hand-off after a successful boss.
// -1 means there is nothing waiting to be revealed.
//
// Ownership itself is already saved immediately as
// part of the normal game save.
// ==================================================

extern int pendingCompanionReveal;

extern int pendingCompanionRevealNext;

void queueCompanionReveal(
  int rosterIndex
);

bool promoteNextCompanionReveal();


// ==================================================
// RESERVE ORDER
//
// This is the player's visible Reserve list order.
// New unlocks are appended to the bottom.
// Active <-> Reserve swaps exchange positions rather
// than rebuilding the list from roster indices.
// ==================================================

extern int reserveOrder[ROSTER_SIZE];

extern int reserveCount;


// ==================================================
// READING
// ==================================================

extern int totalPagesRead;
extern int pagesTowardStep;
extern int bankedSteps;


// ==================================================
// LOCATIONS
// ==================================================

extern int currentLocation;

extern int locationProgress[LOCATION_COUNT];

extern bool locationUnlocked[LOCATION_COUNT];

extern bool locationCompleted[LOCATION_COUNT];


// ==================================================
// BOSS GALLERY / HISTORY
//
// Permanent collection record.
// Each location currently has four bosses:
// 25%, 50%, 75%, 100%.
// ==================================================

extern bool bossEncountered[LOCATION_COUNT][4];

extern bool bossDefeated[LOCATION_COUNT][4];

bool bossHasBeenEncountered(
  int location,
  int bossIndex
);

bool bossHasBeenDefeated(
  int location,
  int bossIndex
);

void markBossEncountered(
  int location,
  int bossIndex
);

void markBossDefeated(
  int location,
  int bossIndex
);

int bossEncounteredCount();

int bossDefeatedCount();

void rebuildBossHistoryFromProgress();


// ==================================================
// EXPEDITION
// ==================================================

extern bool expeditionActive;

extern int expeditionStep;

extern bool wipeoutPending;


// ==================================================
// AUTO
// ==================================================

extern bool autoProgress;


// ==================================================
// DIRECTION
// ==================================================

extern int lastDirection;

extern String lastTravelText;


// ==================================================
// COMPANION COLLECTION HELPERS
// ==================================================

void initializeCompanionCollection();

bool validCompanionCollection();

bool companionIsUnlocked(
  int rosterIndex
);

int companionAtUnlockPosition(
  int position
);

int companionUnlockPosition(
  int rosterIndex
);

int lockedCompanionCount();

bool companionCollectionComplete();

void buildReserveOrderFromUnlocked();

bool validReserveOrder();

int reserveMemberAt(
  int position
);

int reservePositionForRoster(
  int rosterIndex
);

bool swapActiveRoles(
  int firstRole,
  int secondRole
);

bool swapActiveWithReserve(
  int activeRole,
  int reservePosition
);

bool swapReservePositions(
  int firstPosition,
  int secondPosition
);


bool companionIsSpecial(
  int rosterIndex
);

int specialCompanionForLocation(
  int location
);

// Unlock a specific companion without changing stable
// roster indices. Returns the roster index, or -1 if
// the companion was already owned / invalid.
int unlockCompanionByRosterIndex(
  int rosterIndex
);

// Unlock the next still-locked NORMAL companion from
// the pre-generated normal order. Specials are never
// consumed by this function.
int unlockNextNormalCompanion();

// Legacy/general helper retained for compatibility.
int unlockNextCompanion();


// ==================================================
// ROLE HELPERS
// ==================================================

String roleName(int slot);

String roleShort(int slot);


// ==================================================
// LOCATION HELPERS
// ==================================================

// Canonical player-facing location title.
// Use for UI, Boss Gallery, location headers and future
// Adventure Journal / EPUB title/metadata output.
String locationName(int location);

bool validLocation(int location);

// Canonical player progression order:
//
// 0 Whispering Woods
// 1 Ancient Ruins
// 2 Misty Marshes
// 3 Storm Coast
// 4 Frostpeak Mountains
// 5 Emberfield Plains
// 6 Volcanic Badlands
// 7 Skyreach City
//
// UI ordering, unlocks, boss difficulty, special rewards
// and future Adventure EPUB ordering must use these helpers
// rather than assuming LocationId numeric order.
int locationForProgressionPosition(
  int position
);

int progressionPositionForLocation(
  int location
);

int firstUnlockedIncompleteLocation();

void unlockNextLocation();

void rebuildLocationUnlocksForProgressionOrder();

void markCurrentLocationComplete();

bool allLocationsCompleted();

bool allBossesDefeated();


// Bookended run model: explicitly leaving/switching
// away from an active location abandons only that run.
// Permanent progression is untouched.
void abandonCurrentAdventure();


// ==================================================
// DIRECTION HELPERS
// ==================================================

String directionName(int direction);

int oppositeDirection(int direction);

bool directionAllowed(int direction);

int randomAllowedDirection();


// ==================================================
// TEAM HELPERS
// ==================================================

bool isOnTeam(int rosterIndex);

int findSlotForRoster(int rosterIndex);

bool validTeamSlots();


// ==================================================
// EVENT HELPERS
// ==================================================

void clearEventXP();


// ==================================================
// ROLE PROGRESSION
//
// Card aptitude is innate.
// Earned role levels come only from adventuring in
// that role.
//
// Effective value = base aptitude + earned levels.
// Overall Level = sum of all three earned levels.
// ==================================================

void resetCompanionProgress();

int baseRoleLevel(
  int rosterIndex,
  int role
);

int earnedRoleLevel(
  int rosterIndex,
  int role
);

int effectiveRoleLevel(
  int rosterIndex,
  int role
);

int currentRoleXP(
  int rosterIndex,
  int role
);

int roleXPThreshold(
  int rosterIndex,
  int role
);

int overallLevel(
  int rosterIndex
);

int overallLifetimeXP(
  int rosterIndex
);

void addRoleXP(
  int rosterIndex,
  int role,
  int amount
);

void loseRoleXP(
  int rosterIndex,
  int role,
  int amount
);

void awardRoleXP(
  int heartXP,
  int strengthXP,
  int luckXP
);

void removeXPFromReadyTeam(
  int amount
);


// Legacy compatibility helpers.
// They are no longer used for boss checks.
void addXP(
  int rosterIndex,
  int amount
);

void loseXP(
  int rosterIndex,
  int amount
);

int companionPower(
  int rosterIndex
);

int teamPower();


// ==================================================
// AVAILABILITY
// ==================================================

int countReadyActive();

int countReadyRoster();

int randomReadySlot();


// ==================================================
// FATIGUE
// ==================================================

void clearExpeditionFatigue();
