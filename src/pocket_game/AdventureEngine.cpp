#include <Arduino.h>

#include "GameState.h"
#include "AdventureEngine.h"
#include "EventContent.h"
#include "SaveSystem.h"


// ==================================================
// EVENT WEIGHTS
//
// All rows total 100.
// ==================================================

struct EventWeights {

  int travel;

  int battle;

  int friendly;

  int treasure;

  int npc;

  int discovery;

  int rest;

  int rare;
};


const EventWeights
LOCATION_WEIGHTS[
  LOCATION_COUNT
] = {

  // WHISPERWOOD

  {
    30,  // Travel
    6,   // Battle
    18,  // Friendly
    12,  // Treasure
    11,  // Potion Maker / NPC
    11,  // Discovery
    7,   // Rest
    5    // Weird / Rare
  },


  // OPEN PLAINS

  {
    30,  // Travel
    12,  // Battle
    11,  // Friendly
    13,  // Treasure
    9,   // Potion Maker / NPC
    13,  // Discovery
    7,   // Rest
    5    // Weird / Rare
  },


  // DUSTY DESERT

  {
    30,  // Travel
    18,  // Battle
    6,   // Friendly
    13,  // Treasure
    6,   // Potion Maker / NPC
    14,  // Discovery
    7,   // Rest
    6    // Weird / Rare
  },


  // STORM COAST

  {
    30,  // Travel
    22,  // Battle
    5,   // Friendly
    13,  // Treasure
    5,   // Potion Maker / NPC
    9,   // Discovery
    7,   // Rest
    9    // Weird / Rare
  },


  // MISTY MARSH

  {
    30,  // Travel
    20,  // Battle
    7,   // Friendly
    10,  // Treasure
    7,   // NPC
    12,  // Discovery
    7,   // Rest
    7    // Weird / Rare
  },


  // FROSTPEAK

  {
    30,  // Travel
    22,  // Battle
    5,   // Friendly
    9,   // Treasure
    5,   // NPC
    12,  // Discovery
    9,   // Rest
    8    // Weird / Rare
  },


  // VOLCANIC HIGHLANDS

  {
    30,  // Travel
    26,  // Battle
    4,   // Friendly
    9,   // Treasure
    4,   // NPC
    10,  // Discovery
    7,   // Rest
    10   // Weird / Rare
  },


  // SKYREACH

  {
    30,  // Travel
    22,  // Battle
    4,   // Friendly
    13,  // Treasure
    4,   // NPC
    10,  // Discovery
    6,   // Rest
    11   // Weird / Rare
  }
};


// ==================================================
// BOSS GAMEPLAY DATA
//
// Bosses now check the EFFECTIVE value of each active
// role separately:
//
// Effective role value =
// card aptitude + earned training levels.
//
// These are first-pass balance values for TFT testing.
// ==================================================

struct BossDefinition {

  int strengthRequired;
  int heartRequired;
  int luckRequired;

  int heartXP;
  int strengthXP;
  int luckXP;
};


const BossDefinition
BOSSES[
  LOCATION_COUNT
][4] = {

  // =================================================
  // WHISPERING WOODS
  // progression position 1
  // =================================================

  {
    {1, 2, 1, 10, 10, 10},
    {2, 2, 1, 10, 10, 10},
    {3, 2, 2, 10, 10, 10},
    {2, 2, 3, 10, 10, 10}
  },


  // =================================================
  // ANCIENT RUINS
  // progression position 2
  // =================================================

  {
    {2, 3, 2, 10, 10, 10},
    {3, 3, 2, 10, 10, 10},
    {4, 3, 3, 10, 10, 10},
    {3, 3, 4, 10, 10, 10}
  },


  // =================================================
  // EMBERFIELD PLAINS
  // internal id: LOC_DUSTY_DESERT
  // progression position 6
  // =================================================

  {
    {6, 7, 6, 10, 10, 10},
    {7, 7, 6, 10, 10, 10},
    {8, 7, 7, 10, 10, 10},
    {7, 7, 8, 10, 10, 10}
  },


  // =================================================
  // STORM COAST
  // progression position 4
  // =================================================

  {
    {4, 5, 4, 10, 10, 10},
    {5, 5, 4, 10, 10, 10},
    {6, 5, 5, 10, 10, 10},
    {5, 5, 6, 10, 10, 10}
  },


  // =================================================
  // MISTY MARSHES
  // progression position 3
  // =================================================

  {
    {3, 4, 3, 10, 10, 10},
    {4, 4, 3, 10, 10, 10},
    {5, 4, 4, 10, 10, 10},
    {4, 4, 5, 10, 10, 10}
  },


  // =================================================
  // FROSTPEAK MOUNTAINS
  // progression position 5
  // =================================================

  {
    {5, 6, 5, 10, 10, 10},
    {6, 6, 5, 10, 10, 10},
    {7, 6, 6, 10, 10, 10},
    {6, 6, 7, 10, 10, 10}
  },


  // =================================================
  // VOLCANIC BADLANDS
  // progression position 7
  // =================================================

  {
    {7, 8, 7, 10, 10, 10},
    {8, 8, 7, 10, 10, 10},
    {9, 8, 8, 10, 10, 10},
    {8, 8, 9, 10, 10, 10}
  },


  // =================================================
  // SKYREACH CITY
  // progression position 8
  // =================================================

  {
    {8, 9, 8, 10, 10, 10},
    {9, 9, 8, 10, 10, 10},
    {10, 9, 9, 10, 10, 10},
    {9, 9, 10, 10, 10}
  }
};


