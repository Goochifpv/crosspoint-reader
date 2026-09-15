#include "PocketReaderTestActivity.h"

#include <algorithm>

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>

#include "activities/ActivityManager.h"
#include "fontIds.h"

namespace {

constexpr unsigned long SCROLL_DELAY_MS = 0;  // v0.4: single-pass scene + scroll

struct Scene {
  const char* imagePath;
  const char* location;
  const char* line1;
  const char* line2;
  const char* result;

  // Per-scene artwork positioning in logical X4 pixels.
  // Negative Y moves the illustration upward behind the parchment.
  int artOffsetX;
  int artOffsetY;

  // true = parchment at top of screen, false = parchment at bottom.
  bool overlayTop;
};

constexpr Scene SCENES[] = {
    {
        "/PocketReaderTest/whispering_woods.bmp",
        "Whispering Woods",
        "The path narrows beneath trees older than memory.",
        "Sunlight breaks through the canopy beyond the stone bridge.",
        "Travelled East  |  Strength +1  Heart +1  Luck +1",
        0,
        0,
        false,
    },
    {
        "/PocketReaderTest/misty_marshes.bmp",
        "Misty Marshes",
        "The boardwalk disappears into pale morning fog.",
        "A soft splash comes from the reeds. For a moment, even the marsh seems to hold its breath.",
        "Discovery  |  Luck +3",
        0,
        0,
        true,
    },
};

constexpr int SCENE_COUNT = sizeof(SCENES) / sizeof(SCENES[0]);

bool isNextInput(MappedInputManager& input) {
  using Button = MappedInputManager::Button;
  return input.wasReleased(Button::Confirm) || input.wasReleased(Button::Right) ||
         input.wasReleased(Button::Down) || input.wasReleased(Button::PageForward) ||
         input.wasReleased(Button::NavNext);
}

bool isPreviousInput(MappedInputManager& input) {
  using Button = MappedInputManager::Button;
  return input.wasReleased(Button::Left) || input.wasReleased(Button::Up) ||
         input.wasReleased(Button::PageBack) || input.wasReleased(Button::NavPrevious);
}

}  // namespace

PocketReaderTestActivity::PocketReaderTestActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("PocketReaderTest", renderer, mappedInput) {}

void PocketReaderTestActivity::onEnter() {
  Activity::onEnter();

  // CrossPoint's logical X4 portrait canvas is 480x800.
  renderer.setOrientation(GfxRenderer::Portrait);

  sceneIndex = 0;
  scrollVisible = false;
  showScene();
}

void PocketReaderTestActivity::onExit() {
  Activity::onExit();
}

void PocketReaderTestActivity::loop() {
  Activity::loop();

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    activityManager.goHome();
    return;
  }

  if (isNextInput(mappedInput)) {
    sceneIndex = (sceneIndex + 1) % SCENE_COUNT;
    showScene();
    return;
  }

  if (isPreviousInput(mappedInput)) {
    sceneIndex--;
    if (sceneIndex < 0) sceneIndex = SCENE_COUNT - 1;
    showScene();
    return;
  }
}

void PocketReaderTestActivity::showScene() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  const Scene& scene = SCENES[sceneIndex];

  // v0.4: one physical grayscale waveform per event.
  // The parchment and text are composed into the same 4-level frame as the art.
  // This removes the second expensive grayscale refresh entirely.
  if (!drawSceneBitmap(scene.imagePath, scene.artOffsetX, scene.artOffsetY, true)) {
    drawMissingAssetMessage(scene.imagePath);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    scrollVisible = false;
    return;
  }

  scrollVisible = true;
  sceneShownAt = millis();
}

void PocketReaderTestActivity::revealScroll() {
  // Kept as a fallback helper for later experiments.
  // v0.4 normally never calls this because the scroll is part of showScene().
  if (scrollVisible) return;

  RenderLock lock(*this);

  const Scene& scene = SCENES[sceneIndex];
  if (!drawSceneBitmap(scene.imagePath, scene.artOffsetX, scene.artOffsetY, true)) {
    renderer.setRenderMode(GfxRenderer::BW);
    renderer.clearScreen();
    drawMissingAssetMessage(scene.imagePath);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }

  scrollVisible = true;
}

