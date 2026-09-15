#include <Arduino.h>
#include <esp_system.h>

#include "GameState.h"


// ==================================================
// ROSTER
//
// Normal companions come first so the initial random
// shuffle is simple and deterministic to save.
//
// Nany and Buba remain in their historic roster slots for
// save compatibility. Special status is defined by an
// explicit index list rather than physical array position.
// ==================================================

Companion roster[ROSTER_SIZE] = {

  // =================================================
  // ROSTER ENTRIES (23 normal + Nany/Buba special in-place)
  //
  // Values are canonical card aptitude:
  // Strength / Heart / Luck
  // =================================================

  {"Aero", 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Blossom", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Boru", 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Buba", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Bubbles", 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Chippy", 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Daisy", 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Eeyri", 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Kiko", 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Lumi", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Mochi", 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Nany", 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Noodle", 2, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Pebble", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Peepers", 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Pip", 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Pipkin", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Prickle", 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Shadow", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Snap", 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Stripe", 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Sunny", 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Toran", 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Truffle", 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Willow", 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0},


  // =================================================
  // REMAINING SPECIAL / ENDGAME ROSTER ENTRIES
  // Nany and Buba are above at their historic indices.
  // =================================================

  {"Jem", 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Gran", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Pops", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Jakey", 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"BooBoo", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Sian", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0},
  {"Goochi", 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0}
};


// ==================================================
// SPECIAL COMPANION ORDER
//
// This order is tied to PROGRESSION POSITION, not to a
// particular location identity:
//
// 1st 100% boss -> Nany
// 2nd           -> Buba
// 3rd           -> Jem
// 4th           -> Gran
// 5th           -> Pops
// 6th           -> Jakey
// 7th           -> BooBoo
// 8th           -> Sian
//
// Goochi is deliberately NOT tied to one location.
// Goochi is the all-locations / all-bosses reward.
//
// Locations can therefore be renamed/rethemed/reordered
// without changing the locked special-character sequence.
// ==================================================

static const int SPECIAL_COMPANIONS[
  SPECIAL_COMPANION_COUNT
] = {
  NANY_ROSTER_INDEX,
  BUBA_ROSTER_INDEX,
  JEM_ROSTER_INDEX,
  GRAN_ROSTER_INDEX,
  POPS_ROSTER_INDEX,
  JAKEY_ROSTER_INDEX,
  BOOBOO_ROSTER_INDEX,
  SIAN_ROSTER_INDEX,
  GOOCHI_ROSTER_INDEX
};


// ==================================================
// PARTY
//
// These are safe compile-time defaults only.
// A fresh save replaces them with the first three
// entries in the randomly generated unlock order.
// ==================================================

int teamSlots[TEAM_SIZE] = {
  0,
  1,
  2
};


// ==================================================
// COMPANION COLLECTION
// ==================================================

int companionUnlockOrder[ROSTER_SIZE] = {
  0
};


int unlockedCompanionCount =
  0;


// ==================================================
// PENDING COMPANION REVEAL
// ==================================================

int pendingCompanionReveal =
  -1;


int pendingCompanionRevealNext =
  -1;


// ==================================================
// COMPANION REVEAL QUEUE
// ==================================================

void queueCompanionReveal(
  int rosterIndex
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return;
  }


  if (
    pendingCompanionReveal < 0
  ) {

    pendingCompanionReveal =
      rosterIndex;


    return;
  }


  if (
    pendingCompanionRevealNext < 0
  ) {

    pendingCompanionRevealNext =
      rosterIndex;
  }
}


bool promoteNextCompanionReveal() {

  if (
    pendingCompanionReveal >= 0 ||
    pendingCompanionRevealNext < 0
  ) {

    return false;
  }


  pendingCompanionReveal =
    pendingCompanionRevealNext;


  pendingCompanionRevealNext =
    -1;


  return true;
}


// ==================================================
// RESERVE ORDER
// ==================================================

int reserveOrder[ROSTER_SIZE] = {
  -1
};


int reserveCount =
  0;


// ==================================================
// CURRENT EVENT
// ==================================================

AdventureEvent currentEvent;


// ==================================================
// READING
// ==================================================

int totalPagesRead = 0;

int pagesTowardStep = 0;

int bankedSteps = 0;


// ==================================================
// LOCATIONS
// ==================================================

int currentLocation =
  LOC_WHISPERWOOD;


int locationProgress[LOCATION_COUNT] = {
  0, 0, 0, 0,
  0, 0, 0, 0
};


bool locationUnlocked[LOCATION_COUNT] = {
  true, false, false, false,
  false, false, false, false
};


bool locationCompleted[LOCATION_COUNT] = {
  false, false, false, false,
  false, false, false, false
};


// ==================================================
// BOSS GALLERY / HISTORY
// ==================================================

bool bossEncountered[LOCATION_COUNT][4] = {
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false}
};


bool bossDefeated[LOCATION_COUNT][4] = {
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false},
  {false, false, false, false}
};


// ==================================================
// EXPEDITION
// ==================================================

bool expeditionActive = false;

int expeditionStep = 0;

bool wipeoutPending = false;


// ==================================================
// AUTO
// ==================================================

bool autoProgress = false;


// ==================================================
// DIRECTION
// ==================================================

int lastDirection =
  DIR_NONE;


String lastTravelText =
  "";


