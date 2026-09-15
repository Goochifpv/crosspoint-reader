#include <Arduino.h>

#include "GameState.h"
#include "EventContent.h"


// ==================================================
// RAW CONTENT ENTRY
//
// Internal structure used for authored scene text.
// ==================================================

struct RawEventText {
  const char* title;
  const char* line1;
  const char* line2;

  // Optional contextual artwork subject.
  //
  // This is deliberately NOT a filename. ArtSystem uses it
  // as a stable scene key and can still fall back to the
  // normal category/location artwork if no matching image
  // currently exists on the SD card.
  const char* artKey;
};


// ==================================================
// CONTENT ANTI-REPETITION
//
// A scene/result is kept out of circulation for the
// next 8 selections.
//
// This remembers the RAW template before {name} is
// substituted, so:
//
// "Pip finds a very good stick."
//
// and
//
// "Moss finds a very good stick."
//
// still count as the same underlying scene.
//
// History resets naturally when the reader reboots.
// ==================================================


constexpr int CONTENT_HISTORY_SIZE = 8;


static String sceneHistory[
  CONTENT_HISTORY_SIZE
];

static int sceneHistoryCount =
  0;

static int sceneHistoryWrite =
  0;


static String outcomeHistory[
  CONTENT_HISTORY_SIZE
];

static int outcomeHistoryCount =
  0;

static int outcomeHistoryWrite =
  0;


// ==================================================
// IS SOMETHING ALREADY IN A HISTORY?
// ==================================================

static bool isInHistory(
  const String &value,
  String history[],
  int historyCount
) {

  for (
    int i = 0;
    i < historyCount;
    i++
  ) {

    if (
      history[i] ==
      value
    ) {

      return true;
    }
  }


  return false;
}


// ==================================================
// REMEMBER A NEW HISTORY ENTRY
//
// Circular buffer:
//
// after 8 entries, the oldest entry is replaced.
// ==================================================

static void rememberHistory(
  const String &value,
  String history[],
  int &historyCount,
  int &historyWrite
) {

  if (
    value.length() == 0
  ) {

    return;
  }


  history[
    historyWrite
  ] =
    value;


  historyWrite++;


  if (
    historyWrite >=
    CONTENT_HISTORY_SIZE
  ) {

    historyWrite =
      0;
  }


  if (
    historyCount <
    CONTENT_HISTORY_SIZE
  ) {

    historyCount++;
  }
}


// ==================================================
// CREATE UNIQUE KEY FOR A SCENE TEMPLATE
// ==================================================

static String eventTemplateKey(
  const RawEventText &event
) {

  return
    String(
      event.title
    ) +
    "|" +
    String(
      event.line1
    ) +
    "|" +
    String(
      event.line2
    );
}


// ==================================================
// BUILD EVENT
// ==================================================

static EventText makeEvent(
  const RawEventText &raw,
  const String &actorName = ""
) {

  EventText result;


  result.title =
    raw.title;


  result.line1 =
    raw.line1;


  result.line2 =
    raw.line2;


  if (
    actorName.length() > 0
  ) {

    result.title.replace(
      "{name}",
      actorName
    );


    result.line1.replace(
      "{name}",
      actorName
    );


    result.line2.replace(
      "{name}",
      actorName
    );
  }


  return result;
}


// ==================================================
// PICK EVENT
//
// Try every entry in the relevant pool starting at
// a random point.
//
// Prefer anything that has NOT appeared amongst the
// last 8 scenes.
//
// If every possible entry is currently in history,
// reuse is allowed rather than getting stuck.
// ==================================================

static EventText pickEvent(
  const RawEventText* events,
  int count,
  const String &actorName = ""
) {

  if (
    count <= 0
  ) {

    return {
      "",
      "",
      ""
    };
  }


  int startIndex =
    random(
      0,
      count
    );


  int selectedIndex =
    startIndex;


  bool uniqueFound =
    false;


  for (
    int offset = 0;
    offset < count;
    offset++
  ) {

    int candidateIndex =
      (
        startIndex +
        offset
      ) %
      count;


    String candidateKey =
      eventTemplateKey(
        events[
          candidateIndex
        ]
      );


    if (
      !isInHistory(
        candidateKey,
        sceneHistory,
        sceneHistoryCount
      )
    ) {

      selectedIndex =
        candidateIndex;


      uniqueFound =
        true;


      break;
    }
  }


  // If the pool has been exhausted, selectedIndex
  // remains the original random choice.
  //
  // This matters for small content pools.

  String selectedKey =
    eventTemplateKey(
      events[
        selectedIndex
      ]
    );


  rememberHistory(
    selectedKey,
    sceneHistory,
    sceneHistoryCount,
    sceneHistoryWrite
  );


  return makeEvent(
    events[
      selectedIndex
    ],
    actorName
  );
}


// ==================================================
// PICK CONTEXTUAL RESULT LINE
// ==================================================

static String pickNamedLine(
  const char* const* lines,
  int count,
  const String &name
) {

  if (
    count <= 0
  ) {

    return "";
  }


  int startIndex =
    random(
      0,
      count
    );


  int selectedIndex =
    startIndex;


  for (
    int offset = 0;
    offset < count;
    offset++
  ) {

    int candidateIndex =
      (
        startIndex +
        offset
      ) %
      count;


    String candidateTemplate =
      String(
        lines[
          candidateIndex
        ]
      );


    if (
      !isInHistory(
        candidateTemplate,
        outcomeHistory,
        outcomeHistoryCount
      )
    ) {

      selectedIndex =
        candidateIndex;


      break;
    }
  }


  String selectedTemplate =
    String(
      lines[
        selectedIndex
      ]
    );


  rememberHistory(
    selectedTemplate,
    outcomeHistory,
    outcomeHistoryCount,
    outcomeHistoryWrite
  );


  String result =
    selectedTemplate;


  result.replace(
    "{name}",
    name
  );


  return result;
}


// ==================================================
// PICK RESULT LINE WITHOUT A CHARACTER NAME
//
// Used for things such as forced retreats.
// ==================================================

static String pickLine(
  const char* const* lines,
  int count
) {

  if (
    count <= 0
  ) {

    return "";
  }


  int startIndex =
    random(
      0,
      count
    );


  int selectedIndex =
    startIndex;


  for (
    int offset = 0;
    offset < count;
    offset++
  ) {

    int candidateIndex =
      (
        startIndex +
        offset
      ) %
      count;


    String candidateTemplate =
      String(
        lines[
          candidateIndex
        ]
      );


    if (
      !isInHistory(
        candidateTemplate,
        outcomeHistory,
        outcomeHistoryCount
      )
    ) {

      selectedIndex =
        candidateIndex;


      break;
    }
  }


  String result =
    String(
      lines[
        selectedIndex
      ]
    );


  rememberHistory(
    result,
    outcomeHistory,
    outcomeHistoryCount,
    outcomeHistoryWrite
  );


  return result;
}




// ==================================================
// ==================================================
//
// WHISPERING WOODS (internal id: WHISPERWOOD)
//
// ==================================================
// ==================================================


// ==================================================
// TRAVEL / SMALL MOMENTS
// ==================================================

static const RawEventText
WHISPERWOOD_TRAVEL[] = {

  {
    "ONWARD",
    "The trail bends between",
    "two enormous roots.",
    "forest_path"
  },

  {
    "QUIET PATH",
    "Only birdsong follows",
    "the team for a while."
  },

  {
    "UNDER THE CANOPY",
    "Green light filters",
    "through the leaves."
  },

  {
    "OLD TRAIL",
    "The team follows an",
    "almost forgotten path."
  },

  {
    "A LITTLE LOST",
    "The path definitely",
    "was here a moment ago."
  },

  {
    "VERY GOOD STICK",
    "{name} finds a very",
    "good stick."
  },

  {
    "SUSPICIOUS MUSHROOM",
    "{name} is certain that",
    "mushroom just moved.",
    "unusual_phenomenon"
  },

  {
    "FOREST BUSINESS",
    "A squirrel watches them",
    "with deep suspicion.",
    "squirrel_watch"
  },

  {
    "TINY PARADE",
    "A line of beetles crosses",
    "the path ahead.",
    "beetle_path"
  },

  {
    "SOMEWHERE NEARBY",
    "Something sneezes behind",
    "a tree. Nobody checks."
  },

  {
    "STREAM CROSSING",
    "The water is slightly",
    "deeper than expected.",
    "stream_crossing"
  },

  {
    "A NICE CLEARING",
    "Nothing happens here.",
    "It is rather pleasant.",
    "quiet_clearing"
  }
};


// ==================================================
// BATTLES
// ==================================================

static const RawEventText
WHISPERWOOD_BATTLES[] = {

  {
    "THORNBEAST",
    "A thornbeast crashes",
    "through the undergrowth.",
    "thornbeast"
  },

  {
    "BRAMBLE HOUND",
    "A bramble hound blocks",
    "the narrow forest trail.",
    "bramble_hound"
  },

  {
    "ROOTLING PACK",
    "Tiny angry rootlings",
    "pour from the bushes.",
    "rootling_pack"
  },

  {
    "MOSS STALKER",
    "A moss-covered creature",
    "drops from a branch.",
    "moss_stalker"
  },

  {
    "FERN CRAWLER",
    "The ferns erupt as a",
    "crawler charges out.",
    "fern_crawler"
  },

  {
    "BARKBACK",
    "Something disguised as",
    "a log suddenly stands up.",
    "barkback"
  }
};


// ==================================================
// FRIENDLY
// ==================================================

static const RawEventText
WHISPERWOOD_FRIENDLY[] = {

  {
    "A TINY VISITOR",
    "A curious forest bunny",
    "hops over to investigate.",
    "forest_bunny"
  },

  {
    "FEATHERLING",
    "A tiny feathered creature",
    "lands beside the team.",
    "featherling"
  },

  {
    "FOREST FOX",
    "A silver-tailed fox",
    "quietly approaches.",
    "forest_fox"
  },

  {
    "LOST HEDGEHOG",
    "A small hedgehog appears",
    "to be completely lost.",
    "hedgehog"
  },

  {
    "CURIOUS OWL",
    "A round little owl",
    "follows them from above.",
    "forest_owl"
  }
};


// ==================================================
// TREASURE
// ==================================================

static const RawEventText
WHISPERWOOD_TREASURE[] = {

  {
    "BURIED CACHE",
    "Something glints beneath",
    "an enormous tree root.",
    "root_cache"
  },

  {
    "HOLLOW TREE",
    "A hollow trunk contains",
    "a tiny hidden bundle.",
    "hollow_tree"
  },

  {
    "FOREST COINS",
    "Old coins lie scattered",
    "beneath fallen leaves.",
    "hidden_cache"
  },

  {
    "ABANDONED PACK",
    "A forgotten little pack",
    "hangs from a branch.",
    "lost_supplies"
  },

  {
    "SHINY THING",
    "Something extremely shiny",
    "is buried in the moss.",
    "moss_glint"
  }
};


