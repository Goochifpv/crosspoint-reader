#include <Arduino.h>
#include <Preferences.h>

#include "GameState.h"
#include "SaveSystem.h"

static Preferences prefs;


// ==================================================
// V14 COMPANION PROGRESSION BLOB
//
// Card aptitude is static code data and is NOT saved.
// Only earned role training + temporary fatigue state
// are persisted.
// ==================================================

struct CompanionProgressSave {

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
// SAVE COMPANION PROGRESSION
// ==================================================

static void saveCompanionProgress() {

  CompanionProgressSave data[
    ROSTER_SIZE
  ];


  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    data[i].earnedStrength =
      roster[i].earnedStrength;


    data[i].strengthXP =
      roster[i].strengthXP;


    data[i].earnedHeart =
      roster[i].earnedHeart;


    data[i].heartXP =
      roster[i].heartXP;


    data[i].earnedLuck =
      roster[i].earnedLuck;


    data[i].luckXP =
      roster[i].luckXP;


    data[i].restSteps =
      roster[i].restSteps;


    data[i].knockoutCount =
      roster[i].knockoutCount;
  }


  prefs.putBytes(
    "prog14",
    data,
    sizeof(
      data
    )
  );
}


// ==================================================
// LOAD COMPANION PROGRESSION
// ==================================================

static bool loadCompanionProgress() {

  if (
    prefs.getBytesLength(
      "prog14"
    ) !=
    sizeof(
      CompanionProgressSave
    ) *
    ROSTER_SIZE
  ) {

    return false;
  }


  CompanionProgressSave data[
    ROSTER_SIZE
  ];


  prefs.getBytes(
    "prog14",
    data,
    sizeof(
      data
    )
  );


  for (
    int i = 0;
    i < ROSTER_SIZE;
    i++
  ) {

    roster[i].earnedStrength =
      data[i].earnedStrength <
        0
        ? 0
        : data[i].earnedStrength;


    roster[i].strengthXP =
      data[i].strengthXP <
        0
        ? 0
        : data[i].strengthXP;


    roster[i].earnedHeart =
      data[i].earnedHeart <
        0
        ? 0
        : data[i].earnedHeart;


    roster[i].heartXP =
      data[i].heartXP <
        0
        ? 0
        : data[i].heartXP;


    roster[i].earnedLuck =
      data[i].earnedLuck <
        0
        ? 0
        : data[i].earnedLuck;


    roster[i].luckXP =
      data[i].luckXP <
        0
        ? 0
        : data[i].luckXP;


    roster[i].restSteps =
      data[i].restSteps <
        0
        ? 0
        : data[i].restSteps;


    roster[i].knockoutCount =
      data[i].knockoutCount <
        0
        ? 0
        : data[i].knockoutCount;
  }


  return true;
}


// ==================================================
// SAVE
// ==================================================

void saveGame() {

  prefs.begin(
    "companions",
    false
  );


  prefs.putInt(
    "version",
    SAVE_VERSION
  );


  // =================================================
  // COMPANIONS
  // =================================================

  saveCompanionProgress();


  // =================================================
  // COLLECTION ORDER / OWNERSHIP
  // =================================================

  prefs.putBytes(
    "uorder",
    companionUnlockOrder,
    sizeof(
      companionUnlockOrder
    )
  );


  prefs.putInt(
    "ucount",
    unlockedCompanionCount
  );


  // =================================================
  // RESERVE ORDER
  // =================================================

  prefs.putBytes(
    "rorder",
    reserveOrder,
    sizeof(
      reserveOrder
    )
  );


  prefs.putInt(
    "rcount",
    reserveCount
  );


  // =================================================
  // PARTY
  // =================================================

  prefs.putInt(
    "ts0",
    teamSlots[0]
  );


  prefs.putInt(
    "ts1",
    teamSlots[1]
  );


  prefs.putInt(
    "ts2",
    teamSlots[2]
  );


  // =================================================
  // LOCATIONS
  // =================================================

  prefs.putInt(
    "loc",
    currentLocation
  );


  for (
    int i = 0;
    i < LOCATION_COUNT;
    i++
  ) {

    String progressKey =
      "lp" + String(i);

    String unlockKey =
      "lu" + String(i);

    String completeKey =
      "lc" + String(i);


    prefs.putInt(
      progressKey.c_str(),
      locationProgress[i]
    );


    prefs.putBool(
      unlockKey.c_str(),
      locationUnlocked[i]
    );


    prefs.putBool(
      completeKey.c_str(),
      locationCompleted[i]
    );
  }


  // =================================================
  // BOSS GALLERY / HISTORY
  //
  // Two bytes per boss:
  //   [0] encountered
  //   [1] defeated
  // =================================================

  uint8_t bossHistory[
    LOCATION_COUNT *
    4 *
    2
  ];


  int bossHistoryWrite =
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

      bossHistory[
        bossHistoryWrite++
      ] =
        bossEncountered[
          location
        ][
          bossIndex
        ]
          ? 1
          : 0;


      bossHistory[
        bossHistoryWrite++
      ] =
        bossDefeated[
          location
        ][
          bossIndex
        ]
          ? 1
          : 0;
    }
  }


  prefs.putBytes(
    "boss17",
    bossHistory,
    sizeof(
      bossHistory
    )
  );


  // =================================================
  // EXPEDITION
  // =================================================

  prefs.putBool(
    "exp",
    expeditionActive
  );


  prefs.putInt(
    "step",
    expeditionStep
  );


  // =================================================
  // DIRECTION
  // =================================================

  prefs.putInt(
    "dir",
    lastDirection
  );


  // =================================================
  // READING
  // =================================================

  prefs.putInt(
    "pages",
    totalPagesRead
  );


  prefs.putInt(
    "pageRem",
    pagesTowardStep
  );


  prefs.putInt(
    "steps",
    bankedSteps
  );


  // =================================================
  // AUTO
  // =================================================

  prefs.putBool(
    "auto",
    autoProgress
  );


  prefs.end();
}