// ==================================================
// RESET COMPANION PROGRESSION
//
// Used only when creating/migrating to the new
// 32-character collection.
// ==================================================

void resetCompanionProgress() {

  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    roster[i].earnedStrength =
      0;


    roster[i].strengthXP =
      0;


    roster[i].earnedHeart =
      0;


    roster[i].heartXP =
      0;


    roster[i].earnedLuck =
      0;


    roster[i].luckXP =
      0;


    roster[i].restSteps =
      0;


    roster[i].knockoutCount =
      0;
  }
}


// ==================================================
// INITIALIZE COMPANION COLLECTION
//
// 1. Build and shuffle the 23 normal companions.
// 2. First three become the starter team.
// 3. Remaining 20 normals become future boss rewards.
// 4. Append the nine special companions in fixed order.
//
// This runs ONCE for a new/migrated save. The full
// order is persisted afterwards, so rebooting never
// changes future unlocks.
// ==================================================

void initializeCompanionCollection() {

  resetCompanionProgress();


  reserveCount =
    0;


  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    reserveOrder[i] =
      -1;
  }


  int normalPool[
    NORMAL_COMPANION_COUNT
  ];


  int normalCount =
    0;


  for (
    int rosterIndex = 0;
    rosterIndex < ROSTER_SIZE;
    rosterIndex++
  ) {

    bool special =
      false;


    for (
      int i = 0;
      i < SPECIAL_COMPANION_COUNT;
      i++
    ) {

      if (
        SPECIAL_COMPANIONS[i] ==
        rosterIndex
      ) {

        special =
          true;


        break;
      }
    }


    if (
      !special &&
      normalCount <
      NORMAL_COMPANION_COUNT
    ) {

      normalPool[
        normalCount
      ] =
        rosterIndex;


      normalCount++;
    }
  }


  // Fisher-Yates shuffle of ONLY the 23 normal
  // companions. Specials can never become starters.

  for (
    int i =
      NORMAL_COMPANION_COUNT - 1;
    i > 0;
    i--
  ) {

    int j =
      esp_random() %
      (
        i + 1
      );


    int temporary =
      normalPool[i];


    normalPool[i] =
      normalPool[j];


    normalPool[j] =
      temporary;
  }


  for (
    int i = 0;
    i < NORMAL_COMPANION_COUNT;
    i++
  ) {

    companionUnlockOrder[i] =
      normalPool[i];
  }


  // Append the nine specials in their canonical order.
  // Actual acquisition order can later change when a
  // fixed 100% boss reward is earned; the unlock helper
  // safely moves that member into the owned prefix.

  for (
    int i = 0;
    i < SPECIAL_COMPANION_COUNT;
    i++
  ) {

    companionUnlockOrder[
      NORMAL_COMPANION_COUNT +
      i
    ] =
      SPECIAL_COMPANIONS[i];
  }


  unlockedCompanionCount =
    STARTING_COMPANION_COUNT;


  for (
    int slot = 0;
    slot < TEAM_SIZE;
    slot++
  ) {

    teamSlots[slot] =
      companionUnlockOrder[
        slot
      ];
  }
}


// ==================================================
// VALID COMPANION COLLECTION
// ==================================================

bool validCompanionCollection() {

  if (
    unlockedCompanionCount <
      STARTING_COMPANION_COUNT ||
    unlockedCompanionCount >
      ROSTER_SIZE
  ) {

    return false;
  }


  // v18 deliberately accepts any unique acquisition
  // order. This preserves v17 saves where Nany/Buba
  // may already have appeared in the old normal pool,
  // while fresh v18 saves use the new fixed rules.

  bool seen[
    ROSTER_SIZE
  ] = {
    false
  };


  for (
    int position = 0;
    position < ROSTER_SIZE;
    position++
  ) {

    int member =
      companionUnlockOrder[
        position
      ];


    if (
      member < 0 ||
      member >= ROSTER_SIZE ||
      seen[
        member
      ]
    ) {

      return false;
    }


    seen[
      member
    ] =
      true;
  }


  return true;
}


// ==================================================
// IS COMPANION UNLOCKED?
// ==================================================

bool companionIsUnlocked(
  int rosterIndex
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return false;
  }


  for (
    int position = 0;
    position <
      unlockedCompanionCount;
    position++
  ) {

    if (
      companionUnlockOrder[
        position
      ] ==
      rosterIndex
    ) {

      return true;
    }
  }


  return false;
}


// ==================================================
// COMPANION AT ACQUISITION POSITION
// ==================================================

int companionAtUnlockPosition(
  int position
) {

  if (
    position < 0 ||
    position >= ROSTER_SIZE
  ) {

    return -1;
  }


  return companionUnlockOrder[
    position
  ];
}


// ==================================================
// FIND ACQUISITION POSITION
// ==================================================

int companionUnlockPosition(
  int rosterIndex
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return -1;
  }


  for (
    int position = 0;
    position < ROSTER_SIZE;
    position++
  ) {

    if (
      companionUnlockOrder[
        position
      ] ==
      rosterIndex
    ) {

      return position;
    }
  }


  return -1;
}


// ==================================================
// LOCKED COUNT
// ==================================================

int lockedCompanionCount() {

  return
    ROSTER_SIZE -
    unlockedCompanionCount;
}


// ==================================================
// COLLECTION COMPLETE
// ==================================================

bool companionCollectionComplete() {

  return
    unlockedCompanionCount >=
    ROSTER_SIZE;
}