// ==================================================
// NPC
// ==================================================

static const RawEventText
WHISPERWOOD_NPCS[] = {

  {
    "POTION MAKER",
    "A tiny shop sits beneath",
    "the roots of an old tree.",
    "root_shop"
  },

  {
    "MUSHROOM SELLER",
    "A merchant has somehow",
    "set up shop out here.",
    "forest_stall"
  },

  {
    "FOREST GUIDE",
    "An elderly traveller",
    "knows these woods well.",
    "forest_guide"
  },

  {
    "TEA CART",
    "Someone is selling tea",
    "in the middle of nowhere.",
    "tea_cart"
  }
};


// ==================================================
// DISCOVERIES
// ==================================================

static const RawEventText
WHISPERWOOD_DISCOVERIES[] = {

  {
    "STONE DOOR",
    "An overgrown stone door",
    "stands between the trees.",
    "mysterious_gate"
  },

  {
    "GIANT FOOTPRINT",
    "A footprint crosses",
    "almost the entire path.",
    "giant_footprint"
  },

  {
    "OLD SHRINE",
    "A forgotten shrine hides",
    "beneath thick ivy.",
    "forest_shrine"
  },

  {
    "BLUE FLOWERS",
    "A clearing is filled with",
    "faintly glowing flowers.",
    "tranquil_grove"
  },

  {
    "TREE HOUSE",
    "A tiny empty house sits",
    "high in an ancient tree.",
    "abandoned_hut"
  }
};


// ==================================================
// REST
// ==================================================

static const RawEventText
WHISPERWOOD_REST[] = {

  {
    "A QUIET MOMENT",
    "The team finds somewhere",
    "warm beneath the trees."
  },

  {
    "PICNIC ROCK",
    "A flat sunny rock seems",
    "made for taking a break.",
    "sunny_rock"
  },

  {
    "FOREST CAMP",
    "The team settles beside",
    "a gentle little stream.",
    "campfire"
  },

  {
    "OLD SHELTER",
    "An abandoned shelter",
    "keeps the wind away.",
    "old_shelter"
  },

  {
    "AFTERNOON SUN",
    "For a few minutes, nobody",
    "feels like moving."
  }
};


// ==================================================
// RARE
// ==================================================

static const RawEventText
WHISPERWOOD_RARE[] = {

  {
    "GLOWING CREATURE",
    "Something luminous watches",
    "silently from the trees.",
    "luminous_creature"
  },

  {
    "WALKING TREE",
    "A distant tree appears",
    "to change position.",
    "walking_tree"
  },

  {
    "GOLDEN STAG",
    "A golden stag watches",
    "from across a clearing.",
    "golden_stag"
  },

  {
    "STRANGE LANTERN",
    "A lantern moves between",
    "trees with nobody near it.",
    "floating_lantern"
  },

  {
    "THE WAVE",
    "Something in the trees",
    "appears to wave hello.",
    "waving_shape"
  }
};


// ==================================================
// ==================================================
//
// ANCIENT RUINS (internal id: OPEN PLAINS)
//
// ==================================================
// ==================================================


// ==================================================
// TRAVEL
// ==================================================

static const RawEventText
OPEN_PLAINS_TRAVEL[] = {

  {
    "LONG GRASS",
    "The grass rolls in waves",
    "beneath the open sky.",
    "long_grass"
  },

  {
    "LONE TREE",
    "A single old tree stands",
    "far out in the grass.",
    "lone_tree"
  },

  {
    "OLD FENCE",
    "A weathered fence follows",
    "the trail for a while.",
    "old_fence"
  },

  {
    "SHALLOW STREAM",
    "A clear little stream",
    "cuts across the route.",
    "shallow_stream"
  },

  {
    "DISTANT HILLS",
    "Low hills rise slowly",
    "along the horizon.",
    "distant_hills"
  },

  {
    "ABANDONED CART",
    "An old cart leans",
    "beside the track.",
    "abandoned_cart"
  },

  {
    "STANDING STONES",
    "Tall stones watch over",
    "the empty grassland.",
    "standing_stones"
  },

  {
    "OLD WINDMILL",
    "A windmill turns lazily",
    "in the distance.",
    "windmill"
  },

  {
    "RUINED TOWER",
    "A broken tower marks",
    "the far side of the plain.",
    "ruined_tower"
  },

  {
    "VERY BIG SKY",
    "{name} decides the sky",
    "is showing off today.",
    "grass_path"
  }
};


// ==================================================
// BATTLES
// ==================================================

static const RawEventText
OPEN_PLAINS_BATTLES[] = {

  {
    "PRAIRIE HOUND",
    "A lean grassland hound",
    "blocks the trail.",
    "prairie_hound"
  },

  {
    "THISTLE BOAR",
    "A bristling thistle boar",
    "charges from the grass.",
    "thistle_boar"
  },

  {
    "BURROWER",
    "The ground bursts open",
    "beneath the team.",
    "burrower"
  },

  {
    "HORNBEAST",
    "A broad-horned beast",
    "refuses to move aside.",
    "hornbeast"
  },

  {
    "GRASS STALKER",
    "Something low and fast",
    "rushes through the grass.",
    "grass_stalker"
  },

  {
    "SCRAPBACK",
    "A creature wrapped in",
    "old metal plates appears.",
    "scrapback"
  }
};


// ==================================================
// FRIENDLY
// ==================================================

static const RawEventText
OPEN_PLAINS_FRIENDLY[] = {

  {
    "FIELD MOUSE",
    "A tiny field mouse",
    "appears beside the path.",
    "field_mouse"
  },

  {
    "SKYLARK",
    "A bright little bird",
    "lands surprisingly close.",
    "skylark"
  },

  {
    "PLAINS FOX",
    "A curious fox watches",
    "from the long grass.",
    "plains_fox"
  },

  {
    "SMALL GOAT",
    "A very determined goat",
    "has decided to follow.",
    "small_goat"
  },

  {
    "ROUND BIRD",
    "A remarkably round bird",
    "refuses to be hurried.",
    "round_bird"
  }
};


// ==================================================
// TREASURE
// ==================================================

static const RawEventText
OPEN_PLAINS_TREASURE[] = {

  {
    "CART CACHE",
    "A loose board hides",
    "something in an old cart.",
    "cart_cache"
  },

  {
    "FENCEPOST STASH",
    "A hollow fencepost holds",
    "a tiny wrapped bundle.",
    "fencepost_stash"
  },

  {
    "OLD MILESTONE",
    "Something is tucked behind",
    "a cracked old milestone.",
    "old_milestone"
  },

  {
    "STREAM GLINT",
    "Something bright flashes",
    "beneath the shallow water.",
    "stream_glint"
  },

  {
    "BURIED TIN",
    "A rusted little tin",
    "sticks out of the earth.",
    "buried_tin"
  }
};


// ==================================================
// NPC
// ==================================================

static const RawEventText
OPEN_PLAINS_NPCS[] = {

  {
    "SHEPHERD",
    "A travelling shepherd",
    "waves from a low hill.",
    "shepherd"
  },

  {
    "BEEKEEPER",
    "A beekeeper has somehow",
    "set up hives out here.",
    "beekeeper"
  },

  {
    "CART MERCHANT",
    "A cheerful merchant",
    "rests beside a small cart.",
    "cart_merchant"
  },

  {
    "ROAD MENDER",
    "Someone is repairing",
    "a road nobody owns.",
    "road_mender"
  }
};


// ==================================================
// DISCOVERY
// ==================================================

static const RawEventText
OPEN_PLAINS_DISCOVERIES[] = {

  {
    "STONE CIRCLE",
    "A ring of weathered stones",
    "stands in the grass.",
    "standing_stones"
  },

  {
    "OLD WINDMILL",
    "An abandoned windmill",
    "still turns in the breeze.",
    "windmill"
  },

  {
    "RUINED TOWER",
    "A lonely ruined tower",
    "overlooks the plain.",
    "ruined_tower"
  },

  {
    "ANCIENT OAK",
    "A vast old oak stands",
    "where no forest remains.",
    "lone_tree"
  },

  {
    "OLD BRIDGE",
    "A small stone bridge",
    "crosses a narrow stream.",
    "shallow_stream"
  }
};


// ==================================================
// REST
// ==================================================

static const RawEventText
OPEN_PLAINS_REST[] = {

  {
    "TREE SHADE",
    "The lone tree provides",
    "exactly enough shade.",
    "lone_tree"
  },

  {
    "STREAM BANK",
    "The team settles beside",
    "a slow clear stream.",
    "shallow_stream"
  },

  {
    "OLD BARN",
    "An empty little barn",
    "keeps the wind away.",
    "old_barn"
  },

  {
    "HILLTOP",
    "The team stops where",
    "they can see for miles.",
    "distant_hills"
  },

  {
    "SOFT GRASS",
    "The grass here is almost",
    "too comfortable."
  }
};


// ==================================================
// RARE
// ==================================================

static const RawEventText
OPEN_PLAINS_RARE[] = {

  {
    "WHITE STAG",
    "A pale stag watches",
    "from the far grass.",
    "white_stag"
  },

  {
    "WANDERING LIGHTS",
    "Small lights drift slowly",
    "across the plain.",
    "wandering_lights"
  },

  {
    "CLOUD SHADOW",
    "A huge shadow passes",
    "without a cloud above.",
    "cloud_shadow"
  },

  {
    "SINGING STONES",
    "The standing stones hum",
    "when the wind changes.",
    "standing_stones"
  },

  {
    "GOLDEN FIELD",
    "For one brief moment",
    "the whole plain glows.",
    "long_grass"
  }
};


// ==================================================
// ==================================================
//
// EMBERFIELD PLAINS (internal id: DUSTY DESERT)
//
// ==================================================
// ==================================================


// ==================================================
// TRAVEL
// ==================================================

static const RawEventText
DUSTY_DESERT_TRAVEL[] = {

  {
    "DUNE PATH",
    "The trail winds between",
    "low windblown dunes.",
    "dune_path"
  },

  {
    "ROCK SPINE",
    "A ridge of dark stone",
    "cuts across the sand.",
    "rock_spine"
  },

  {
    "DRY WASH",
    "The team follows an old",
    "dry riverbed for a while.",
    "dry_wash"
  },

  {
    "HEAT HAZE",
    "The horizon refuses",
    "to stay in one place.",
    "heat_haze"
  },

  {
    "OLD TRACK",
    "Faded wheel marks lead",
    "toward distant rocks.",
    "old_track"
  },

  {
    "DESERT BLOOM",
    "A few tiny flowers",
    "have ignored the heat.",
    "desert_bloom"
  },

  {
    "WIND-CARVED ROCK",
    "The wind has sculpted",
    "the stone into odd shapes.",
    "wind_carved_rock"
  },

  {
    "SAND EVERYWHERE",
    "{name} discovers sand",
    "has reached everywhere.",
    "dune_path"
  },

  {
    "DARK OUTCROP",
    "Black rock rises sharply",
    "from the pale desert.",
    "dark_outcrop"
  },

  {
    "LONG SHADOWS",
    "The lowering sun stretches",
    "everything across the sand.",
    "long_shadows"
  }
};