// ==================================================
// LOAD
// ==================================================

void loadGame() {

  prefs.begin(
    "companions",
    true
  );


  int oldVersion =
    prefs.getInt(
      "version",
      0
    );


  bool collectionLoaded =
    false;


  bool progressLoaded =
    false;


  bool reserveLoaded =
    false;


  bool bossHistoryLoaded =
    false;


  bool runModelMigrated =
    false;


  // =================================================
  // COLLECTION - VERSION 12+
  // =================================================

  if (
    oldVersion >=
    12
  ) {

    if (
      prefs.getBytesLength(
        "uorder"
      ) ==
      sizeof(
        companionUnlockOrder
      )
    ) {

      prefs.getBytes(
        "uorder",
        companionUnlockOrder,
        sizeof(
          companionUnlockOrder
        )
      );


      unlockedCompanionCount =
        prefs.getInt(
          "ucount",
          STARTING_COMPANION_COUNT
        );


      collectionLoaded =
        validCompanionCollection();
    }


    if (
      collectionLoaded &&
      oldVersion >=
        14
    ) {

      progressLoaded =
        loadCompanionProgress();
    }


    if (
      oldVersion >=
      13 &&
      prefs.getBytesLength(
        "rorder"
      ) ==
      sizeof(
        reserveOrder
      )
    ) {

      prefs.getBytes(
        "rorder",
        reserveOrder,
        sizeof(
          reserveOrder
        )
      );


      reserveCount =
        prefs.getInt(
          "rcount",
          0
        );


      reserveLoaded =
        true;
    }
  }


  // =================================================
  // PARTY
  // =================================================

  if (
    oldVersion >= 6
  ) {

    teamSlots[0] =
      prefs.getInt(
        "ts0",
        0
      );


    teamSlots[1] =
      prefs.getInt(
        "ts1",
        1
      );


    teamSlots[2] =
      prefs.getInt(
        "ts2",
        2
      );
  }


  // =================================================
  // READING
  // =================================================

  totalPagesRead =
    prefs.getInt(
      "pages",
      0
    );


  pagesTowardStep =
    prefs.getInt(
      "pageRem",
      0
    );


  bankedSteps =
    prefs.getInt(
      "steps",
      0
    );


  // =================================================
  // LOCATION SAVE - VERSION 9+
  // =================================================

  if (
    oldVersion >= 9
  ) {

    currentLocation =
      prefs.getInt(
        "loc",
        LOC_WHISPERWOOD
      );


    for (
      int i = 0;
      i < LOCATION_COUNT;
      i++
    ) {

      String progressKey =
        "lp" + String(i);

      String unlockKey =
        "lu" + String(i);

      String completeKey =
        "lc" + String(i);


      locationProgress[i] =
        prefs.getInt(
          progressKey.c_str(),
          0
        );


      locationUnlocked[i] =
        prefs.getBool(
          unlockKey.c_str(),
          i == 0
        );


      locationCompleted[i] =
        prefs.getBool(
          completeKey.c_str(),
          false
        );
    }


    expeditionActive =
      prefs.getBool(
        "exp",
        false
      );


    autoProgress =
      prefs.getBool(
        "auto",
        false
      );
  }


  // =================================================
  // MIGRATE VERSION 6-8
  // =================================================

  else if (
    oldVersion >= 6
  ) {

    int oldStep =
      prefs.getInt(
        "step",
        0
      );


    if (
      oldStep < 0
    ) {
      oldStep = 0;
    }


    if (
      oldStep > 100
    ) {
      oldStep = 100;
    }


    currentLocation =
      LOC_WHISPERWOOD;


    for (
      int i = 0;
      i < LOCATION_COUNT;
      i++
    ) {

      locationProgress[i] =
        0;


      locationUnlocked[i] =
        false;


      locationCompleted[i] =
        false;
    }


    locationUnlocked[
      LOC_WHISPERWOOD
    ] =
      true;


    locationProgress[
      LOC_WHISPERWOOD
    ] =
      oldStep;


    if (
      oldStep >= 50
    ) {

      locationUnlocked[
        LOC_OPEN_PLAINS
      ] =
        true;
    }


    if (
      oldStep >= 100
    ) {

      locationCompleted[
        LOC_WHISPERWOOD
      ] =
        true;
    }


    expeditionActive =
      prefs.getBool(
        "exp",
        false
      );


    autoProgress =
      prefs.getBool(
        "auto",
        false
      );
  }


  // =================================================
  // FRESH SAVE
  // =================================================

  else {

    currentLocation =
      LOC_WHISPERWOOD;


    for (
      int i = 0;
      i < LOCATION_COUNT;
      i++
    ) {

      locationProgress[i] =
        0;


      locationUnlocked[i] =
        false;


      locationCompleted[i] =
        false;
    }


    locationUnlocked[
      LOC_WHISPERWOOD
    ] =
      true;


    expeditionActive =
      false;


    autoProgress =
      false;
  }


  // =================================================
  // BOSS GALLERY / HISTORY - VERSION 17+
  // =================================================

  if (
    oldVersion >=
      17 &&
    prefs.getBytesLength(
      "boss17"
    ) ==
      LOCATION_COUNT *
      4 *
      2
  ) {

    uint8_t bossHistory[
      LOCATION_COUNT *
      4 *
      2
    ];


    prefs.getBytes(
      "boss17",
      bossHistory,
      sizeof(
        bossHistory
      )
    );


    int bossHistoryRead =
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

        bossEncountered[
          location
        ][
          bossIndex
        ] =
          bossHistory[
            bossHistoryRead++
          ] !=
          0;


        bossDefeated[
          location
        ][
          bossIndex
        ] =
          bossHistory[
            bossHistoryRead++
          ] !=
          0;


        // Victory always implies the boss has been seen.
        if (
          bossDefeated[
            location
          ][
            bossIndex
          ]
        ) {

          bossEncountered[
            location
          ][
            bossIndex
          ] =
            true;
        }
      }
    }


    bossHistoryLoaded =
      true;
  }


  // =================================================
  // MIGRATE v16 BOSS GALLERY
  //
  // v16 only knew defeated/not-defeated. Every boss
  // already defeated is therefore also marked seen.
  // Undefeated bosses remain unseen because v16 did
  // not retain failed encounters.
  // =================================================

  else if (
    oldVersion >=
      16 &&
    prefs.getBytesLength(
      "boss16"
    ) ==
      LOCATION_COUNT *
      4
  ) {

    uint8_t oldBossHistory[
      LOCATION_COUNT *
      4
    ];


    prefs.getBytes(
      "boss16",
      oldBossHistory,
      sizeof(
        oldBossHistory
      )
    );


    int bossHistoryRead =
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

        bool defeated =
          oldBossHistory[
            bossHistoryRead++
          ] !=
          0;


        bossEncountered[
          location
        ][
          bossIndex
        ] =
          defeated;


        bossDefeated[
          location
        ][
          bossIndex
        ] =
          defeated;
      }
    }


    bossHistoryLoaded =
      true;
  }


  // =================================================
  // MIGRATE v15 BOSS GALLERY (16 BOSSES -> 32)
  //
  // v15 stored four locations x four bosses in boss15.
  // Preserve every one of those permanent victories and
  // initialise the four new zones as undiscovered.
  // =================================================

  else if (
    oldVersion >=
      15 &&
    prefs.getBytesLength(
      "boss15"
    ) ==
      16
  ) {

    uint8_t oldBossHistory[
      16
    ];


    prefs.getBytes(
      "boss15",
      oldBossHistory,
      sizeof(
        oldBossHistory
      )
    );


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
          location < 4
        ) {

          bool defeated =
            oldBossHistory[
              location *
              4 +
              bossIndex
            ] !=
            0;


          bossEncountered[
            location
          ][
            bossIndex
          ] =
            defeated;


          bossDefeated[
            location
          ][
            bossIndex
          ] =
            defeated;
        }


        else {

          bossEncountered[
            location
          ][
            bossIndex
          ] =
            false;


          bossDefeated[
            location
          ][
            bossIndex
          ] =
            false;
        }
      }
    }


    bossHistoryLoaded =
      true;
  }


  // =================================================
  // DIRECTION
  // =================================================

  if (
    oldVersion >= 11
  ) {

    lastDirection =
      prefs.getInt(
        "dir",
        DIR_NONE
      );
  }

  else {

    lastDirection =
      DIR_NONE;
  }


  prefs.end();


  // =================================================
  // COLLECTION MIGRATION / SAFETY
  //
  // Versions before 12 had only five placeholder companions.
  // v12/v13 collection state is preserved; only old generic
  // Level/XP training resets when migrating to v14.
  // Their indices do not safely map to the new
  // canonical 32-character roster, so migration
  // deliberately starts the new collection fresh
  // while preserving reading/location progress.
  // =================================================

  bool collectionReset =
    false;


  bool reserveRebuilt =
    false;


  bool bossHistoryRebuilt =
    false;


  if (
    !collectionLoaded
  ) {

    initializeCompanionCollection();


    collectionReset =
      true;


    progressLoaded =
      true;
  }


  else if (
    !progressLoaded
  ) {

    // v12/v13 -> v14:
    // Preserve collection order, unlocked companions,
    // Active team, Reserve ordering, reading progress
    // and location progress.
    //
    // Old generic Level/XP cannot map cleanly to the
    // new three-role training system, so earned role
    // training begins at zero.

    resetCompanionProgress();


    progressLoaded =
      true;


    collectionReset =
      true;
  }


  // =================================================
  // SAFETY - PARTY
  // =================================================

  if (
    !validTeamSlots()
  ) {

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


    collectionReset =
      true;
  }


  // =================================================
  // RESERVE MIGRATION / SAFETY
  //
  // v12 knew which companions were unlocked, but did
  // not yet persist a visible Reserve ordering.
  //
  // For v12 -> v13 migration we build it from earned
  // acquisition order, excluding the current Active
  // party. From then on it is saved independently so
  // swaps preserve exact Reserve positions.
  // =================================================

  if (
    !reserveLoaded ||
    !validReserveOrder()
  ) {

    buildReserveOrderFromUnlocked();


    reserveRebuilt =
      true;
  }


  // =================================================
  // SAFETY - LOCATION
  // =================================================

  if (
    !validLocation(
      currentLocation
    )
  ) {

    currentLocation =
      LOC_WHISPERWOOD;
  }


  // Whispering Woods can never be locked.

  locationUnlocked[
    LOC_WHISPERWOOD
  ] =
    true;


  // =================================================
  // BOSS GALLERY MIGRATION / SAFETY
  // =================================================

  if (
    !bossHistoryLoaded
  ) {

    rebuildBossHistoryFromProgress();


    bossHistoryRebuilt =
      true;
  }


  // =================================================
  // BOOKENDED ADVENTURE MIGRATION - VERSION 18
  //
  // Old builds allowed partial progress to sit in many
  // locations indefinitely. v18 keeps at most the
  // current location as the active unfinished run.
  // Other abandoned partial runs reset to 0 while all
  // permanent completion/boss/companion data remains.
  // =================================================

  if (
    oldVersion > 0 &&
    oldVersion < 18
  ) {

    for (
      int location = 0;
      location < LOCATION_COUNT;
      location++
    ) {

      if (
        location !=
          currentLocation &&
        locationProgress[
          location
        ] >
          0 &&
        locationProgress[
          location
        ] <
          100
      ) {

        locationProgress[
          location
        ] =
          0;


        runModelMigrated =
          true;
      }
    }


    if (
      locationProgress[
        currentLocation
      ] >
        0 &&
      locationProgress[
        currentLocation
      ] <
        100
    ) {

      expeditionActive =
        true;
    }


    runModelMigrated =
      true;
  }


  // =================================================
  // CANONICAL LOCATION ORDER - VERSION 19
  //
  // v19 changes progression order without changing the
  // stable internal LocationId values. Rebuild unlocks
  // from permanent 50% boss history after any older
  // bookended-run migration has finished.
  // =================================================

  rebuildLocationUnlocksForProgressionOrder();


  // =================================================
  // CURRENT EXPEDITION STEP
  // =================================================

  expeditionStep =
    locationProgress[
      currentLocation
    ];


  // =================================================
  // DIRECTION SAFETY
  // =================================================

  if (
    lastDirection < DIR_NONE ||
    lastDirection > DIR_WEST
  ) {

    lastDirection =
      DIR_NONE;
  }


  // =================================================
  // HOME STATE
  // =================================================

  if (
    !expeditionActive
  ) {

    clearExpeditionFatigue();


    lastDirection =
      DIR_NONE;
  }


  lastTravelText =
    "";


  // Persist the new version/collection immediately
  // after first migration so the shuffle never changes
  // on the next reboot.

  if (
    oldVersion <
      SAVE_VERSION ||
    collectionReset ||
    reserveRebuilt ||
    bossHistoryRebuilt ||
    runModelMigrated
  ) {

    saveGame();
  }
}