bool PocketReaderTestActivity::drawSceneBitmap(const char* path, const int artOffsetX, const int artOffsetY, const bool withScroll) {
  HalFile file;

  if (!Storage.openFileForRead("PRG", path, file)) {
    return false;
  }

  const bool absoluteGray =
      renderer.grayscaleCapabilities(HalDisplay::GrayscaleMode::Absolute).supported() &&
      display.getController() == HalDisplay::Controller::SSD1677;

  Bitmap bitmap(file, true, absoluteGray);

  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    file.close();
    return false;
  }

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  // A pure B/W asset can still use CrossPoint's ordinary path.
  if (!bitmap.hasGreyscale()) {
    renderer.setRenderMode(GfxRenderer::BW);

    if (!renderer.drawBitmap(bitmap, artOffsetX, artOffsetY, pageWidth, pageHeight, 0, 0)) {
      file.close();
      return false;
    }

    if (withScroll) drawScrollOverlay();

    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    file.close();
    return true;
  }

  // Use CrossPoint/FreeInk's absolute four-level grayscale mode on the X4.
  // Crucially, we do NOT reconstruct and push a 1-bit baseline afterward.
  // The next staged frame is another absolute grayscale frame instead.
  const bool absolute =
      renderer.grayscaleCapabilities(HalDisplay::GrayscaleMode::Absolute).supported();

  if (absolute) {
    if (!renderer.displayGrayscaleBase(HalDisplay::GrayscaleMode::Absolute)) {
      file.close();
      return false;
    }
  } else {
    renderer.displayGrayscaleBase(HalDisplay::HALF_REFRESH);
  }

  bool planesReady = true;

  for (const auto mode : {GfxRenderer::GRAYSCALE_LSB, GfxRenderer::GRAYSCALE_MSB}) {
    if (bitmap.rewindToData() != BmpReaderError::Ok) {
      planesReady = false;
      break;
    }

    renderer.clearScreen(absolute ? 0xFF : 0x00);
    renderer.setRenderMode(mode);

    if (!renderer.drawBitmap(bitmap, artOffsetX, artOffsetY, pageWidth, pageHeight, 0, 0)) {
      planesReady = false;
      break;
    }

    // Draw the same white parchment + black text into BOTH planes.
    // Pure white/black therefore stays exact; the small gray roll accents
    // are deliberately dithered for this prototype.
    if (withScroll) {
      drawScrollOverlay();
    }

    if (mode == GfxRenderer::GRAYSCALE_LSB) {
      renderer.copyGrayscaleLsbBuffers();
    } else {
      renderer.copyGrayscaleMsbBuffers();
    }
  }

  if (planesReady) {
    renderer.displayGrayBuffer();
  }

  // Only change the host drawing mode. Do not perform a B/W display or
  // grayscale cleanup here: doing so changes the controller baseline and is
  // exactly what made the artwork collapse in earlier prototypes.
  renderer.setRenderMode(GfxRenderer::BW);

  file.close();
  return planesReady;
}