// ==================================================
// BATTLES
// ==================================================

static const RawEventText
DUSTY_DESERT_BATTLES[] = {

  {
    "SAND CRAWLER",
    "A sand crawler bursts",
    "from beneath the surface.",
    "sand_crawler"
  },

  {
    "DUNE HOUND",
    "A lean dune hound",
    "appears on the ridge.",
    "dune_hound"
  },

  {
    "SUN SCORPION",
    "A huge desert scorpion",
    "scrambles over the rocks.",
    "sun_scorpion"
  },

  {
    "ROCK LIZARD",
    "A heavy scaled creature",
    "drops from a stone ledge.",
    "rock_lizard"
  },

  {
    "GLASSBACK",
    "A glittering glassback",
    "blocks the dry wash.",
    "glassback"
  },

  {
    "DUST BURROWER",
    "The sand erupts as",
    "something charges upward.",
    "dust_burrower"
  }
};


// ==================================================
// FRIENDLY
// ==================================================

static const RawEventText
DUSTY_DESERT_FRIENDLY[] = {

  {
    "DESERT FOX",
    "A tiny desert fox",
    "watches from the shade.",
    "desert_fox"
  },

  {
    "SAND MOUSE",
    "A long-eared little mouse",
    "hops over to investigate.",
    "sand_mouse"
  },

  {
    "SUN LIZARD",
    "A bright-eyed lizard",
    "rests on a warm stone.",
    "sun_lizard"
  },

  {
    "DUNE BEETLE",
    "A polished black beetle",
    "pushes on with purpose.",
    "dune_beetle"
  },

  {
    "LOST TORTOISE",
    "A small tortoise seems",
    "deeply unimpressed by this.",
    "tortoise"
  }
};


// ==================================================
// TREASURE
// ==================================================

static const RawEventText
DUSTY_DESERT_TREASURE[] = {

  {
    "HALF-BURIED CHEST",
    "The corner of a small chest",
    "sticks out of the sand.",
    "half_buried_chest"
  },

  {
    "CARAVAN CACHE",
    "A marked stone hides",
    "an old traveller's cache.",
    "caravan_cache"
  },

  {
    "OLD CANTEEN",
    "A battered canteen has",
    "something tucked inside.",
    "old_canteen"
  },

  {
    "FOSSIL POCKET",
    "A cracked rock reveals",
    "something carefully hidden.",
    "fossil_pocket"
  },

  {
    "DESERT GLASS",
    "A dark glassy fragment",
    "gleams beneath the dust.",
    "desert_glass"
  }
};


// ==================================================
// NPC
// ==================================================

static const RawEventText
DUSTY_DESERT_NPCS[] = {

  {
    "CARAVAN TRADER",
    "A travelling trader",
    "rests beside a loaded cart.",
    "caravan_trader"
  },

  {
    "WELL KEEPER",
    "A weathered keeper tends",
    "a tiny desert well.",
    "well_keeper"
  },

  {
    "DESERT GUIDE",
    "A local guide knows",
    "which tracks are real.",
    "desert_guide"
  },

  {
    "TINKERER",
    "Someone has built a workshop",
    "beneath a canvas awning.",
    "tinkerer"
  }
};


// ==================================================
// DISCOVERY
// ==================================================

static const RawEventText
DUSTY_DESERT_DISCOVERIES[] = {

  {
    "HIDDEN OASIS",
    "Palm shadows reveal",
    "a small hidden pool.",
    "hidden_oasis"
  },

  {
    "STONE ARCH",
    "A natural arch rises",
    "from the empty desert.",
    "stone_arch"
  },

  {
    "FOSSIL BED",
    "Ancient shapes cover",
    "a slab of exposed stone.",
    "fossil_bed"
  },

  {
    "OLD WAYSTATION",
    "A ruined waystation",
    "sits beside the old track.",
    "old_waystation"
  },

  {
    "CANYON GATE",
    "Two dark cliffs form",
    "a narrow passage ahead.",
    "canyon_gate"
  }
};


// ==================================================
// REST
// ==================================================

static const RawEventText
DUSTY_DESERT_REST[] = {

  {
    "SHADE ROCK",
    "A broad rock offers",
    "a precious patch of shade.",
    "shade_rock"
  },

  {
    "OASIS EDGE",
    "The team rests beside",
    "cool still water.",
    "hidden_oasis"
  },

  {
    "OLD AWNING",
    "A faded canvas shelter",
    "still keeps off the sun.",
    "old_awning"
  },

  {
    "COOL CAVE",
    "A shallow rock cave",
    "is wonderfully cool.",
    "cool_cave"
  },

  {
    "SUNSET RIDGE",
    "The heat finally fades",
    "as the sun drops.",
    "sunset_ridge"
  }
};


// ==================================================
// RARE
// ==================================================

static const RawEventText
DUSTY_DESERT_RARE[] = {

  {
    "MIRAGE CARAVAN",
    "A distant caravan appears",
    "and then simply vanishes.",
    "mirage_caravan"
  },

  {
    "SINGING DUNES",
    "The sand begins to hum",
    "beneath the team's feet.",
    "singing_dunes"
  },

  {
    "GLASS STORM",
    "Tiny shining fragments",
    "dance inside a dust devil.",
    "glass_storm"
  },

  {
    "GIANT SHADOW",
    "Something enormous passes",
    "across the desert floor.",
    "giant_shadow"
  },

  {
    "NIGHT BLOOM",
    "Pale flowers open",
    "all at once in the dusk.",
    "night_bloom"
  }
};


// ==================================================
// ==================================================
//
// STORM COAST
//
// ==================================================
// ==================================================


// ==================================================
// TRAVEL
// ==================================================

static const RawEventText
COAST_TRAVEL[] = {

  {
    "ALONG THE COAST",
    "The team follows",
    "the rocky shoreline.",
    "rocky_shoreline"
  },

  {
    "HEAVY WAVES",
    "Waves crash nearby",
    "as they travel.",
    "heavy_waves"
  },

  {
    "GREY SKIES",
    "The party walks beneath",
    "heavy storm clouds.",
    "storm_clouds"
  },

  {
    "COASTAL PATH",
    "The narrow trail follows",
    "the edge of the cliffs.",
    "coastal_path"
  },

  {
    "SEA SPRAY",
    "A sudden gust covers",
    "everyone in sea spray.",
    "sea_spray"
  },

  {
    "SHELL COLLECTION",
    "{name} finds a shell",
    "that is almost impressive.",
    "shell"
  },

  {
    "BIG WAVE",
    "Everyone sees the wave.",
    "Nobody moves fast enough.",
    "big_wave"
  },

  {
    "CRAB BUSINESS",
    "A crab blocks the path.",
    "It refuses to negotiate.",
    "crab"
  },

  {
    "WINDY",
    "Walking forward currently",
    "requires considerable effort.",
    "windy_coast"
  },

  {
    "CALM WATER",
    "For a little while",
    "the sea becomes peaceful.",
    "calm_water"
  }
};


// ==================================================
// BATTLES
// ==================================================

static const RawEventText
COAST_BATTLES[] = {

  {
    "REEFCLAW",
    "A Reefclaw rushes",
    "from the surf.",
    "reefclaw"
  },

  {
    "CLIFF STALKER",
    "Something leaps down",
    "from the rocks above.",
    "cliff_stalker"
  },

  {
    "TIDE CRAWLER",
    "A huge crawler emerges",
    "from a tidal pool.",
    "tide_crawler"
  },

  {
    "SHELLBACK",
    "A heavily armoured beast",
    "blocks the coastal path.",
    "shellback"
  },

  {
    "STORM HOUND",
    "A storm hound appears",
    "through the sea spray.",
    "storm_hound"
  },

  {
    "ROCK CRAB",
    "An enormous crab has",
    "strong opinions about this.",
    "rock_crab"
  }
};


// ==================================================
// FRIENDLY
// ==================================================

static const RawEventText
COAST_FRIENDLY[] = {

  {
    "STRANDED PUFFIN",
    "A soaked little bird",
    "needs some assistance.",
    "puffin"
  },

  {
    "TINY SEAL",
    "A tiny seal watches",
    "from a sheltered pool.",
    "seal"
  },

  {
    "COAST FOX",
    "A windswept fox follows",
    "the party along the cliffs.",
    "coast_fox"
  },

  {
    "HERMIT CRAB",
    "A very ambitious crab",
    "has chosen a huge shell.",
    "hermit_crab"
  },

  {
    "SEA BIRD",
    "An exhausted sea bird",
    "lands beside the team.",
    "sea_bird"
  }
};


// ==================================================
// TREASURE
// ==================================================

static const RawEventText
COAST_TREASURE[] = {

  {
    "SHIPWRECK",
    "An old shipwreck has",
    "washed onto the rocks.",
    "shipwreck"
  },

  {
    "TIDAL CACHE",
    "A small sealed box sits",
    "inside a tidal pool.",
    "tidal_cache"
  },

  {
    "BOTTLE",
    "A bottle contains something",
    "that isn't a message.",
    "bottle"
  },

  {
    "SMUGGLER'S STASH",
    "A hidden cliff hollow",
    "contains an old stash.",
    "smuggler_stash"
  },

  {
    "DRIFTWOOD CHEST",
    "A battered little chest",
    "is wedged beneath driftwood.",
    "driftwood_chest"
  }
};


// ==================================================
// NPC
// ==================================================

static const RawEventText
COAST_NPCS[] = {

  {
    "OLD SAILOR",
    "An old sailor waves",
    "from beside a small fire.",
    "old_sailor"
  },

  {
    "LIGHTHOUSE KEEPER",
    "A lighthouse keeper",
    "offers the team shelter.",
    "lighthouse_keeper"
  },

  {
    "FISHERMAN",
    "A cheerful fisherman",
    "has caught absolutely nothing.",
    "fisherman"
  },

  {
    "TRAVELLING COOK",
    "Someone has established",
    "a tiny seaside kitchen.",
    "seaside_kitchen"
  }
};


// ==================================================
// DISCOVERY
// ==================================================

static const RawEventText
COAST_DISCOVERIES[] = {

  {
    "SEA CAVE",
    "A narrow cave appears",
    "as the tide retreats.",
    "sea_cave"
  },

  {
    "OLD LIGHTHOUSE",
    "A ruined lighthouse stands",
    "on a distant headland.",
    "old_lighthouse"
  },

  {
    "CLIFF STAIRS",
    "Ancient steps descend",
    "toward the water.",
    "cliff_stairs"
  },

  {
    "STONE CIRCLE",
    "A ring of standing stones",
    "overlooks the sea.",
    "stone_circle"
  },

  {
    "HIDDEN BEACH",
    "A tiny untouched beach",
    "lies beneath the cliffs.",
    "hidden_beach"
  }
};