// ==================================================
// CONTENT HELPERS
// ==================================================

static void useEventText(
  const EventText &text
) {

  currentEvent.title =
    text.title;


  currentEvent.line1 =
    text.line1;


  currentEvent.line2 =
    text.line2;
}


// ==================================================
// GET CHARACTER IN A ROLE
// ==================================================

static String roleCharacterName(
  int role
) {

  if (
    role < 0 ||
    role >= TEAM_SIZE
  ) {

    return "Someone";
  }


  int member =
    teamSlots[
      role
    ];


  if (
    member < 0 ||
    member >= ROSTER_SIZE
  ) {

    return "Someone";
  }


  return roster[
    member
  ].name;
}


// ==================================================
// RANDOM READY CHARACTER FOR FLAVOUR TEXT
//
// No gameplay effect.
// ==================================================

static String randomReadyCharacterName() {

  int slot =
    randomReadySlot();


  if (
    slot < 0
  ) {

    return "Someone";
  }


  int member =
    teamSlots[
      slot
    ];


  return roster[
    member
  ].name;
}


// ==================================================
// READING -> STEPS
// ==================================================

void addPagesRead(
  int pages
) {

  totalPagesRead +=
    pages;


  pagesTowardStep +=
    pages;


  while (
    pagesTowardStep >=
    PAGES_PER_STEP
  ) {

    pagesTowardStep -=
      PAGES_PER_STEP;


    bankedSteps++;
  }


  saveGame();
}


// ==================================================
// START EXPEDITION
// ==================================================

void startExpedition() {

  if (
    bankedSteps <= 0
  ) {

    return;
  }


  if (
    !validLocation(
      currentLocation
    )
  ) {

    return;
  }


  if (
    !locationUnlocked[
      currentLocation
    ]
  ) {

    return;
  }


  // =================================================
  // RESUME ACTIVE ADVENTURE
  //
  // Menus, reading, sleep and reboot do not end the
  // current run. Re-entering the same active location
  // resumes it without clearing temporary fatigue.
  // =================================================

  if (
    expeditionActive &&
    locationProgress[
      currentLocation
    ] <
      100
  ) {

    expeditionStep =
      locationProgress[
        currentLocation
      ];


    saveGame();


    return;
  }


  // =================================================
  // REPLAY
  //
  // A completed location begins a fresh run at 0/100.
  // Permanent completion/history remains untouched.
  // =================================================

  if (
    locationCompleted[
      currentLocation
    ] &&
    locationProgress[
      currentLocation
    ] >= 100
  ) {

    locationProgress[
      currentLocation
    ] =
      0;
  }


  clearExpeditionFatigue();


  expeditionStep =
    locationProgress[
      currentLocation
    ];


  expeditionActive =
    true;


  wipeoutPending =
    false;


  lastDirection =
    DIR_NONE;


  lastTravelText =
    "";


  saveGame();
}


// ==================================================
// FINISH EXPEDITION
// ==================================================

void finishExpedition() {

  expeditionActive =
    false;


  expeditionStep =
    locationProgress[
      currentLocation
    ];


  wipeoutPending =
    false;


  autoProgress =
    false;


  clearExpeditionFatigue();


  lastDirection =
    DIR_NONE;


  lastTravelText =
    "";


  saveGame();
}