void PocketReaderTestActivity::drawScrollOverlay() {
  const Scene& scene = SCENES[sceneIndex];

  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();

  // v0.7b:
  // - full-screen artwork stays behind the UI
  // - parchment height follows actual wrapped line count
  // - stats/reward text lives inside its own bordered sub-panel
  // - bottom-positioned parchment gets extra breathing room above controls
  // - controls remain fixed to the physical bottom edge

  constexpr int CARD_X = 12;
  constexpr int CARD_W = 456;
  constexpr int SIDE_PAD = 22;
  constexpr int CORNER = 12;

  constexpr int FOOTER_X = 12;
  constexpr int FOOTER_Y = 756;
  constexpr int FOOTER_W = 456;
  constexpr int FOOTER_H = 36;

  constexpr int CARD_MIN_H = 160;
  constexpr int CARD_MAX_H = 246;

  // More visual separation for low cards such as Whispering Woods.
  constexpr int CARD_GAP_TO_FOOTER = 16;

  const int textX = CARD_X + SIDE_PAD;
  const int textWidth = CARD_W - (SIDE_PAD * 2);

  const int titleFont = NOTOSERIF_14_FONT_ID;
  const int bodyFont = NOTOSERIF_12_FONT_ID;
  const int resultFont = UI_10_FONT_ID;

  const std::string body = std::string(scene.line1) + " " + scene.line2;

  const auto bodyLines =
      renderer.wrappedText(bodyFont, body.c_str(), textWidth, 5);

  // Reward/stat text gets a little more inner padding, so calculate against
  // a narrower width than the main story text.
  constexpr int RESULT_BOX_SIDE_PAD = 10;
  const int resultTextWidth = textWidth - (RESULT_BOX_SIDE_PAD * 2);

  const auto resultLines =
      renderer.wrappedText(
          resultFont,
          scene.result,
          resultTextWidth,
          2,
          EpdFontFamily::BOLD);

  const int titleH = renderer.getLineHeight(titleFont);
  const int bodyLineH = renderer.getLineHeight(bodyFont);
  const int resultLineH = renderer.getLineHeight(resultFont);

  constexpr int RESULT_BOX_TOP_PAD = 7;
  constexpr int RESULT_BOX_BOTTOM_PAD = 7;

  const int resultBoxH =
      RESULT_BOX_TOP_PAD +
      static_cast<int>(resultLines.size()) * resultLineH +
      RESULT_BOX_BOTTOM_PAD;

  // Grow/shrink from ACTUAL wrapped line count.
  int cardH = 0;
  cardH += 11;  // top padding
  cardH += titleH;
  cardH += 4;   // title -> divider
  cardH += 8;   // divider -> body
  cardH += static_cast<int>(bodyLines.size()) * bodyLineH;
  cardH += 8;   // body -> result box
  cardH += resultBoxH;
  cardH += 13;  // bottom breathing room above the lower roll

  cardH = std::max(CARD_MIN_H, std::min(CARD_MAX_H, cardH));

  // Misty Marshes = top overlay.
  // Whispering Woods = bottom overlay with extra gap above footer.
  const int cardY =
      scene.overlayTop
          ? 18
          : (FOOTER_Y - CARD_GAP_TO_FOOTER - cardH);

  // Main parchment.
  renderer.fillRoundedRect(CARD_X, cardY, CARD_W, cardH, CORNER, Color::White);
  renderer.drawRoundedRect(CARD_X, cardY, CARD_W, cardH, 2, CORNER, true);

  // Restrained roll decoration.
  renderer.fillRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 1, 7, true);
  renderer.fillRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 1, 7, true);

  int cursorY = cardY + 11;

  // Title.
  renderer.drawCenteredText(
      titleFont,
      cursorY,
      scene.location,
      true,
      EpdFontFamily::BOLD);

  cursorY += titleH + 3;

  renderer.drawLine(
      textX,
      cursorY,
      CARD_X + CARD_W - SIDE_PAD,
      cursorY,
      true);

  cursorY += 8;

  // Story body.
  for (const auto& line : bodyLines) {
    renderer.drawText(bodyFont, textX, cursorY, line.c_str(), true);
    cursorY += bodyLineH;
  }

  cursorY += 7;

  // Dedicated bordered result / reward / stats box.
  const int resultBoxX = textX;
  const int resultBoxY = cursorY;
  const int resultBoxW = textWidth;

  renderer.fillRoundedRect(
      resultBoxX,
      resultBoxY,
      resultBoxW,
      resultBoxH,
      7,
      Color::White);

  renderer.drawRoundedRect(
      resultBoxX,
      resultBoxY,
      resultBoxW,
      resultBoxH,
      1,
      7,
      true);

  int resultY = resultBoxY + RESULT_BOX_TOP_PAD;

  for (const auto& line : resultLines) {
    renderer.drawText(
        resultFont,
        resultBoxX + RESULT_BOX_SIDE_PAD,
        resultY,
        line.c_str(),
        true,
        EpdFontFamily::BOLD);
    resultY += resultLineH;
  }

  // Fixed footer, visually independent from parchment.
  renderer.fillRoundedRect(FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, 8, Color::White);
  renderer.drawRoundedRect(FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, 1, 8, true);

  const int footerTextY = FOOTER_Y + 10;
  constexpr int FOOTER_PAD = 18;

  renderer.drawText(
      SMALL_FONT_ID,
      FOOTER_X + FOOTER_PAD,
      footerTextY,
      "BACK",
      true);

  const char* next = "NEXT";
  renderer.drawText(
      SMALL_FONT_ID,
      FOOTER_X + FOOTER_W - FOOTER_PAD -
          renderer.getTextWidth(SMALL_FONT_ID, next),
      footerTextY,
      next,
      true);
}
void PocketReaderTestActivity::drawMissingAssetMessage(const char* path) {
  renderer.clearScreen();

  renderer.drawCenteredText(NOTOSERIF_18_FONT_ID, 160, "Pocket Reader X4 Test", true, EpdFontFamily::BOLD);

  renderer.drawCenteredText(NOTOSERIF_14_FONT_ID, 250, "Artwork file not found.", true);

  const auto pathLines = renderer.wrappedText(UI_10_FONT_ID, path, renderer.getScreenWidth() - 50, 3);
  int y = 310;
  for (const auto& line : pathLines) {
    renderer.drawCenteredText(UI_10_FONT_ID, y, line.c_str(), true);
    y += renderer.getLineHeight(UI_10_FONT_ID) + 4;
  }

  renderer.drawCenteredText(UI_10_FONT_ID, 430, "Copy the PocketReaderTest folder", true);
  renderer.drawCenteredText(UI_10_FONT_ID, 458, "from this package to the SD-card root.", true);
  renderer.drawCenteredText(UI_10_FONT_ID, 560, "BACK returns to normal CrossPoint.", true);
}