// ==================================================
// REST
// ==================================================

static const RawEventText
COAST_REST[] = {

  {
    "SHELTERED COVE",
    "The storm briefly clears",
    "above a quiet cove.",
    "sheltered_cove"
  },

  {
    "DRIFTWOOD FIRE",
    "A small fire makes",
    "the coast almost cosy.",
    "driftwood_fire"
  },

  {
    "CLIFF SHELTER",
    "A rocky overhang keeps",
    "the rain away.",
    "cliff_shelter"
  },

  {
    "SUN BREAK",
    "Sunlight unexpectedly",
    "breaks through the clouds.",
    "sun_break"
  },

  {
    "QUIET BEACH",
    "The team finds a stretch",
    "of surprisingly calm sand.",
    "quiet_beach"
  }
};


// ==================================================
// RARE
// ==================================================

static const RawEventText
COAST_RARE[] = {

  {
    "GHOST SHIP",
    "A silent ship appears",
    "far beyond the breakers.",
    "ghost_ship"
  },

  {
    "GIANT SHADOW",
    "Something enormous passes",
    "beneath the water.",
    "giant_shadow"
  },

  {
    "LIGHTNING GLASS",
    "Lightning strikes the sand",
    "and leaves glass behind.",
    "lightning_glass"
  },

  {
    "SINGING SEA",
    "For a moment, the waves",
    "sound almost like voices.",
    "singing_sea"
  },

  {
    "DISTANT LIGHT",
    "A light flashes far out",
    "at sea. Twice.",
    "distant_light"
  }
};


// ==================================================
// MISTY MARSHES (internal id: MISTY MARSH)
// ==================================================

static const RawEventText MISTY_MARSH_TRAVEL[] = {
  {"REED PATH", "The trail narrows between", "walls of silver reeds.", "reed_path"},
  {"BOARDWALK", "Old boards creak above", "black marsh water.", "boardwalk"},
  {"FOG BANK", "A wall of mist rolls", "slowly across the path.", "fog_bank"},
  {"FLOODED TRACK", "The old track disappears", "under ankle-deep water.", "flooded_track"},
  {"CROOKED TREES", "Twisted trees lean over", "a glassy pool.", "crooked_trees"},
  {"LILY CHANNEL", "Broad lily pads cover", "a narrow channel.", "lily_channel"},
  {"MUDDY CAUSEWAY", "A raised strip of earth", "crosses the wetland.", "muddy_causeway"},
  {"DISTANT HUT", "A tiny hut appears", "and vanishes in fog.", "distant_hut"}
};

static const RawEventText MISTY_MARSH_BATTLES[] = {
  {"BOG HOUND", "A mud-caked hound shape", "bursts from the reeds.", "bog_hound"},
  {"REED SERPENT", "A long scaled body", "slides across the boardwalk.", "reed_serpent"},
  {"MIRE CRAB", "A huge marsh crab", "clacks from the shallows.", "mire_crab"},
  {"SWAMP BOAR", "A broad-backed boar", "charges through the sedge.", "swamp_boar"},
  {"LEECH SWARM", "The water suddenly fills", "with hungry black shapes.", "leech_swarm"},
  {"FEN STALKER", "Something tall moves", "between the dead trees.", "fen_stalker"}
};

static const RawEventText MISTY_MARSH_FRIENDLY[] = {
  {"MARSH OTTER", "A curious marsh otter", "paddles alongside the team.", "marsh_otter"},
  {"REED BIRD", "A bright little bird", "lands on a pack strap.", "reed_bird"},
  {"MUD TURTLE", "An ancient-looking turtle", "blocks the boardwalk.", "mud_turtle"},
  {"FEN DEER", "A pale deer watches", "quietly through the mist.", "fen_deer"},
  {"FIREFLIES", "A cloud of fireflies", "follows for a while.", "fireflies"}
};

static const RawEventText MISTY_MARSH_TREASURE[] = {
  {"SUNKEN SATCHEL", "A leather satchel rests", "beneath clear shallow water.", "sunken_satchel"},
  {"HOLLOW STUMP", "Something glints inside", "a hollow old stump.", "hollow_stump"},
  {"BOARDWALK CACHE", "One loose plank hides", "a carefully wrapped bundle.", "boardwalk_cache"},
  {"BOG IRON", "A dark metallic lump", "protrudes from the peat.", "bog_iron"},
  {"LOST LANTERN", "An ornate lantern hangs", "from a crooked branch.", "lost_lantern"}
};

static const RawEventText MISTY_MARSH_NPCS[] = {
  {"HERBALIST", "A marsh herbalist gathers", "plants beside the water.", "herbalist"},
  {"FERRY KEEPER", "A tiny flat-bottomed boat", "waits beside its keeper.", "ferry_keeper"},
  {"FEN GUIDE", "A local guide knows", "which ground is actually ground.", "fen_guide"},
  {"TEA HUT", "Warm light spills from", "a hut on stilts.", "tea_hut"}
};

static const RawEventText MISTY_MARSH_DISCOVERIES[] = {
  {"SUNKEN ROAD", "Stone paving continues", "beneath the dark water.", "sunken_road"},
  {"BELL TREE", "Tiny old bells hang", "from a dead tree.", "bell_tree"},
  {"STONE EYES", "Two carved stone eyes", "peer from the peat.", "stone_eyes"},
  {"HIDDEN ISLAND", "A dry little island", "sits behind the reeds.", "hidden_island"},
  {"OLD SHRINE", "A mossy shrine stands", "where paths should not meet.", "old_shrine"}
};

static const RawEventText MISTY_MARSH_REST[] = {
  {"DRY ROOTS", "Huge exposed roots make", "a surprisingly dry seat.", "dry_roots"},
  {"STILT SHELTER", "An empty shelter stands", "above the wet ground.", "stilt_shelter"},
  {"WARM SPRING", "A small warm spring", "steams in the cool mist.", "warm_spring"},
  {"QUIET ISLET", "A grassy islet offers", "a peaceful place to stop.", "quiet_islet"}
};

static const RawEventText MISTY_MARSH_RARE[] = {
  {"WILL-O-WISP", "A blue light waits", "just beyond the reeds.", "will_o_wisp"},
  {"UPSIDE-DOWN RAIN", "Droplets rise briefly", "from the marsh surface.", "upside_down_rain"},
  {"MIRROR POOL", "One pool reflects stars", "in the middle of day.", "mirror_pool"},
  {"WALKING HUT", "The distant hut appears", "to have moved again.", "walking_hut"},
  {"WHISPERING REEDS", "The reeds murmur something", "almost understandable.", "whispering_reeds"}
};


// ==================================================
// FROSTPEAK MOUNTAINS (internal id: FROSTPEAK)
// ==================================================

static const RawEventText FROSTPEAK_TRAVEL[] = {
  {"SNOW TRAIL", "A narrow trail winds", "between deep snowdrifts.", "snow_trail"},
  {"PINE RIDGE", "Dark pines line", "a wind-cut ridge.", "pine_ridge"},
  {"ROPE BRIDGE", "A rope bridge crosses", "a white ravine.", "rope_bridge"},
  {"ICE SHELF", "The route skirts", "a blue wall of ice.", "ice_shelf"},
  {"MOUNTAIN PASS", "The pass climbs toward", "a wall of cloud.", "mountain_pass"},
  {"FROZEN STREAM", "A frozen stream makes", "a smooth silver path.", "frozen_stream"},
  {"SNOW FIELD", "Fresh snow has erased", "every previous footprint.", "snow_field"},
  {"HIGH CABIN", "A lonely cabin clings", "to the mountainside.", "high_cabin"}
};

static const RawEventText FROSTPEAK_BATTLES[] = {
  {"ICE WOLF", "A pale wolf steps", "silently from the snow.", "ice_wolf"},
  {"CLIFF RAM", "A huge horned ram", "guards the narrow trail.", "cliff_ram"},
  {"SNOW CAT", "A white mountain cat", "drops from a ledge.", "snow_cat"},
  {"FROST SERPENT", "A scaled shape coils", "beneath powdery snow.", "frost_serpent"},
  {"ICE BEETLE", "A plated beetle scrapes", "across the frozen rock.", "ice_beetle"},
  {"RIDGE HUNTER", "A winged hunter circles", "above the exposed ridge.", "ridge_hunter"}
};

static const RawEventText FROSTPEAK_FRIENDLY[] = {
  {"SNOW HARE", "A snow hare follows", "at a very safe distance.", "snow_hare"},
  {"MOUNTAIN GOAT", "A shaggy goat seems", "unimpressed by the climb.", "mountain_goat"},
  {"PINE MARTEN", "A pine marten investigates", "the team's supplies.", "pine_marten"},
  {"WHITE OWL", "A white owl glides", "onto a nearby post.", "white_owl"},
  {"YAK CALF", "A lost shaggy calf", "calls from the snow.", "yak_calf"}
};

static const RawEventText FROSTPEAK_TREASURE[] = {
  {"ICE CACHE", "A sealed case is", "frozen into clear ice.", "ice_cache"},
  {"CLIMBER'S PACK", "An old pack rests", "beneath a rocky overhang.", "climbers_pack"},
  {"FROZEN LOCKBOX", "A little metal box", "is buried in snow.", "frozen_lockbox"},
  {"CRYSTAL POCKET", "A crack in the rock", "sparkles with crystals.", "crystal_pocket"},
  {"SUMMIT TOKEN", "A weathered token sits", "inside a stone cairn.", "summit_token"}
};

static const RawEventText FROSTPEAK_NPCS[] = {
  {"MOUNTAIN GUIDE", "A bundled guide appears", "from behind a snowbank.", "mountain_guide"},
  {"HERMIT", "Smoke curls from", "a tiny cliffside home.", "mountain_hermit"},
  {"RESCUE POST", "A rescue keeper waves", "from a sheltered post.", "rescue_post"},
  {"TEA CLIMBER", "A climber has somehow", "made tea up here.", "tea_climber"}
};

static const RawEventText FROSTPEAK_DISCOVERIES[] = {
  {"ICE ARCH", "A natural arch of ice", "spans a narrow gorge.", "ice_arch"},
  {"OLD OBSERVATORY", "A snow-covered observatory", "faces the stars.", "old_observatory"},
  {"FROZEN FALLS", "A waterfall hangs", "motionless from the cliff.", "frozen_falls"},
  {"CARVED PASS", "Old symbols mark", "the walls of the pass.", "carved_pass"},
  {"SUMMIT BELL", "A bronze bell stands", "alone on a ridge.", "summit_bell"}
};

static const RawEventText FROSTPEAK_REST[] = {
  {"PINE SHELTER", "Dense pines block", "most of the wind.", "pine_shelter"},
  {"MOUNTAIN HUT", "An unlocked mountain hut", "offers a warm break.", "mountain_hut"},
  {"SUNNY LEDGE", "Sunlight warms one", "perfectly placed ledge.", "sunny_ledge"},
  {"HOT VENT", "Warm air rises", "through cracks in the rock.", "hot_vent"}
};