// ==================================================
// BUILD RESERVE ORDER FROM UNLOCKED COLLECTION
//
// Used for migration/safety only.
//
// It preserves acquisition order as closely as
// possible by walking the saved unlock order, while
// omitting the three companions currently Active.
// ==================================================

void buildReserveOrderFromUnlocked() {

  reserveCount =
    0;


  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    reserveOrder[i] =
      -1;
  }


  for (
    int position = 0;
    position < unlockedCompanionCount;
    position++
  ) {

    int member =
      companionUnlockOrder[
        position
      ];


    if (
      isOnTeam(
        member
      )
    ) {

      continue;
    }


    reserveOrder[
      reserveCount
    ] =
      member;


    reserveCount++;
  }
}


// ==================================================
// VALID RESERVE ORDER
// ==================================================

bool validReserveOrder() {

  int expectedCount =
    unlockedCompanionCount -
    TEAM_SIZE;


  if (
    expectedCount <
    0
  ) {

    expectedCount =
      0;
  }


  if (
    reserveCount !=
    expectedCount
  ) {

    return false;
  }


  bool seen[
    ROSTER_SIZE
  ] = {
    false
  };


  for (
    int i = 0;
    i < reserveCount;
    i++
  ) {

    int member =
      reserveOrder[
        i
      ];


    if (
      member < 0 ||
      member >= ROSTER_SIZE
    ) {

      return false;
    }


    if (
      !companionIsUnlocked(
        member
      )
    ) {

      return false;
    }


    if (
      isOnTeam(
        member
      )
    ) {

      return false;
    }


    if (
      seen[
        member
      ]
    ) {

      return false;
    }


    seen[
      member
    ] =
      true;
  }


  // Every unlocked non-active character must appear
  // exactly once in Reserve.

  for (
    int position = 0;
    position < unlockedCompanionCount;
    position++
  ) {

    int member =
      companionUnlockOrder[
        position
      ];


    if (
      isOnTeam(
        member
      )
    ) {

      continue;
    }


    if (
      !seen[
        member
      ]
    ) {

      return false;
    }
  }


  return true;
}


// ==================================================
// RESERVE MEMBER AT POSITION
// ==================================================

int reserveMemberAt(
  int position
) {

  if (
    position < 0 ||
    position >= reserveCount
  ) {

    return -1;
  }


  return reserveOrder[
    position
  ];
}


// ==================================================
// FIND RESERVE POSITION
// ==================================================

int reservePositionForRoster(
  int rosterIndex
) {

  for (
    int position = 0;
    position < reserveCount;
    position++
  ) {

    if (
      reserveOrder[
        position
      ] ==
      rosterIndex
    ) {

      return position;
    }
  }


  return -1;
}


// ==================================================
// SWAP ACTIVE ROLES
// ==================================================

bool swapActiveRoles(
  int firstRole,
  int secondRole
) {

  if (
    firstRole < 0 ||
    firstRole >= TEAM_SIZE ||
    secondRole < 0 ||
    secondRole >= TEAM_SIZE
  ) {

    return false;
  }


  if (
    firstRole ==
    secondRole
  ) {

    return false;
  }


  int temporary =
    teamSlots[
      firstRole
    ];


  teamSlots[
    firstRole
  ] =
    teamSlots[
      secondRole
    ];


  teamSlots[
    secondRole
  ] =
    temporary;


  return true;
}


// ==================================================
// SWAP ACTIVE WITH RESERVE
//
// This is a true position exchange:
// - Reserve member moves into the selected role.
// - Outgoing Active member occupies that exact Reserve
//   list position.
// ==================================================

bool swapActiveWithReserve(
  int activeRole,
  int reservePosition
) {

  if (
    activeRole < 0 ||
    activeRole >= TEAM_SIZE ||
    reservePosition < 0 ||
    reservePosition >= reserveCount
  ) {

    return false;
  }


  int activeMember =
    teamSlots[
      activeRole
    ];


  int reserveMember =
    reserveOrder[
      reservePosition
    ];


  if (
    reserveMember < 0 ||
    !companionIsUnlocked(
      reserveMember
    )
  ) {

    return false;
  }


  teamSlots[
    activeRole
  ] =
    reserveMember;


  reserveOrder[
    reservePosition
  ] =
    activeMember;


  return true;
}


// ==================================================
// SWAP TWO RESERVE POSITIONS
// ==================================================

bool swapReservePositions(
  int firstPosition,
  int secondPosition
) {

  if (
    firstPosition < 0 ||
    firstPosition >= reserveCount ||
    secondPosition < 0 ||
    secondPosition >= reserveCount
  ) {

    return false;
  }


  if (
    firstPosition ==
    secondPosition
  ) {

    return false;
  }


  int temporary =
    reserveOrder[
      firstPosition
    ];


  reserveOrder[
    firstPosition
  ] =
    reserveOrder[
      secondPosition
    ];


  reserveOrder[
    secondPosition
  ] =
    temporary;


  return true;
}


// ==================================================
// UNLOCK NEXT COMPANION
//
// Successful 100% boss clears call this.
//
// The next character in the pre-generated collection
// order becomes owned and is appended to the BOTTOM
// of the visible Reserve list.
//
// Returns roster index, or -1 if complete.
// ==================================================