// ==================================================
// ABANDON CURRENT ADVENTURE
//
// Explicitly switching away from an active location
// ends that run and returns its run progress to 0.
//
// Permanent systems are intentionally untouched:
// - role XP / levels
// - companions
// - location unlock/completion history
// - boss encountered/defeated history
// ==================================================

void abandonCurrentAdventure() {

  if (
    validLocation(
      currentLocation
    )
  ) {

    locationProgress[
      currentLocation
    ] =
      0;
  }


  expeditionStep =
    0;


  expeditionActive =
    false;


  wipeoutPending =
    false;


  autoProgress =
    false;


  clearExpeditionFatigue();


  lastDirection =
    DIR_NONE;


  lastTravelText =
    "";


  saveGame();
}


// ==================================================
// BOSS MILESTONES
// ==================================================

bool isBossMilestone(
  int stepNumber
) {

  return (
    stepNumber == 25 ||
    stepNumber == 50 ||
    stepNumber == 75 ||
    stepNumber == 100
  );
}


// ==================================================
// BOSS INDEX
// ==================================================

static int bossIndexForStep(
  int stepNumber
) {

  if (
    stepNumber == 25
  ) {

    return 0;
  }


  if (
    stepNumber == 50
  ) {

    return 1;
  }


  if (
    stepNumber == 75
  ) {

    return 2;
  }


  if (
    stepNumber == 100
  ) {

    return 3;
  }


  return -1;
}


// ==================================================
// PREVIOUS CHECKPOINT
// ==================================================

static int previousCheckpoint(
  int stepNumber
) {

  if (
    stepNumber == 25
  ) {

    return 0;
  }


  if (
    stepNumber == 50
  ) {

    return 25;
  }


  if (
    stepNumber == 75
  ) {

    return 50;
  }


  if (
    stepNumber == 100
  ) {

    return 75;
  }


  return 0;
}


// ==================================================
// BOSS
// ==================================================

bool generateBoss(
  int stepNumber
) {

  clearEventXP();


  currentEvent.type =
    BOSS_EVENT;


  currentEvent.pauseAuto =
    false;


  int bossIndex =
    bossIndexForStep(
      stepNumber
    );


  if (
    bossIndex < 0
  ) {

    return true;
  }


  BossDefinition boss =
    BOSSES[
      currentLocation
    ][
      bossIndex
    ];


  BossText text =
    contentBoss(
      currentLocation,
      bossIndex
    );


  // First sighting permanently reveals this boss in
  // the Boss Gallery, even if the player loses.

  markBossEncountered(
    currentLocation,
    bossIndex
  );


  int strengthMember =
    teamSlots[
      ROLE_STRENGTH
    ];


  int heartMember =
    teamSlots[
      ROLE_HEART
    ];


  int luckMember =
    teamSlots[
      ROLE_LUCK
    ];


  int strengthValue =
    roster[
      strengthMember
    ].restSteps >
      0
      ? 0
      : effectiveRoleLevel(
          strengthMember,
          ROLE_STRENGTH
        );


  int heartValue =
    roster[
      heartMember
    ].restSteps >
      0
      ? 0
      : effectiveRoleLevel(
          heartMember,
          ROLE_HEART
        );


  int luckValue =
    roster[
      luckMember
    ].restSteps >
      0
      ? 0
      : effectiveRoleLevel(
          luckMember,
          ROLE_LUCK
        );


  bool strengthPassed =
    strengthValue >=
    boss.strengthRequired;


  bool heartPassed =
    heartValue >=
    boss.heartRequired;


  bool luckPassed =
    luckValue >=
    boss.luckRequired;


  currentEvent.title =
    text.name;


  currentEvent.line1 =
    text.encounter;


  currentEvent.line2 =
    "S" +
    String(
      strengthValue
    ) +
    "/" +
    String(
      boss.strengthRequired
    ) +
    " H" +
    String(
      heartValue
    ) +
    "/" +
    String(
      boss.heartRequired
    ) +
    " L" +
    String(
      luckValue
    ) +
    "/" +
    String(
      boss.luckRequired
    );


  // =================================================
  // VICTORY
  // =================================================

  if (
    strengthPassed &&
    heartPassed &&
    luckPassed
  ) {

    awardRoleXP(
      boss.heartXP,
      boss.strengthXP,
      boss.luckXP
    );


    bool firstVictory =
      !bossHasBeenDefeated(
        currentLocation,
        bossIndex
      );


    // Permanent Boss Gallery record.

    markBossDefeated(
      currentLocation,
      bossIndex
    );


    // =================================================
    // FIRST-DEFEAT COMPANION REWARDS
    //
    // 25/50/75% bosses:
    //   next normal companion, until all 23 normals
    //   (including the three starters) are owned.
    //
    // 100% boss:
    //   fixed special for this location's progression position.
    //
    // Replay victories never duplicate rewards.
    // =================================================

    if (
      firstVictory
    ) {

      int unlockedMember =
        -1;


      if (
        bossIndex ==
          3
      ) {

        unlockedMember =
          unlockCompanionByRosterIndex(
            specialCompanionForLocation(
              currentLocation
            )
          );
      }


      else {

        unlockedMember =
          unlockNextNormalCompanion();
      }


      queueCompanionReveal(
        unlockedMember
      );
    }


    if (
      stepNumber ==
      50
    ) {

      currentEvent.result =
        "VICTORY - NEW AREA UNLOCKED!";
    }


    else if (
      stepNumber ==
      100
    ) {

      currentEvent.result =
        "VICTORY - LOCATION COMPLETE!";
    }


    else {

      currentEvent.result =
        "VICTORY!";
    }


    return true;
  }


  // =================================================
  // DEFEAT
  //
  // No XP/stat/level loss.
  // =================================================

  int checkpoint =
    previousCheckpoint(
      stepNumber
    );


  currentEvent.result =
    "DEFEAT - BACK TO " +
    String(
      checkpoint
    );


  return false;
}