static const RawEventText FROSTPEAK_RARE[] = {
  {"ICE HALO", "A perfect ring of light", "forms around the sun.", "ice_halo"},
  {"SNOW LIGHTS", "Soft lights move", "under the snowfield.", "snow_lights"},
  {"FROZEN ECHO", "An echo answers", "before anyone speaks.", "frozen_echo"},
  {"GLASS SNOW", "A patch of snow", "chimes like tiny bells.", "glass_snow"},
  {"CLOUD STAIRS", "For a moment the clouds", "look exactly like stairs.", "cloud_stairs"}
};


// ==================================================
// VOLCANIC BADLANDS (internal id: VOLCANIC HIGHLANDS)
// ==================================================

static const RawEventText VOLCANIC_HIGHLANDS_TRAVEL[] = {
  {"ASH PATH", "A dark path cuts", "through pale drifting ash.", "ash_path"},
  {"LAVA CHANNEL", "Bright lava crawls", "beside the rocky trail.", "lava_channel"},
  {"BLACK RIDGE", "The route follows", "a knife-edge black ridge.", "black_ridge"},
  {"BASALT STEPS", "Natural basalt steps climb", "toward the smoking peak.", "basalt_steps"},
  {"CINDER FIELD", "Loose cinders crunch", "under every step.", "cinder_field"},
  {"STEAM VENTS", "White steam bursts", "from cracks ahead.", "steam_vents"},
  {"OBSIDIAN TRACK", "A glassy black track", "reflects the red sky.", "obsidian_track"},
  {"CALDERA ROAD", "An old road curves", "toward the caldera rim.", "caldera_road"}
};

static const RawEventText VOLCANIC_HIGHLANDS_BATTLES[] = {
  {"CINDER LIZARD", "A plated lizard crawls", "out of warm rubble.", "cinder_lizard"},
  {"ASH HOUND", "An ash-grey hound", "bounds across the ridge.", "ash_hound"},
  {"LAVA CRAB", "A stone-shelled crab", "climbs from a hot vent.", "lava_crab"},
  {"EMBER BAT", "A huge ember-winged bat", "dives through the smoke.", "ember_bat"},
  {"BASALT BOAR", "A black-armoured boar", "scrapes at the ground.", "basalt_boar"},
  {"FIRE SERPENT", "A long shape moves", "between glowing cracks.", "fire_serpent"}
};

static const RawEventText VOLCANIC_HIGHLANDS_FRIENDLY[] = {
  {"ASH FOX", "A soot-covered fox", "watches from a warm rock.", "ash_fox"},
  {"CINDER BIRD", "A tiny black bird", "bathes in warm ash.", "cinder_bird"},
  {"ROCK TORTOISE", "A stone-coloured tortoise", "crosses very slowly.", "rock_tortoise"},
  {"EMBER MOTH", "A huge gentle moth", "glows beside a vent.", "ember_moth"},
  {"LAVA NEWT", "A bright little newt", "peeks from hot stones.", "lava_newt"}
};

static const RawEventText VOLCANIC_HIGHLANDS_TREASURE[] = {
  {"OBSIDIAN CACHE", "A carved obsidian box", "rests in a rock hollow.", "obsidian_cache"},
  {"MINER'S TIN", "An old metal tin", "survived beneath the ash.", "miners_tin"},
  {"GLASS POCKET", "Volcanic glass fills", "a shallow stone pocket.", "glass_pocket"},
  {"BASALT VAULT", "A fitted stone lid", "hides a tiny vault.", "basalt_vault"},
  {"COOLED FLOW", "Something metallic shines", "inside cooled lava.", "cooled_flow"}
};

static const RawEventText VOLCANIC_HIGHLANDS_NPCS[] = {
  {"ASH MINER", "A miner works carefully", "beside a glassy seam.", "ash_miner"},
  {"VENT KEEPER", "Someone tends gauges", "around a steam vent.", "vent_keeper"},
  {"RIDGE TRADER", "A stubborn trader has", "set up on the ridge.", "ridge_trader"},
  {"HOT SPRING COOK", "A cook is using", "the volcano as a stove.", "hot_spring_cook"}
};

static const RawEventText VOLCANIC_HIGHLANDS_DISCOVERIES[] = {
  {"LAVA FALL", "A bright lava fall", "drops into darkness.", "lava_fall"},
  {"OBSIDIAN CAVE", "A cave wall shines", "like black glass.", "obsidian_cave"},
  {"OLD FORGE", "A ruined forge stands", "beside natural heat vents.", "old_forge"},
  {"BASALT ORGAN", "Tall basalt columns", "rise like organ pipes.", "basalt_organ"},
  {"CALDERA SHRINE", "A small stone shrine", "faces the crater.", "caldera_shrine"}
};

static const RawEventText VOLCANIC_HIGHLANDS_REST[] = {
  {"COOL CAVE", "A deep basalt cave", "is unexpectedly cool.", "cool_cave"},
  {"STEAM POOL", "A warm mineral pool", "offers a safe rest.", "steam_pool"},
  {"STONE SHELTER", "A heavy rock shelf", "blocks ash and wind.", "stone_shelter"},
  {"OLD CAMP", "An abandoned camp still", "has intact benches.", "old_camp"}
};

static const RawEventText VOLCANIC_HIGHLANDS_RARE[] = {
  {"BLUE FLAME", "A blue flame burns", "on bare black stone.", "blue_flame"},
  {"FLOATING ASH", "A patch of ash", "hangs motionless in midair.", "floating_ash"},
  {"GLASS RAIN", "Tiny black glass beads", "fall from a clear sky.", "glass_rain"},
  {"HEARTBEAT", "The ground pulses once", "like a giant heartbeat.", "heartbeat"},
  {"RED LIGHTS", "Small red lights move", "inside the distant smoke.", "red_lights"}
};


// ==================================================
// SKYREACH CITY (internal id: SKYREACH)
// ==================================================

static const RawEventText SKYREACH_TRAVEL[] = {
  {"CLOUD BRIDGE", "A narrow bridge crosses", "open cloud-filled air.", "cloud_bridge"},
  {"CLIFF ROAD", "The road clings", "to an immense cliff face.", "cliff_road"},
  {"FLOATING ROCKS", "Small floating rocks drift", "beside the path.", "floating_rocks"},
  {"WATERFALL PATH", "A path passes behind", "a waterfall into clouds.", "waterfall_path"},
  {"HIGH ARCH", "A stone arch frames", "nothing but blue sky.", "high_arch"},
  {"ROPE SPAN", "A long rope span", "sways between two pillars.", "rope_span"},
  {"CLOUD STAIRS", "Stone stairs rise", "through a bank of cloud.", "sky_cloud_stairs"},
  {"AERIE ROAD", "The old road climbs", "toward a distant aerie.", "aerie_road"}
};

static const RawEventText SKYREACH_BATTLES[] = {
  {"CLIFF TALON", "A hooked-wing predator", "swoops across the path.", "cliff_talon"},
  {"CLOUD SERPENT", "A pale serpent coils", "through the cloud bank.", "cloud_serpent"},
  {"STONE WING", "A rock-skinned flyer", "drops from above.", "stone_wing"},
  {"SKY RAM", "A horned mountain beast", "guards the bridge.", "sky_ram"},
  {"GALE HUNTER", "Something fast circles", "inside the wind.", "gale_hunter"},
  {"AERIE STALKER", "A long-legged hunter", "steps onto the ledge.", "aerie_stalker"}
};

static const RawEventText SKYREACH_FRIENDLY[] = {
  {"CLOUD FOX", "A pale fox trots", "across a floating rock.", "cloud_fox"},
  {"SKY SWALLOW", "A bright swallow circles", "the team twice.", "sky_swallow"},
  {"CLIFF GOAT", "A tiny cliff goat", "stands somewhere impossible.", "cliff_goat"},
  {"WIND MANTA", "A gentle manta-like creature", "glides through the air.", "wind_manta"},
  {"AERIE CAT", "A fluffy cat appears", "to own this entire mountain.", "aerie_cat"}
};

static const RawEventText SKYREACH_TREASURE[] = {
  {"CLOUD CACHE", "A weatherproof box hangs", "beneath a bridge.", "cloud_cache"},
  {"CLIFF NICHE", "A carved cliff niche", "contains a wrapped bundle.", "cliff_niche"},
  {"FALLEN PACK", "A travel pack rests", "on a floating ledge.", "fallen_pack"},
  {"AERIE CHEST", "A small carved chest", "sits behind old feathers.", "aerie_chest"},
  {"WIND CHIME", "An ornate wind chime", "hides a tiny compartment.", "wind_chime"}
};

static const RawEventText SKYREACH_NPCS[] = {
  {"BRIDGE KEEPER", "A bridge keeper checks", "every rope twice.", "bridge_keeper"},
  {"CLOUD SHEPHERD", "A shepherd watches creatures", "grazing on high ledges.", "cloud_shepherd"},
  {"AERIE TRADER", "A trader's stall occupies", "an impossible perch.", "aerie_trader"},
  {"WIND MONK", "A quiet traveller sits", "facing the open sky.", "wind_monk"}
};

static const RawEventText SKYREACH_DISCOVERIES[] = {
  {"FLOATING ISLAND", "A small island hangs", "unsupported over the clouds.", "floating_island"},
  {"SKY TEMPLE", "An old temple crowns", "a distant stone pillar.", "sky_temple"},
  {"UPWARD FALL", "Water rises briefly", "from one cliff to another.", "upward_fall"},
  {"ANCIENT LIFT", "An old platform hangs", "from enormous chains.", "ancient_lift"},
  {"CLOUD CASTLE", "A distant castle appears", "above the cloud line.", "cloud_castle"}
};

static const RawEventText SKYREACH_REST[] = {
  {"WIND SHELTER", "A stone alcove blocks", "the constant high wind.", "wind_shelter"},
  {"SUNNY PLATFORM", "A broad platform sits", "warm above the clouds.", "sunny_platform"},
  {"AERIE HUT", "An empty aerie hut", "has a very good roof.", "aerie_hut"},
  {"QUIET LEDGE", "For once the air", "becomes completely still.", "quiet_ledge"}
};

static const RawEventText SKYREACH_RARE[] = {
  {"CLOUD WHALE", "An enormous shadow swims", "through the clouds below.", "cloud_whale"},
  {"DOUBLE HORIZON", "For a moment there are", "two separate horizons.", "double_horizon"},
  {"FALLING STAR", "A bright star falls", "upward into daylight.", "falling_star"},
  {"SILENT LIGHTNING", "Lightning flashes nearby", "without making a sound.", "silent_lightning"},
  {"DOOR IN THE SKY", "A distant dark rectangle", "hangs alone in the air.", "door_in_sky"}
};