bool companionIsSpecial(
  int rosterIndex
) {

  for (
    int i = 0;
    i < SPECIAL_COMPANION_COUNT;
    i++
  ) {

    if (
      SPECIAL_COMPANIONS[i] ==
      rosterIndex
    ) {

      return true;
    }
  }


  return false;
}


// ==================================================
// SPECIAL FOR LOCATION PROGRESSION POSITION
// ==================================================

int specialCompanionForLocation(
  int location
) {

  int position =
    progressionPositionForLocation(
      location
    );


  if (
    position < 0 ||
    position >=
      LOCATION_COUNT
  ) {

    return -1;
  }


  // The first eight entries in SPECIAL_COMPANIONS are
  // the locked 100%-boss reward sequence. Goochi is the
  // ninth entry and remains the meta-completion reward.
  return
    SPECIAL_COMPANIONS[
      position
    ];
}


// ==================================================
// RESET NEWLY UNLOCKED MEMBER
// ==================================================

static void resetNewlyUnlockedMember(
  int member
) {

  roster[
    member
  ].earnedStrength =
    0;


  roster[
    member
  ].strengthXP =
    0;


  roster[
    member
  ].earnedHeart =
    0;


  roster[
    member
  ].heartXP =
    0;


  roster[
    member
  ].earnedLuck =
    0;


  roster[
    member
  ].luckXP =
    0;


  roster[
    member
  ].restSteps =
    0;


  roster[
    member
  ].knockoutCount =
    0;
}


// ==================================================
// UNLOCK SPECIFIC COMPANION
//
// Ownership remains represented by the prefix of
// companionUnlockOrder. To award a fixed special out
// of sequence, move it into the next acquisition slot.
// Stable roster indices never change.
// ==================================================

int unlockCompanionByRosterIndex(
  int rosterIndex
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE ||
    companionIsUnlocked(
      rosterIndex
    )
  ) {

    return -1;
  }


  int sourcePosition =
    -1;


  for (
    int position =
      unlockedCompanionCount;
    position <
      ROSTER_SIZE;
    position++
  ) {

    if (
      companionUnlockOrder[
        position
      ] ==
      rosterIndex
    ) {

      sourcePosition =
        position;


      break;
    }
  }


  if (
    sourcePosition < 0
  ) {

    return -1;
  }


  int acquisitionPosition =
    unlockedCompanionCount;


  int displaced =
    companionUnlockOrder[
      acquisitionPosition
    ];


  companionUnlockOrder[
    acquisitionPosition
  ] =
    rosterIndex;


  companionUnlockOrder[
    sourcePosition
  ] =
    displaced;


  unlockedCompanionCount++;


  if (
    reserveCount <
      ROSTER_SIZE
  ) {

    reserveOrder[
      reserveCount
    ] =
      rosterIndex;


    reserveCount++;
  }


  resetNewlyUnlockedMember(
    rosterIndex
  );


  return rosterIndex;
}


// ==================================================
// UNLOCK NEXT NORMAL
// ==================================================

int unlockNextNormalCompanion() {

  for (
    int position =
      unlockedCompanionCount;
    position <
      ROSTER_SIZE;
    position++
  ) {

    int member =
      companionUnlockOrder[
        position
      ];


    if (
      !companionIsSpecial(
        member
      )
    ) {

      return
        unlockCompanionByRosterIndex(
          member
        );
    }
  }


  return -1;
}


// ==================================================
// UNLOCK NEXT COMPANION - LEGACY/GENERAL
// ==================================================

int unlockNextCompanion() {

  if (
    companionCollectionComplete()
  ) {

    return -1;
  }


  return
    unlockCompanionByRosterIndex(
      companionUnlockOrder[
        unlockedCompanionCount
      ]
    );
}


// ==================================================
// ROLE HELPERS
// ==================================================

String roleName(int slot) {
  if (slot == ROLE_HEART) {
    return "Heart";
  }

  if (slot == ROLE_STRENGTH) {
    return "Strength";
  }

  if (slot == ROLE_LUCK) {
    return "Luck";
  }

  return "?";
}


String roleShort(int slot) {
  if (slot == ROLE_HEART) {
    return "H";
  }

  if (slot == ROLE_STRENGTH) {
    return "S";
  }

  if (slot == ROLE_LUCK) {
    return "L";
  }

  return "?";
}


// ==================================================
// LOCATION HELPERS
// ==================================================

// Canonical progression order.
//
// Internal LocationId values remain stable for save/art/
// content compatibility. Never use raw numeric LocationId
// order for player progression.
static const int LOCATION_PROGRESSION_ORDER[
  LOCATION_COUNT
] = {
  LOC_WHISPERWOOD,
  LOC_OPEN_PLAINS,
  LOC_MISTY_MARSH,
  LOC_STORM_COAST,
  LOC_FROSTPEAK,
  LOC_DUSTY_DESERT,
  LOC_VOLCANIC_HIGHLANDS,
  LOC_SKYREACH
};


int locationForProgressionPosition(
  int position
) {

  if (
    position < 0 ||
    position >= LOCATION_COUNT
  ) {

    return -1;
  }


  return
    LOCATION_PROGRESSION_ORDER[
      position
    ];
}


int progressionPositionForLocation(
  int location
) {

  for (
    int position = 0;
    position < LOCATION_COUNT;
    position++
  ) {

    if (
      LOCATION_PROGRESSION_ORDER[
        position
      ] ==
        location
    ) {

      return position;
    }
  }


  return -1;
}