// ==================================================
// BATTLE
// ==================================================

void generateBattle() {

  clearEventXP();


  currentEvent.type =
    BATTLE;


  currentEvent.pauseAuto =
    false;


  EventText battleText =
    contentBattle(
      currentLocation
    );


  useEventText(
    battleText
  );


  int partySize =
    countReadyActive();


  int winChance;


  if (
    partySize == 3
  ) {

    winChance =
      65;
  }


  else if (
    partySize == 2
  ) {

    winChance =
      57;
  }


  else {

    winChance =
      48;
  }


  int roll =
    random(
      1,
      101
    );


  // =================================================
  // VICTORY
  // =================================================

  if (
    roll <=
    winChance
  ) {

    currentEvent.result =
      contentBattleVictory(
        roleCharacterName(
          ROLE_STRENGTH
        )
      );


    awardRoleXP(
      2,
      3,
      2
    );


    return;
  }


  // =================================================
  // FORCED RETREAT
  // =================================================

  if (
    roll <=
    winChance + 25
  ) {

    currentEvent.title =
      "FORCED TO RETREAT";


    currentEvent.result =
      contentBattleRetreat();


    removeXPFromReadyTeam(
      1
    );


    currentEvent.pauseAuto =
      false;


    return;
  }


  // =================================================
  // KNOCKOUT
  // =================================================

  int victimSlot =
    randomReadySlot();


  if (
    victimSlot < 0
  ) {

    currentEvent.title =
      "RETREAT";


    currentEvent.line1 =
      "Nobody was ready";


    currentEvent.line2 =
      "for that.";


    currentEvent.result =
      "Time to regroup.";


    currentEvent.pauseAuto =
      true;


    return;
  }


  int victim =
    teamSlots[
      victimSlot
    ];


  roster[
    victim
  ].knockoutCount++;


  int recovery =
    roster[
      victim
    ].knockoutCount;


  if (
    recovery > 3
  ) {

    recovery =
      3;
  }


  roster[
    victim
  ].restSteps =
    recovery;


  loseRoleXP(
    victim,
    victimSlot,
    2
  );


  currentEvent.xpDelta[
    victimSlot
  ] =
    -2;


  currentEvent.title =
    "KNOCKED OUT!";


  currentEvent.line1 =
    roster[
      victim
    ].name +
    " took a heavy hit.";


  currentEvent.line2 =
    "They'll be fine.";


  currentEvent.result =
    roleName(
      victimSlot
    ) +
    " rests " +
    String(
      recovery
    ) +
    " step";


  if (
    recovery != 1
  ) {

    currentEvent.result +=
      "s";
  }


  currentEvent.pauseAuto =
    false;
}