// ==================================================
// BOSS CONTENT
//
// Statistics stay in AdventureEngine.cpp.
// Names/story belong here.
// ==================================================

static const BossText
WHISPERWOOD_BOSSES[] = {

  {
    "BRAMBLEBACK",
    "Brambleback blocks the path."
  },

  {
    "LANTERN WYRM",
    "A Lantern Wyrm descends."
  },

  {
    "FOREST GUARDIAN",
    "The Guardian bars the road."
  },

  {
    "FOREST COLOSSUS",
    "The Colossus rises."
  }
};


static const BossText
OPEN_PLAINS_BOSSES[] = {

  {
    "THISTLE KING",
    "The Thistle King lowers its horns."
  },

  {
    "GALE ROC",
    "A Gale Roc drops from the sky."
  },

  {
    "STONE HERD",
    "The Stone Herd blocks the plain."
  },

  {
    "PLAINS COLOSSUS",
    "The Colossus rises from the grass."
  }
};


static const BossText
DUSTY_DESERT_BOSSES[] = {

  {
    "SANDMAW",
    "Sandmaw erupts from the dunes."
  },

  {
    "SUN SCORPION",
    "The Sun Scorpion raises its claws."
  },

  {
    "DUNE GUARDIAN",
    "The Guardian steps from the dust."
  },

  {
    "DESERT TITAN",
    "The Desert Titan wakes."
  }
};


static const BossText
COAST_BOSSES[] = {

  {
    "REEFCLAW",
    "Reefclaw leaves the surf."
  },

  {
    "TEMPEST RAY",
    "The Tempest Ray circles."
  },

  {
    "LIGHTHOUSE GUARDIAN",
    "The Guardian blocks the beacon."
  },

  {
    "TIDE TITAN",
    "The Tide Titan rises."
  }
};


static const BossText
MISTY_MARSH_BOSSES[] = {
  {"MOSSJAW", "Mossjaw rises from the reeds."},
  {"FEN WYRM", "The Fen Wyrm coils through the mist."},
  {"BOG GUARDIAN", "The Bog Guardian blocks the causeway."},
  {"MARSH LEVIATHAN", "The Marsh Leviathan breaks the water."}
};


static const BossText
FROSTPEAK_BOSSES[] = {
  {"RIMEHORN", "Rimehorn stamps across the pass."},
  {"SNOW ROC", "The Snow Roc drops from the clouds."},
  {"ICE GUARDIAN", "The Ice Guardian leaves the frozen wall."},
  {"PEAK COLOSSUS", "The Peak Colossus wakes beneath the snow."}
};


static const BossText
VOLCANIC_HIGHLANDS_BOSSES[] = {
  {"CINDERBACK", "Cinderback climbs from the ash."},
  {"ASH DRAKE", "The Ash Drake circles the crater."},
  {"MAGMA GUARDIAN", "The Magma Guardian crosses the lava road."},
  {"CALDERA TITAN", "The Caldera Titan rises through the smoke."}
};


static const BossText
SKYREACH_BOSSES[] = {
  {"CLOUDHORN", "Cloudhorn steps onto the high bridge."},
  {"STORM GRYPHON", "The Storm Gryphon folds its wings."},
  {"AERIE GUARDIAN", "The Aerie Guardian bars the ascent."},
  {"SKY LEVIATHAN", "The Sky Leviathan descends through the clouds."}
};



// ==================================================
// CHARACTER OUTCOME POOLS
// ==================================================

static const char*
BATTLE_SUCCESS_LINES[] = {

  "{name} drives it back.",

  "{name} holds the line.",

  "{name} clears the path.",

  "{name} stands their ground.",

  "{name} sends it fleeing.",

  "{name} refuses to budge.",

  "{name} makes short work of it.",

  "{name} handles the problem."
};


static const char*
BATTLE_RETREAT_LINES[] = {

  "That one was tougher than expected.",

  "The team decides discretion is wise.",

  "Not every fight needs finishing.",

  "They retreat while they still can.",

  "The team makes a tactical exit."
};


static const char*
FRIENDLY_RESULT_LINES[] = {

  "{name} wins its trust.",

  "{name} makes a new friend.",

  "{name} calms it down.",

  "{name} knows exactly what to do.",

  "{name} earns a tiny nod.",

  "{name} is immediately accepted.",

  "{name} somehow understands."
};


static const char*
TREASURE_RESULT_LINES[] = {

  "{name} spots the hiding place.",

  "{name} notices the glint.",

  "{name} finds the cache.",

  "{name} checks exactly the right place.",

  "{name} gets lucky again.",

  "{name} nearly walks straight past it.",

  "{name} finds something worthwhile."
};


static const char*
NPC_RESULT_LINES[] = {

  "{name} gets them talking.",

  "{name} makes a good impression.",

  "{name} quickly wins them over.",

  "{name} knows the right thing to say.",

  "{name} makes another acquaintance.",

  "{name} handles the introductions."
};


static const char*
DISCOVERY_RESULT_LINES[] = {

  "{name} notices something unusual.",

  "{name} spots the clue.",

  "{name} sees what everyone missed.",

  "{name} finds the way forward.",

  "{name} notices a hidden detail.",

  "{name} has a very lucky hunch."
};


static const char*
REST_RESULT_LINES[] = {

  "{name} finds the perfect spot.",

  "{name} declares this good enough.",

  "{name} suggests everyone stops here.",

  "{name} makes the place comfortable.",

  "{name} insists they deserve a break."
};


static const char*
RARE_RESULT_LINES[] = {

  "{name} notices it first.",

  "{name} spots something impossible.",

  "{name} is certain they saw it.",

  "{name} points out something strange.",

  "{name} was looking in exactly the right place.",

  "{name} finds something very unusual."
};


// ==================================================
// PUBLIC EVENT FUNCTIONS
// ==================================================