String locationName(int location) {

  // =================================================
  // CANONICAL PLAYER-FACING LOCATION TITLES
  //
  // This is the single source of truth for:
  // - Location Select
  // - expedition/location headers
  // - Boss Gallery
  // - boss detail pages
  // - future Adventure Journal / EPUB metadata
  //
  // Internal enums and SD-card artwork slugs remain
  // unchanged for save/art compatibility.
  // =================================================

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return
      "Whispering Woods";
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return
      "Ancient Ruins";
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return
      "Emberfield Plains";
  }


  if (
    location ==
    LOC_STORM_COAST
  ) {

    return
      "Storm Coast";
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return
      "Misty Marshes";
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return
      "Frostpeak Mountains";
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return
      "Volcanic Badlands";
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return
      "Skyreach City";
  }


  return
    "Unknown";
}


bool validLocation(int location) {
  return (
    location >= 0 &&
    location < LOCATION_COUNT
  );
}


int firstUnlockedIncompleteLocation() {

  for (
    int position = 0;
    position < LOCATION_COUNT;
    position++
  ) {

    int location =
      locationForProgressionPosition(
        position
      );


    if (
      locationUnlocked[
        location
      ] &&
      !locationCompleted[
        location
      ]
    ) {

      return location;
    }
  }


  return -1;
}


// ==================================================
// UNLOCK NEXT LOCATION
//
// Boss 50 calls this. "Next" means canonical progression
// position, never raw internal LocationId + 1.
// ==================================================

void unlockNextLocation() {

  int position =
    progressionPositionForLocation(
      currentLocation
    );


  int next =
    locationForProgressionPosition(
      position +
      1
    );


  if (
    next >=
      0
  ) {

    locationUnlocked[
      next
    ] =
      true;
  }
}


// ==================================================
// REBUILD LOCATION UNLOCKS FOR CANONICAL ORDER
//
// The 50% boss is the permanent unlock gate. Boss history
// survives abandoned/replayed bookended runs, so it is the
// authoritative source after save migration.
//
// Completed/active legacy locations are also preserved so
// migration never strands existing progress.
// ==================================================

void rebuildLocationUnlocksForProgressionOrder() {

  bool rebuilt[
    LOCATION_COUNT
  ] = {
    false, false, false, false,
    false, false, false, false
  };


  int first =
    locationForProgressionPosition(
      0
    );


  rebuilt[
    first
  ] =
    true;


  for (
    int position = 1;
    position < LOCATION_COUNT;
    position++
  ) {

    int previous =
      locationForProgressionPosition(
        position -
        1
      );


    int location =
      locationForProgressionPosition(
        position
      );


    bool previousGateCleared =
      bossHasBeenDefeated(
        previous,
        1
      ) ||
      locationCompleted[
        previous
      ];


    bool preserveExistingProgress =
      locationCompleted[
        location
      ] ||
      locationProgress[
        location
      ] >
        0 ||
      (
        expeditionActive &&
        currentLocation ==
          location
      );


    rebuilt[
      location
    ] =
      previousGateCleared ||
      preserveExistingProgress;
  }


  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    locationUnlocked[
      location
    ] =
      rebuilt[
        location
      ];
  }
}


// ==================================================
// COMPLETE CURRENT LOCATION
//
// Boss 100 calls this.
// ==================================================

void markCurrentLocationComplete() {
  if (
    !validLocation(
      currentLocation
    )
  ) {
    return;
  }


  locationProgress[
    currentLocation
  ] = 100;


  locationCompleted[
    currentLocation
  ] = true;
}


// ==================================================
// ALL LOCATIONS COMPLETE
// ==================================================

bool allLocationsCompleted() {

  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    if (
      !locationCompleted[
        location
      ]
    ) {

      return false;
    }
  }


  return true;
}


// ==================================================
// ALL BOSSES DEFEATED
// ==================================================

bool allBossesDefeated() {

  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    for (
      int bossIndex = 0;
      bossIndex < 4;
      bossIndex++
    ) {

      if (
        !bossDefeated[
          location
        ][
          bossIndex
        ]
      ) {

        return false;
      }
    }
  }


  return true;
}


// ==================================================
// BOSS GALLERY HELPERS
// ==================================================

bool bossHasBeenEncountered(
  int location,
  int bossIndex
) {

  if (
    location < 0 ||
    location >= LOCATION_COUNT ||
    bossIndex < 0 ||
    bossIndex >= 4
  ) {

    return false;
  }


  return
    bossEncountered[
      location
    ][
      bossIndex
    ];
}


// ==================================================
// MARK BOSS ENCOUNTERED
// ==================================================

void markBossEncountered(
  int location,
  int bossIndex
) {

  if (
    location < 0 ||
    location >= LOCATION_COUNT ||
    bossIndex < 0 ||
    bossIndex >= 4
  ) {

    return;
  }


  bossEncountered[
    location
  ][
    bossIndex
  ] =
    true;
}


// ==================================================
// BOSS ENCOUNTERED COUNT
// ==================================================

int bossEncounteredCount() {

  int count =
    0;


  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    for (
      int bossIndex = 0;
      bossIndex < 4;
      bossIndex++
    ) {

      if (
        bossEncountered[
          location
        ][
          bossIndex
        ]
      ) {

        count++;
      }
    }
  }


  return count;
}