// ==================================================
// TRAVEL / SMALL MOMENT
// ==================================================

static void generateTravel() {

  clearEventXP();


  currentEvent.type =
    TRAVEL_EVENT;


  currentEvent.pauseAuto =
    false;


  EventText text =
    contentTravel(
      currentLocation,
      randomReadyCharacterName()
    );


  useEventText(
    text
  );


  awardRoleXP(
    1,
    1,
    1
  );


  currentEvent.result =
    "";
}


// ==================================================
// FRIENDLY
// ==================================================

static void generateFriendly() {

  clearEventXP();


  currentEvent.type =
    FRIENDLY_ENCOUNTER;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentFriendly(
      currentLocation
    )
  );


  currentEvent.result =
    contentFriendlyResult(
      roleCharacterName(
        ROLE_HEART
      )
    );


  awardRoleXP(
    3,
    2,
    2
  );
}


// ==================================================
// TREASURE
// ==================================================

static void generateTreasure() {

  clearEventXP();


  currentEvent.type =
    TREASURE;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentTreasure(
      currentLocation
    )
  );


  currentEvent.result =
    contentTreasureResult(
      roleCharacterName(
        ROLE_LUCK
      )
    );


  awardRoleXP(
    2,
    2,
    3
  );
}


// ==================================================
// NPC
// ==================================================

static void generateNPC() {

  clearEventXP();


  currentEvent.type =
    POTION_MAKER;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentNPC(
      currentLocation
    )
  );


  currentEvent.result =
    contentNPCResult(
      roleCharacterName(
        ROLE_HEART
      )
    );


  awardRoleXP(
    3,
    2,
    1
  );
}


// ==================================================
// DISCOVERY
// ==================================================

static void generateDiscovery() {

  clearEventXP();


  currentEvent.type =
    DISCOVERY;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentDiscovery(
      currentLocation
    )
  );


  currentEvent.result =
    contentDiscoveryResult(
      roleCharacterName(
        ROLE_LUCK
      )
    );


  awardRoleXP(
    2,
    2,
    3
  );
}


// ==================================================
// REST
// ==================================================

static void generateRest() {

  clearEventXP();


  currentEvent.type =
    REST_EVENT;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentRest(
      currentLocation
    )
  );


  currentEvent.result =
    contentRestResult(
      roleCharacterName(
        ROLE_HEART
      )
    );


  awardRoleXP(
    1,
    1,
    1
  );
}


// ==================================================
// RARE
// ==================================================

static void generateRare() {

  clearEventXP();


  currentEvent.type =
    RARE_ENCOUNTER;


  currentEvent.pauseAuto =
    false;


  useEventText(
    contentRare(
      currentLocation
    )
  );


  currentEvent.result =
    contentRareResult(
      roleCharacterName(
        ROLE_LUCK
      )
    );


  awardRoleXP(
    2,
    3,
    3
  );
}


// ==================================================
// NORMAL EVENT GENERATOR
// ==================================================

void generateEvent() {

  clearEventXP();


  currentEvent.pauseAuto =
    false;


  EventWeights weights =
    LOCATION_WEIGHTS[
      currentLocation
    ];


  int roll =
    random(
      1,
      101
    );


  int threshold =
    weights.travel;


  // TRAVEL

  if (
    roll <= threshold
  ) {

    generateTravel();


    return;
  }


  // BATTLE

  threshold +=
    weights.battle;


  if (
    roll <= threshold
  ) {

    generateBattle();


    return;
  }


  // FRIENDLY

  threshold +=
    weights.friendly;


  if (
    roll <= threshold
  ) {

    generateFriendly();


    return;
  }


  // TREASURE

  threshold +=
    weights.treasure;


  if (
    roll <= threshold
  ) {

    generateTreasure();


    return;
  }


  // NPC

  threshold +=
    weights.npc;


  if (
    roll <= threshold
  ) {

    generateNPC();


    return;
  }


  // DISCOVERY

  threshold +=
    weights.discovery;


  if (
    roll <= threshold
  ) {

    generateDiscovery();


    return;
  }


  // REST

  threshold +=
    weights.rest;


  if (
    roll <= threshold
  ) {

    generateRest();


    return;
  }


  // RARE

  generateRare();
}