EventText contentTravel(
  int location,
  const String &actorName
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_TRAVEL,
      sizeof(WHISPERWOOD_TRAVEL) /
      sizeof(WHISPERWOOD_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_TRAVEL,
      sizeof(OPEN_PLAINS_TRAVEL) /
      sizeof(OPEN_PLAINS_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_TRAVEL,
      sizeof(DUSTY_DESERT_TRAVEL) /
      sizeof(DUSTY_DESERT_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_TRAVEL,
      sizeof(MISTY_MARSH_TRAVEL) /
      sizeof(MISTY_MARSH_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_TRAVEL,
      sizeof(FROSTPEAK_TRAVEL) /
      sizeof(FROSTPEAK_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_TRAVEL,
      sizeof(VOLCANIC_HIGHLANDS_TRAVEL) /
      sizeof(VOLCANIC_HIGHLANDS_TRAVEL[0]),
      actorName
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_TRAVEL,
      sizeof(SKYREACH_TRAVEL) /
      sizeof(SKYREACH_TRAVEL[0]),
      actorName
    );
  }


  return pickEvent(
    COAST_TRAVEL,
    sizeof(COAST_TRAVEL) /
    sizeof(COAST_TRAVEL[0]),
    actorName
  );
}


// ==================================================

EventText contentBattle(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_BATTLES,
      sizeof(WHISPERWOOD_BATTLES) /
      sizeof(WHISPERWOOD_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_BATTLES,
      sizeof(OPEN_PLAINS_BATTLES) /
      sizeof(OPEN_PLAINS_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_BATTLES,
      sizeof(DUSTY_DESERT_BATTLES) /
      sizeof(DUSTY_DESERT_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_BATTLES,
      sizeof(MISTY_MARSH_BATTLES) /
      sizeof(MISTY_MARSH_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_BATTLES,
      sizeof(FROSTPEAK_BATTLES) /
      sizeof(FROSTPEAK_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_BATTLES,
      sizeof(VOLCANIC_HIGHLANDS_BATTLES) /
      sizeof(VOLCANIC_HIGHLANDS_BATTLES[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_BATTLES,
      sizeof(SKYREACH_BATTLES) /
      sizeof(SKYREACH_BATTLES[0])
    );
  }


  return pickEvent(
    COAST_BATTLES,
    sizeof(COAST_BATTLES) /
    sizeof(COAST_BATTLES[0])
  );
}


// ==================================================

EventText contentFriendly(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_FRIENDLY,
      sizeof(WHISPERWOOD_FRIENDLY) /
      sizeof(WHISPERWOOD_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_FRIENDLY,
      sizeof(OPEN_PLAINS_FRIENDLY) /
      sizeof(OPEN_PLAINS_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_FRIENDLY,
      sizeof(DUSTY_DESERT_FRIENDLY) /
      sizeof(DUSTY_DESERT_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_FRIENDLY,
      sizeof(MISTY_MARSH_FRIENDLY) /
      sizeof(MISTY_MARSH_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_FRIENDLY,
      sizeof(FROSTPEAK_FRIENDLY) /
      sizeof(FROSTPEAK_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_FRIENDLY,
      sizeof(VOLCANIC_HIGHLANDS_FRIENDLY) /
      sizeof(VOLCANIC_HIGHLANDS_FRIENDLY[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_FRIENDLY,
      sizeof(SKYREACH_FRIENDLY) /
      sizeof(SKYREACH_FRIENDLY[0])
    );
  }


  return pickEvent(
    COAST_FRIENDLY,
    sizeof(COAST_FRIENDLY) /
    sizeof(COAST_FRIENDLY[0])
  );
}


// ==================================================

EventText contentTreasure(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_TREASURE,
      sizeof(WHISPERWOOD_TREASURE) /
      sizeof(WHISPERWOOD_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_TREASURE,
      sizeof(OPEN_PLAINS_TREASURE) /
      sizeof(OPEN_PLAINS_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_TREASURE,
      sizeof(DUSTY_DESERT_TREASURE) /
      sizeof(DUSTY_DESERT_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_TREASURE,
      sizeof(MISTY_MARSH_TREASURE) /
      sizeof(MISTY_MARSH_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_TREASURE,
      sizeof(FROSTPEAK_TREASURE) /
      sizeof(FROSTPEAK_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_TREASURE,
      sizeof(VOLCANIC_HIGHLANDS_TREASURE) /
      sizeof(VOLCANIC_HIGHLANDS_TREASURE[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_TREASURE,
      sizeof(SKYREACH_TREASURE) /
      sizeof(SKYREACH_TREASURE[0])
    );
  }


  return pickEvent(
    COAST_TREASURE,
    sizeof(COAST_TREASURE) /
    sizeof(COAST_TREASURE[0])
  );
}


// ==================================================

EventText contentNPC(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_NPCS,
      sizeof(WHISPERWOOD_NPCS) /
      sizeof(WHISPERWOOD_NPCS[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_NPCS,
      sizeof(OPEN_PLAINS_NPCS) /
      sizeof(OPEN_PLAINS_NPCS[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_NPCS,
      sizeof(DUSTY_DESERT_NPCS) /
      sizeof(DUSTY_DESERT_NPCS[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_NPCS,
      sizeof(MISTY_MARSH_NPCS) /
      sizeof(MISTY_MARSH_NPCS[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_NPCS,
      sizeof(FROSTPEAK_NPCS) /
      sizeof(FROSTPEAK_NPCS[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_NPCS,
      sizeof(VOLCANIC_HIGHLANDS_NPCS) /
      sizeof(VOLCANIC_HIGHLANDS_NPCS[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_NPCS,
      sizeof(SKYREACH_NPCS) /
      sizeof(SKYREACH_NPCS[0])
    );
  }


  return pickEvent(
    COAST_NPCS,
    sizeof(COAST_NPCS) /
    sizeof(COAST_NPCS[0])
  );
}


// ==================================================

EventText contentDiscovery(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_DISCOVERIES,
      sizeof(WHISPERWOOD_DISCOVERIES) /
      sizeof(WHISPERWOOD_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_DISCOVERIES,
      sizeof(OPEN_PLAINS_DISCOVERIES) /
      sizeof(OPEN_PLAINS_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_DISCOVERIES,
      sizeof(DUSTY_DESERT_DISCOVERIES) /
      sizeof(DUSTY_DESERT_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_DISCOVERIES,
      sizeof(MISTY_MARSH_DISCOVERIES) /
      sizeof(MISTY_MARSH_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_DISCOVERIES,
      sizeof(FROSTPEAK_DISCOVERIES) /
      sizeof(FROSTPEAK_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_DISCOVERIES,
      sizeof(VOLCANIC_HIGHLANDS_DISCOVERIES) /
      sizeof(VOLCANIC_HIGHLANDS_DISCOVERIES[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_DISCOVERIES,
      sizeof(SKYREACH_DISCOVERIES) /
      sizeof(SKYREACH_DISCOVERIES[0])
    );
  }


  return pickEvent(
    COAST_DISCOVERIES,
    sizeof(COAST_DISCOVERIES) /
    sizeof(COAST_DISCOVERIES[0])
  );
}


// ==================================================

EventText contentRest(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_REST,
      sizeof(WHISPERWOOD_REST) /
      sizeof(WHISPERWOOD_REST[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_REST,
      sizeof(OPEN_PLAINS_REST) /
      sizeof(OPEN_PLAINS_REST[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_REST,
      sizeof(DUSTY_DESERT_REST) /
      sizeof(DUSTY_DESERT_REST[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_REST,
      sizeof(MISTY_MARSH_REST) /
      sizeof(MISTY_MARSH_REST[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_REST,
      sizeof(FROSTPEAK_REST) /
      sizeof(FROSTPEAK_REST[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_REST,
      sizeof(VOLCANIC_HIGHLANDS_REST) /
      sizeof(VOLCANIC_HIGHLANDS_REST[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_REST,
      sizeof(SKYREACH_REST) /
      sizeof(SKYREACH_REST[0])
    );
  }


  return pickEvent(
    COAST_REST,
    sizeof(COAST_REST) /
    sizeof(COAST_REST[0])
  );
}


// ==================================================

EventText contentRare(
  int location
) {

  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return pickEvent(
      WHISPERWOOD_RARE,
      sizeof(WHISPERWOOD_RARE) /
      sizeof(WHISPERWOOD_RARE[0])
    );
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return pickEvent(
      OPEN_PLAINS_RARE,
      sizeof(OPEN_PLAINS_RARE) /
      sizeof(OPEN_PLAINS_RARE[0])
    );
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return pickEvent(
      DUSTY_DESERT_RARE,
      sizeof(DUSTY_DESERT_RARE) /
      sizeof(DUSTY_DESERT_RARE[0])
    );
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    return pickEvent(
      MISTY_MARSH_RARE,
      sizeof(MISTY_MARSH_RARE) /
      sizeof(MISTY_MARSH_RARE[0])
    );
  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    return pickEvent(
      FROSTPEAK_RARE,
      sizeof(FROSTPEAK_RARE) /
      sizeof(FROSTPEAK_RARE[0])
    );
  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    return pickEvent(
      VOLCANIC_HIGHLANDS_RARE,
      sizeof(VOLCANIC_HIGHLANDS_RARE) /
      sizeof(VOLCANIC_HIGHLANDS_RARE[0])
    );
  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    return pickEvent(
      SKYREACH_RARE,
      sizeof(SKYREACH_RARE) /
      sizeof(SKYREACH_RARE[0])
    );
  }


  return pickEvent(
    COAST_RARE,
    sizeof(COAST_RARE) /
    sizeof(COAST_RARE[0])
  );
}



// ==================================================
// CONTEXTUAL ART KEY LOOKUP
//
// The art key belongs to authored content rather than
// the artwork code. This keeps visible wording free to
// change without forcing filename/title alias changes.
//
// If an event has no art key, ArtSystem simply uses
// the normal event-category/location fallbacks.
// ==================================================

static String findEventArtKey(
  const RawEventText* events,
  int count,
  const String &eventTitle
) {

  for (
    int i = 0;
    i < count;
    i++
  ) {

    if (
      eventTitle ==
      events[i].title
    ) {

      if (
        events[i].artKey ==
        nullptr
      ) {

        return "";
      }


      return String(
        events[i].artKey
      );
    }
  }


  return "";
}


static String simpleArtSlug(
  const String &text
) {

  String result =
    "";


  bool lastWasUnderscore =
    false;


  for (
    int i = 0;
    i < text.length();
    i++
  ) {

    char c =
      text.charAt(i);


    if (
      c >= 'A' &&
      c <= 'Z'
    ) {

      c =
        c - 'A' + 'a';
    }


    bool valid =
      (
        c >= 'a' &&
        c <= 'z'
      ) ||
      (
        c >= '0' &&
        c <= '9'
      );


    if (
      valid
    ) {

      result +=
        c;


      lastWasUnderscore =
        false;
    }


    else if (
      result.length() > 0 &&
      !lastWasUnderscore
    ) {

      result +=
        '_';


      lastWasUnderscore =
        true;
    }
  }


  while (
    result.endsWith(
      "_"
    )
  ) {

    result.remove(
      result.length() - 1
    );
  }


  return result;
}


String contentArtKeyFor(
  int location,
  const String &eventTitle,
  int eventType
) {

  // Boss artwork uses the milestone filenames already
  // present in the SD artwork pack:
  //
  // <location>_boss_25.jpg
  // <location>_boss_50.jpg
  // <location>_boss_75.jpg
  // <location>_boss_100.jpg
  //
  // Mapping by authored boss name means the correct
  // milestone image is selected deterministically.

  if (
    eventType ==
    BOSS_EVENT
  ) {

    const BossText* bosses =
      nullptr;


    if (
      location ==
      LOC_WHISPERWOOD
    ) {

      bosses =
        WHISPERWOOD_BOSSES;
    }


    else if (
      location ==
      LOC_OPEN_PLAINS
    ) {

      bosses =
        OPEN_PLAINS_BOSSES;
    }


    else if (
      location ==
      LOC_DUSTY_DESERT
    ) {

      bosses =
        DUSTY_DESERT_BOSSES;
    }


    else if (
      location ==
      LOC_STORM_COAST
    ) {

      bosses =
        COAST_BOSSES;
    }


    else if (location == LOC_MISTY_MARSH) {
      bosses = MISTY_MARSH_BOSSES;
    }

    else if (location == LOC_FROSTPEAK) {
      bosses = FROSTPEAK_BOSSES;
    }

    else if (location == LOC_VOLCANIC_HIGHLANDS) {
      bosses = VOLCANIC_HIGHLANDS_BOSSES;
    }

    else if (location == LOC_SKYREACH) {
      bosses = SKYREACH_BOSSES;
    }


    if (
      bosses !=
      nullptr
    ) {

      const char* milestoneKeys[4] = {
        "25",
        "50",
        "75",
        "100"
      };


      for (
        int i = 0;
        i < 4;
        i++
      ) {

        if (
          eventTitle ==
          bosses[i].name
        ) {

          return String(
            milestoneKeys[i]
          );
        }
      }
    }


    return simpleArtSlug(
      eventTitle
    );
  }


  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(WHISPERWOOD_TRAVEL, sizeof(WHISPERWOOD_TRAVEL) / sizeof(WHISPERWOOD_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(WHISPERWOOD_BATTLES, sizeof(WHISPERWOOD_BATTLES) / sizeof(WHISPERWOOD_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(WHISPERWOOD_FRIENDLY, sizeof(WHISPERWOOD_FRIENDLY) / sizeof(WHISPERWOOD_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(WHISPERWOOD_TREASURE, sizeof(WHISPERWOOD_TREASURE) / sizeof(WHISPERWOOD_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(WHISPERWOOD_NPCS, sizeof(WHISPERWOOD_NPCS) / sizeof(WHISPERWOOD_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(WHISPERWOOD_DISCOVERIES, sizeof(WHISPERWOOD_DISCOVERIES) / sizeof(WHISPERWOOD_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(WHISPERWOOD_REST, sizeof(WHISPERWOOD_REST) / sizeof(WHISPERWOOD_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(WHISPERWOOD_RARE, sizeof(WHISPERWOOD_RARE) / sizeof(WHISPERWOOD_RARE[0]), eventTitle);
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(OPEN_PLAINS_TRAVEL, sizeof(OPEN_PLAINS_TRAVEL) / sizeof(OPEN_PLAINS_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(OPEN_PLAINS_BATTLES, sizeof(OPEN_PLAINS_BATTLES) / sizeof(OPEN_PLAINS_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(OPEN_PLAINS_FRIENDLY, sizeof(OPEN_PLAINS_FRIENDLY) / sizeof(OPEN_PLAINS_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(OPEN_PLAINS_TREASURE, sizeof(OPEN_PLAINS_TREASURE) / sizeof(OPEN_PLAINS_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(OPEN_PLAINS_NPCS, sizeof(OPEN_PLAINS_NPCS) / sizeof(OPEN_PLAINS_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(OPEN_PLAINS_DISCOVERIES, sizeof(OPEN_PLAINS_DISCOVERIES) / sizeof(OPEN_PLAINS_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(OPEN_PLAINS_REST, sizeof(OPEN_PLAINS_REST) / sizeof(OPEN_PLAINS_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(OPEN_PLAINS_RARE, sizeof(OPEN_PLAINS_RARE) / sizeof(OPEN_PLAINS_RARE[0]), eventTitle);
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(DUSTY_DESERT_TRAVEL, sizeof(DUSTY_DESERT_TRAVEL) / sizeof(DUSTY_DESERT_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(DUSTY_DESERT_BATTLES, sizeof(DUSTY_DESERT_BATTLES) / sizeof(DUSTY_DESERT_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(DUSTY_DESERT_FRIENDLY, sizeof(DUSTY_DESERT_FRIENDLY) / sizeof(DUSTY_DESERT_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(DUSTY_DESERT_TREASURE, sizeof(DUSTY_DESERT_TREASURE) / sizeof(DUSTY_DESERT_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(DUSTY_DESERT_NPCS, sizeof(DUSTY_DESERT_NPCS) / sizeof(DUSTY_DESERT_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(DUSTY_DESERT_DISCOVERIES, sizeof(DUSTY_DESERT_DISCOVERIES) / sizeof(DUSTY_DESERT_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(DUSTY_DESERT_REST, sizeof(DUSTY_DESERT_REST) / sizeof(DUSTY_DESERT_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(DUSTY_DESERT_RARE, sizeof(DUSTY_DESERT_RARE) / sizeof(DUSTY_DESERT_RARE[0]), eventTitle);
  }


  if (
    location ==
    LOC_STORM_COAST
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(COAST_TRAVEL, sizeof(COAST_TRAVEL) / sizeof(COAST_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(COAST_BATTLES, sizeof(COAST_BATTLES) / sizeof(COAST_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(COAST_FRIENDLY, sizeof(COAST_FRIENDLY) / sizeof(COAST_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(COAST_TREASURE, sizeof(COAST_TREASURE) / sizeof(COAST_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(COAST_NPCS, sizeof(COAST_NPCS) / sizeof(COAST_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(COAST_DISCOVERIES, sizeof(COAST_DISCOVERIES) / sizeof(COAST_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(COAST_REST, sizeof(COAST_REST) / sizeof(COAST_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(COAST_RARE, sizeof(COAST_RARE) / sizeof(COAST_RARE[0]), eventTitle);
  }


  if (
    location ==
    LOC_MISTY_MARSH
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(MISTY_MARSH_TRAVEL, sizeof(MISTY_MARSH_TRAVEL) / sizeof(MISTY_MARSH_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(MISTY_MARSH_BATTLES, sizeof(MISTY_MARSH_BATTLES) / sizeof(MISTY_MARSH_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(MISTY_MARSH_FRIENDLY, sizeof(MISTY_MARSH_FRIENDLY) / sizeof(MISTY_MARSH_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(MISTY_MARSH_TREASURE, sizeof(MISTY_MARSH_TREASURE) / sizeof(MISTY_MARSH_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(MISTY_MARSH_NPCS, sizeof(MISTY_MARSH_NPCS) / sizeof(MISTY_MARSH_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(MISTY_MARSH_DISCOVERIES, sizeof(MISTY_MARSH_DISCOVERIES) / sizeof(MISTY_MARSH_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(MISTY_MARSH_REST, sizeof(MISTY_MARSH_REST) / sizeof(MISTY_MARSH_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(MISTY_MARSH_RARE, sizeof(MISTY_MARSH_RARE) / sizeof(MISTY_MARSH_RARE[0]), eventTitle);

  }


  if (
    location ==
    LOC_FROSTPEAK
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(FROSTPEAK_TRAVEL, sizeof(FROSTPEAK_TRAVEL) / sizeof(FROSTPEAK_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(FROSTPEAK_BATTLES, sizeof(FROSTPEAK_BATTLES) / sizeof(FROSTPEAK_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(FROSTPEAK_FRIENDLY, sizeof(FROSTPEAK_FRIENDLY) / sizeof(FROSTPEAK_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(FROSTPEAK_TREASURE, sizeof(FROSTPEAK_TREASURE) / sizeof(FROSTPEAK_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(FROSTPEAK_NPCS, sizeof(FROSTPEAK_NPCS) / sizeof(FROSTPEAK_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(FROSTPEAK_DISCOVERIES, sizeof(FROSTPEAK_DISCOVERIES) / sizeof(FROSTPEAK_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(FROSTPEAK_REST, sizeof(FROSTPEAK_REST) / sizeof(FROSTPEAK_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(FROSTPEAK_RARE, sizeof(FROSTPEAK_RARE) / sizeof(FROSTPEAK_RARE[0]), eventTitle);

  }


  if (
    location ==
    LOC_VOLCANIC_HIGHLANDS
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(VOLCANIC_HIGHLANDS_TRAVEL, sizeof(VOLCANIC_HIGHLANDS_TRAVEL) / sizeof(VOLCANIC_HIGHLANDS_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(VOLCANIC_HIGHLANDS_BATTLES, sizeof(VOLCANIC_HIGHLANDS_BATTLES) / sizeof(VOLCANIC_HIGHLANDS_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(VOLCANIC_HIGHLANDS_FRIENDLY, sizeof(VOLCANIC_HIGHLANDS_FRIENDLY) / sizeof(VOLCANIC_HIGHLANDS_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(VOLCANIC_HIGHLANDS_TREASURE, sizeof(VOLCANIC_HIGHLANDS_TREASURE) / sizeof(VOLCANIC_HIGHLANDS_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(VOLCANIC_HIGHLANDS_NPCS, sizeof(VOLCANIC_HIGHLANDS_NPCS) / sizeof(VOLCANIC_HIGHLANDS_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(VOLCANIC_HIGHLANDS_DISCOVERIES, sizeof(VOLCANIC_HIGHLANDS_DISCOVERIES) / sizeof(VOLCANIC_HIGHLANDS_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(VOLCANIC_HIGHLANDS_REST, sizeof(VOLCANIC_HIGHLANDS_REST) / sizeof(VOLCANIC_HIGHLANDS_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(VOLCANIC_HIGHLANDS_RARE, sizeof(VOLCANIC_HIGHLANDS_RARE) / sizeof(VOLCANIC_HIGHLANDS_RARE[0]), eventTitle);

  }


  if (
    location ==
    LOC_SKYREACH
  ) {

    if (eventType == TRAVEL_EVENT)
      return findEventArtKey(SKYREACH_TRAVEL, sizeof(SKYREACH_TRAVEL) / sizeof(SKYREACH_TRAVEL[0]), eventTitle);

    if (eventType == BATTLE)
      return findEventArtKey(SKYREACH_BATTLES, sizeof(SKYREACH_BATTLES) / sizeof(SKYREACH_BATTLES[0]), eventTitle);

    if (eventType == FRIENDLY_ENCOUNTER)
      return findEventArtKey(SKYREACH_FRIENDLY, sizeof(SKYREACH_FRIENDLY) / sizeof(SKYREACH_FRIENDLY[0]), eventTitle);

    if (eventType == TREASURE)
      return findEventArtKey(SKYREACH_TREASURE, sizeof(SKYREACH_TREASURE) / sizeof(SKYREACH_TREASURE[0]), eventTitle);

    if (eventType == POTION_MAKER)
      return findEventArtKey(SKYREACH_NPCS, sizeof(SKYREACH_NPCS) / sizeof(SKYREACH_NPCS[0]), eventTitle);

    if (eventType == DISCOVERY)
      return findEventArtKey(SKYREACH_DISCOVERIES, sizeof(SKYREACH_DISCOVERIES) / sizeof(SKYREACH_DISCOVERIES[0]), eventTitle);

    if (eventType == REST_EVENT)
      return findEventArtKey(SKYREACH_REST, sizeof(SKYREACH_REST) / sizeof(SKYREACH_REST[0]), eventTitle);

    if (eventType == RARE_ENCOUNTER)
      return findEventArtKey(SKYREACH_RARE, sizeof(SKYREACH_RARE) / sizeof(SKYREACH_RARE[0]), eventTitle);

  }


  return "";
}


// ==================================================
// BOSS
// ==================================================

BossText contentBoss(
  int location,
  int bossIndex
) {

  if (
    bossIndex < 0
  ) {

    return {
      "BOSS",
      "Something enormous appears."
    };
  }


  if (
    bossIndex > 3
  ) {

    bossIndex =
      3;
  }


  if (
    location ==
    LOC_WHISPERWOOD
  ) {

    return WHISPERWOOD_BOSSES[
      bossIndex
    ];
  }


  if (
    location ==
    LOC_OPEN_PLAINS
  ) {

    return OPEN_PLAINS_BOSSES[
      bossIndex
    ];
  }


  if (
    location ==
    LOC_DUSTY_DESERT
  ) {

    return DUSTY_DESERT_BOSSES[
      bossIndex
    ];
  }


  if (location == LOC_STORM_COAST) {
    return COAST_BOSSES[bossIndex];
  }

  if (location == LOC_MISTY_MARSH) {
    return MISTY_MARSH_BOSSES[bossIndex];
  }

  if (location == LOC_FROSTPEAK) {
    return FROSTPEAK_BOSSES[bossIndex];
  }

  if (location == LOC_VOLCANIC_HIGHLANDS) {
    return VOLCANIC_HIGHLANDS_BOSSES[bossIndex];
  }

  if (location == LOC_SKYREACH) {
    return SKYREACH_BOSSES[bossIndex];
  }

  return {
    "BOSS",
    "Something enormous appears."
  };
}


// ==================================================
// CONTEXTUAL RESULTS
// ==================================================

String contentBattleVictory(
  const String &name
) {

  return pickNamedLine(
    BATTLE_SUCCESS_LINES,
    sizeof(BATTLE_SUCCESS_LINES) /
    sizeof(BATTLE_SUCCESS_LINES[0]),
    name
  );
}


// ==================================================

String contentBattleRetreat() {

  return pickLine(
    BATTLE_RETREAT_LINES,
    sizeof(BATTLE_RETREAT_LINES) /
    sizeof(BATTLE_RETREAT_LINES[0])
  );

}


// ==================================================

String contentFriendlyResult(
  const String &name
) {

  return pickNamedLine(
    FRIENDLY_RESULT_LINES,
    sizeof(FRIENDLY_RESULT_LINES) /
    sizeof(FRIENDLY_RESULT_LINES[0]),
    name
  );
}


// ==================================================

String contentTreasureResult(
  const String &name
) {

  return pickNamedLine(
    TREASURE_RESULT_LINES,
    sizeof(TREASURE_RESULT_LINES) /
    sizeof(TREASURE_RESULT_LINES[0]),
    name
  );
}


// ==================================================

String contentNPCResult(
  const String &name
) {

  return pickNamedLine(
    NPC_RESULT_LINES,
    sizeof(NPC_RESULT_LINES) /
    sizeof(NPC_RESULT_LINES[0]),
    name
  );
}


// ==================================================

String contentDiscoveryResult(
  const String &name
) {

  return pickNamedLine(
    DISCOVERY_RESULT_LINES,
    sizeof(DISCOVERY_RESULT_LINES) /
    sizeof(DISCOVERY_RESULT_LINES[0]),
    name
  );
}


// ==================================================

String contentRestResult(
  const String &name
) {

  return pickNamedLine(
    REST_RESULT_LINES,
    sizeof(REST_RESULT_LINES) /
    sizeof(REST_RESULT_LINES[0]),
    name
  );
}


// ==================================================

String contentRareResult(
  const String &name
) {

  return pickNamedLine(
    RARE_RESULT_LINES,
    sizeof(RARE_RESULT_LINES) /
    sizeof(RARE_RESULT_LINES[0]),
    name
  );
}