// ==================================================
// BOSS DEFEATED
// ==================================================

bool bossHasBeenDefeated(
  int location,
  int bossIndex
) {

  if (
    location < 0 ||
    location >= LOCATION_COUNT ||
    bossIndex < 0 ||
    bossIndex >= 4
  ) {

    return false;
  }


  return
    bossDefeated[
      location
    ][
      bossIndex
    ];
}


// ==================================================
// MARK BOSS DEFEATED
// ==================================================

void markBossDefeated(
  int location,
  int bossIndex
) {

  if (
    location < 0 ||
    location >= LOCATION_COUNT ||
    bossIndex < 0 ||
    bossIndex >= 4
  ) {

    return;
  }


  bossEncountered[
    location
  ][
    bossIndex
  ] =
    true;


  bossDefeated[
    location
  ][
    bossIndex
  ] =
    true;
}


// ==================================================
// BOSS DEFEATED COUNT
// ==================================================

int bossDefeatedCount() {

  int count =
    0;


  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    for (
      int bossIndex = 0;
      bossIndex < 4;
      bossIndex++
    ) {

      if (
        bossDefeated[
          location
        ][
          bossIndex
        ]
      ) {

        count++;
      }
    }
  }


  return count;
}


// ==================================================
// REBUILD BOSS HISTORY FROM EXISTING PROGRESS
//
// Used when migrating a pre-gallery save.
//
// A permanently completed location guarantees all four
// bosses were defeated at least once.
//
// Otherwise current progress safely tells us which
// successful milestones have been passed in this run.
// ==================================================

void rebuildBossHistoryFromProgress() {

  const int milestones[4] = {
    25,
    50,
    75,
    100
  };


  for (
    int location = 0;
    location < LOCATION_COUNT;
    location++
  ) {

    for (
      int bossIndex = 0;
      bossIndex < 4;
      bossIndex++
    ) {

      bool knownVictory =
        locationCompleted[
          location
        ] ||
        locationProgress[
          location
        ] >=
          milestones[
            bossIndex
          ];


      bossEncountered[
        location
      ][
        bossIndex
      ] =
        knownVictory;


      bossDefeated[
        location
      ][
        bossIndex
      ] =
        knownVictory;
    }
  }
}


// ==================================================
// DIRECTION NAME
// ==================================================

String directionName(int direction) {
  if (direction == DIR_NORTH) {
    return "North";
  }

  if (direction == DIR_EAST) {
    return "East";
  }

  if (direction == DIR_SOUTH) {
    return "South";
  }

  if (direction == DIR_WEST) {
    return "West";
  }

  return "";
}


// ==================================================
// OPPOSITE DIRECTION
// ==================================================

int oppositeDirection(int direction) {
  if (direction == DIR_NORTH) {
    return DIR_SOUTH;
  }

  if (direction == DIR_SOUTH) {
    return DIR_NORTH;
  }

  if (direction == DIR_EAST) {
    return DIR_WEST;
  }

  if (direction == DIR_WEST) {
    return DIR_EAST;
  }

  return DIR_NONE;
}


// ==================================================
// DIRECTION ALLOWED
//
// The direction we just came FROM is blocked.
// ==================================================

bool directionAllowed(int direction) {
  if (
    direction < DIR_NORTH ||
    direction > DIR_WEST
  ) {
    return false;
  }


  int blockedDirection =
    oppositeDirection(
      lastDirection
    );


  return (
    direction !=
    blockedDirection
  );
}


// ==================================================
// RANDOM VALID DIRECTION
//
// Used by Auto.
// ==================================================

int randomAllowedDirection() {
  int candidates[4];

  int count = 0;


  for (
    int direction = DIR_NORTH;
    direction <= DIR_WEST;
    direction++
  ) {
    if (
      directionAllowed(
        direction
      )
    ) {
      candidates[count] =
        direction;

      count++;
    }
  }


  if (
    count <= 0
  ) {
    return DIR_NORTH;
  }


  return candidates[
    random(
      0,
      count
    )
  ];
}


// ==================================================
// TEAM HELPERS
// ==================================================

bool isOnTeam(int rosterIndex) {
  for (
    int slot = 0;
    slot < TEAM_SIZE;
    slot++
  ) {
    if (
      teamSlots[slot] ==
      rosterIndex
    ) {
      return true;
    }
  }

  return false;
}


int findSlotForRoster(int rosterIndex) {
  for (
    int slot = 0;
    slot < TEAM_SIZE;
    slot++
  ) {
    if (
      teamSlots[slot] ==
      rosterIndex
    ) {
      return slot;
    }
  }

  return -1;
}


bool validTeamSlots() {
  for (
    int a = 0;
    a < TEAM_SIZE;
    a++
  ) {
    if (
      teamSlots[a] < 0 ||
      teamSlots[a] >= ROSTER_SIZE
    ) {
      return false;
    }


    if (
      !companionIsUnlocked(
        teamSlots[a]
      )
    ) {
      return false;
    }


    for (
      int b = a + 1;
      b < TEAM_SIZE;
      b++
    ) {
      if (
        teamSlots[a] ==
        teamSlots[b]
      ) {
        return false;
      }
    }
  }

  return true;
}


// ==================================================
// EVENT XP RESET
// ==================================================

void clearEventXP() {
  for (
    int i = 0;
    i < TEAM_SIZE;
    i++
  ) {
    currentEvent.xpDelta[i] =
      0;
  }
}