// ==================================================
// AUTO / ALL-ACTIVE RECOVERY STEP
//
// Auto never stops merely because the active party
// is resting. If nobody in the active party is ready,
// the next non-boss step becomes a quiet recovery
// moment. The step is still spent and normal recovery
// ticks happen afterwards.
// ==================================================

static void generateRecoveryStep() {

  clearEventXP();


  currentEvent.type =
    REST_EVENT;


  currentEvent.pauseAuto =
    false;


  currentEvent.title =
    "CATCHING BREATH";


  currentEvent.line1 =
    "The team slows the pace.";


  currentEvent.line2 =
    "Everyone catches their breath.";


  currentEvent.result =
    "Recovery continues.";
}


// ==================================================
// PERFORM ADVENTURE STEP
// ==================================================

void performAdventureStep(
  int direction
) {

  // =================================================
  // SAFETY
  // =================================================

  if (
    bankedSteps <= 0
  ) {

    return;
  }


  if (
    locationProgress[
      currentLocation
    ] >= 100
  ) {

    return;
  }


  if (
    !directionAllowed(
      direction
    )
  ) {

    return;
  }


  // =================================================
  // DIRECTION
  // =================================================

  lastDirection =
    direction;


  lastTravelText =
    "Travelled " +
    directionName(
      direction
    );


  // =================================================
  // WHO WAS ALREADY RESTING?
  // =================================================

  bool restingBefore[
    ROSTER_SIZE
  ];


  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    restingBefore[i] =
      roster[i].restSteps > 0;
  }


  // =================================================
  // SPEND STEP
  // =================================================

  bankedSteps--;


  // =================================================
  // ADVANCE LOCATION
  // =================================================

  locationProgress[
    currentLocation
  ]++;


  expeditionStep =
    locationProgress[
      currentLocation
    ];


  // =================================================
  // BOSS OR NORMAL EVENT
  // =================================================

  bool bossEncounter =
    isBossMilestone(
      expeditionStep
    );


  bool bossWon =
    true;


  int bossStep =
    expeditionStep;


  if (
    bossEncounter
  ) {

    bossWon =
      generateBoss(
        bossStep
      );
  }


  else if (
    countReadyActive() ==
    0
  ) {

    generateRecoveryStep();
  }


  else {

    generateEvent();
  }


  // =================================================
  // BOSS PROGRESSION
  // =================================================

  if (
    bossEncounter
  ) {

    // =================================================
    // VICTORY
    // =================================================

    if (
      bossWon
    ) {

      if (
        bossStep == 50
      ) {

        unlockNextLocation();
      }


      if (
        bossStep == 100
      ) {

        markCurrentLocationComplete();


        // Goochi is the true meta-completion reward:
        // every location complete AND all 32 bosses
        // defeated. On the final clear this queues
        // after the location's own special reveal.

        if (
          allLocationsCompleted() &&
          allBossesDefeated()
        ) {

          int goochi =
            unlockCompanionByRosterIndex(
              GOOCHI_ROSTER_INDEX
            );


          queueCompanionReveal(
            goochi
          );
        }
      }
    }


    // =================================================
    // DEFEAT
    // =================================================

    else {

      int checkpoint =
        previousCheckpoint(
          bossStep
        );


      locationProgress[
        currentLocation
      ] =
        checkpoint;


      expeditionStep =
        checkpoint;
    }
  }


  // =================================================
  // RECOVERY TICKS
  //
  // Anyone KO'd during this event does NOT
  // immediately recover a step.
  // =================================================

  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    if (
      restingBefore[i] &&
      roster[i].restSteps > 0
    ) {

      roster[i].restSteps--;
    }
  }


  // =================================================
  // WIPEOUT
  // =================================================

  wipeoutPending =
    countReadyRoster() == 0;


  // =================================================
  // AUTO STOP CONDITIONS
  //
  // Auto stops only when:
  // 1. No banked steps remain.
  // 2. The player taps to stop it (handled in UI.cpp).
  // 3. The current location reaches 100%.
  //
  // Bosses, defeat, KO, rest and wipeout do not stop
  // Auto.
  // =================================================

  if (
    autoProgress &&
    (
      bankedSteps <= 0 ||
      locationProgress[
        currentLocation
      ] >= 100
    )
  ) {

    autoProgress =
      false;
  }


  saveGame();

}