// ==================================================
// ROLE VALUE HELPERS
// ==================================================

int baseRoleLevel(
  int rosterIndex,
  int role
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return 0;
  }


  if (
    role ==
    ROLE_STRENGTH
  ) {

    return
      roster[
        rosterIndex
      ].baseStrength;
  }


  if (
    role ==
    ROLE_HEART
  ) {

    return
      roster[
        rosterIndex
      ].baseHeart;
  }


  if (
    role ==
    ROLE_LUCK
  ) {

    return
      roster[
        rosterIndex
      ].baseLuck;
  }


  return 0;
}


// ==================================================
// EARNED ROLE LEVEL
// ==================================================

int earnedRoleLevel(
  int rosterIndex,
  int role
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return 0;
  }


  if (
    role ==
    ROLE_STRENGTH
  ) {

    return
      roster[
        rosterIndex
      ].earnedStrength;
  }


  if (
    role ==
    ROLE_HEART
  ) {

    return
      roster[
        rosterIndex
      ].earnedHeart;
  }


  if (
    role ==
    ROLE_LUCK
  ) {

    return
      roster[
        rosterIndex
      ].earnedLuck;
  }


  return 0;
}


// ==================================================
// EFFECTIVE ROLE LEVEL
//
// Card aptitude + training.
// ==================================================

int effectiveRoleLevel(
  int rosterIndex,
  int role
) {

  return
    baseRoleLevel(
      rosterIndex,
      role
    ) +
    earnedRoleLevel(
      rosterIndex,
      role
    );
}


// ==================================================
// CURRENT ROLE XP
// ==================================================

int currentRoleXP(
  int rosterIndex,
  int role
) {

  if (
    rosterIndex < 0 ||
    rosterIndex >= ROSTER_SIZE
  ) {

    return 0;
  }


  if (
    role ==
    ROLE_STRENGTH
  ) {

    return
      roster[
        rosterIndex
      ].strengthXP;
  }


  if (
    role ==
    ROLE_HEART
  ) {

    return
      roster[
        rosterIndex
      ].heartXP;
  }


  if (
    role ==
    ROLE_LUCK
  ) {

    return
      roster[
        rosterIndex
      ].luckXP;
  }


  return 0;
}


// ==================================================
// NEXT ROLE LEVEL THRESHOLD
//
// Earned 0 -> 1 = 50 XP
// Earned 1 -> 2 = 100 XP
// Earned 2 -> 3 = 150 XP
// etc.
// ==================================================

int roleXPThreshold(
  int rosterIndex,
  int role
) {

  return
    ROLE_XP_BASE *
    (
      earnedRoleLevel(
        rosterIndex,
        role
      ) +
      1
    );
}


// ==================================================
// OVERALL LEVEL
//
// Purely derived from training.
// Innate card aptitude does not contribute.
//
// Everyone therefore starts at Overall Level 0.
// ==================================================

int overallLevel(
  int rosterIndex
) {

  return
    earnedRoleLevel(
      rosterIndex,
      ROLE_STRENGTH
    ) +
    earnedRoleLevel(
      rosterIndex,
      ROLE_HEART
    ) +
    earnedRoleLevel(
      rosterIndex,
      ROLE_LUCK
    );
}


// ==================================================
// LIFETIME XP IN ONE ROLE
//
// Completed earned levels use thresholds:
//
// 50 + 100 + 150 + ...
//
// = 25 * n * (n + 1)
// ==================================================

static int lifetimeRoleXP(
  int rosterIndex,
  int role
) {

  int earned =
    earnedRoleLevel(
      rosterIndex,
      role
    );


  int completed =
    25 *
    earned *
    (
      earned +
      1
    );


  return
    completed +
    currentRoleXP(
      rosterIndex,
      role
    );
}


// ==================================================
// OVERALL LIFETIME XP
//
// Used only as a compact experience summary in
// Reserve / character-card views.
// ==================================================

int overallLifetimeXP(
  int rosterIndex
) {

  return
    lifetimeRoleXP(
      rosterIndex,
      ROLE_STRENGTH
    ) +
    lifetimeRoleXP(
      rosterIndex,
      ROLE_HEART
    ) +
    lifetimeRoleXP(
      rosterIndex,
      ROLE_LUCK
    );
}


// ==================================================
// ADD ROLE XP
// ==================================================

void addRoleXP(
  int rosterIndex,
  int role,
  int amount
) {

  if (
    !companionIsUnlocked(
      rosterIndex
    ) ||
    amount <= 0
  ) {

    return;
  }


  int *earned =
    nullptr;


  int *xp =
    nullptr;


  if (
    role ==
    ROLE_STRENGTH
  ) {

    earned =
      &roster[
        rosterIndex
      ].earnedStrength;


    xp =
      &roster[
        rosterIndex
      ].strengthXP;
  }


  else if (
    role ==
    ROLE_HEART
  ) {

    earned =
      &roster[
        rosterIndex
      ].earnedHeart;


    xp =
      &roster[
        rosterIndex
      ].heartXP;
  }


  else if (
    role ==
    ROLE_LUCK
  ) {

    earned =
      &roster[
        rosterIndex
      ].earnedLuck;


    xp =
      &roster[
        rosterIndex
      ].luckXP;
  }


  if (
    earned ==
      nullptr ||
    xp ==
      nullptr
  ) {

    return;
  }


  *xp +=
    amount;


  while (
    *xp >=
    ROLE_XP_BASE *
    (
      *earned +
      1
    )
  ) {

    *xp -=
      ROLE_XP_BASE *
      (
        *earned +
        1
      );


    (*earned)++;
  }
}


// ==================================================
// LOSE ROLE XP
//
// Earned levels can never be lost.
// Only XP inside the current training level falls.
// ==================================================

void loseRoleXP(
  int rosterIndex,
  int role,
  int amount
) {

  if (
    !companionIsUnlocked(
      rosterIndex
    ) ||
    amount <= 0
  ) {

    return;
  }


  int *xp =
    nullptr;


  if (
    role ==
    ROLE_STRENGTH
  ) {

    xp =
      &roster[
        rosterIndex
      ].strengthXP;
  }


  else if (
    role ==
    ROLE_HEART
  ) {

    xp =
      &roster[
        rosterIndex
      ].heartXP;
  }


  else if (
    role ==
    ROLE_LUCK
  ) {

    xp =
      &roster[
        rosterIndex
      ].luckXP;
  }


  if (
    xp ==
      nullptr
  ) {

    return;
  }


  *xp -=
    amount;


  if (
    *xp <
    0
  ) {

    *xp =
      0;
  }
}


// ==================================================
// ROLE XP
//
// The event reward for a slot trains only the
// companion currently occupying that role.
// ==================================================

void awardRoleXP(
  int heartXP,
  int strengthXP,
  int luckXP
) {

  int rewards[
    TEAM_SIZE
  ] = {
    heartXP,
    strengthXP,
    luckXP
  };


  for (
    int role = 0;
    role < TEAM_SIZE;
    role++
  ) {

    int member =
      teamSlots[
        role
      ];


    if (
      roster[
        member
      ].restSteps >
      0
    ) {

      currentEvent.xpDelta[
        role
      ] =
        0;


      continue;
    }


    currentEvent.xpDelta[
      role
    ] =
      rewards[
        role
      ];


    addRoleXP(
      member,
      role,
      rewards[
        role
      ]
    );
  }
}


// ==================================================
// TEAM XP LOSS
//
// Retreat loss is applied to the role each companion
// is currently occupying.
// ==================================================

void removeXPFromReadyTeam(
  int amount
) {

  for (
    int role = 0;
    role < TEAM_SIZE;
    role++
  ) {

    int member =
      teamSlots[
        role
      ];


    if (
      roster[
        member
      ].restSteps >
      0
    ) {

      currentEvent.xpDelta[
        role
      ] =
        0;


      continue;
    }


    loseRoleXP(
      member,
      role,
      amount
    );


    currentEvent.xpDelta[
      role
    ] =
      -amount;
  }
}


// ==================================================
// LEGACY COMPATIBILITY HELPERS
//
// No boss or UI gameplay relies on these now.
// ==================================================

void addXP(
  int rosterIndex,
  int amount
) {

  int role =
    findSlotForRoster(
      rosterIndex
    );


  if (
    role >=
    0
  ) {

    addRoleXP(
      rosterIndex,
      role,
      amount
    );
  }
}


void loseXP(
  int rosterIndex,
  int amount
) {

  int role =
    findSlotForRoster(
      rosterIndex
    );


  if (
    role >=
    0
  ) {

    loseRoleXP(
      rosterIndex,
      role,
      amount
    );
  }
}


int companionPower(
  int rosterIndex
) {

  return
    overallLifetimeXP(
      rosterIndex
    );
}


int teamPower() {

  int total =
    0;


  for (
    int role = 0;
    role < TEAM_SIZE;
    role++
  ) {

    int member =
      teamSlots[
        role
      ];


    if (
      roster[
        member
      ].restSteps ==
      0
    ) {

      total +=
        overallLifetimeXP(
          member
        );
    }
  }


  return total;
}


// ==================================================
// READY ACTIVE
// ==================================================

int countReadyActive() {
  int count = 0;


  for (
    int slot = 0;
    slot < TEAM_SIZE;
    slot++
  ) {
    int member =
      teamSlots[slot];


    if (
      roster[member].restSteps ==
      0
    ) {
      count++;
    }
  }


  return count;
}


// ==================================================
// READY ROSTER
//
// Locked companions do not count.
// ==================================================

int countReadyRoster() {
  int count = 0;


  for (
    int position = 0;
    position <
      unlockedCompanionCount;
    position++
  ) {

    int member =
      companionUnlockOrder[
        position
      ];


    if (
      roster[member].restSteps ==
      0
    ) {
      count++;
    }
  }


  return count;
}


// ==================================================
// RANDOM READY SLOT
// ==================================================

int randomReadySlot() {
  int candidates[TEAM_SIZE];

  int count = 0;


  for (
    int slot = 0;
    slot < TEAM_SIZE;
    slot++
  ) {
    int member =
      teamSlots[slot];


    if (
      roster[member].restSteps ==
      0
    ) {
      candidates[count] =
        slot;

      count++;
    }
  }


  if (
    count == 0
  ) {
    return -1;
  }


  return candidates[
    random(
      0,
      count
    )
  ];
}


// ==================================================
// FATIGUE RESET
// ==================================================

void clearExpeditionFatigue() {
  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {
    roster[i].restSteps = 0;

    roster[i].knockoutCount = 0;
  }
}
