#include <math.h>
#include <stdio.h>

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>

#include <vita2d.h>

#define LUT_SIZE (357)
#define COLOR_WHITE (RGBA8(255, 255, 255, 255))

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define LUT_FILE_1 ("ur0:/tai/vitabright_lut.txt")
#define LUT_FILE_2 ("ux0:/tai/vitabright_lut.txt")
static const int DECREMENT_FAST_THRESHOLD = 300 * 1000;

int vitabrightReload();
int vitabrightOledGetLevel();
int vitabrightOledSetLevel(unsigned int level);
int vitabrightOledGetLut(unsigned char oledLut[LUT_SIZE]);
int vitabrightOledSetLut(unsigned char oledLut[LUT_SIZE]);

int currentLevel = 0;
int screenLevel = 0;

int curEditGroup = 0;

unsigned char oledLut[LUT_SIZE] = {0};
uint32_t lastGroupStartEditTime = 0;

void changeLevel(int isStart, int increment) {
  if (!isStart && (sceKernelGetProcessTimeLow() - lastGroupStartEditTime) < DECREMENT_FAST_THRESHOLD)
    return;

  if (isStart) {
    lastGroupStartEditTime = sceKernelGetProcessTimeLow();
  }

  oledLut[currentLevel * 21 + curEditGroup] = MAX(MIN(((int)oledLut[currentLevel * 21 + curEditGroup]) + increment, 255), 0);
  vitabrightOledSetLut(oledLut);
}

void writeLut() {
  char *lutFilePath = LUT_FILE_1;

  FILE *readLut = fopen(lutFilePath, "r");
  if (!readLut) {
    lutFilePath = LUT_FILE_2;
  } else {
    fclose(readLut);
  }

  FILE *lutFile = fopen(lutFilePath, "w");

  char newLut[2000] = {0};
  unsigned int position = 0;

  for (int i = 0; i < 17; ++i) {
    for (int j = 0; j < 20; ++j) {
      sprintf(&(newLut[position]), "%02X ", oledLut[i * 21 + j]);
      position += 3;
    }

    sprintf(&(newLut[position]), "%02X\n", oledLut[i * 21 + 20]);
    position += 3;
  }

  fputs(newLut, lutFile);
  fclose(lutFile);
}

int main() {
  vita2d_pgf *pgf;
  vita2d_pvf *pvf;

  vita2d_init();
  vita2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));

  pgf = vita2d_load_default_pgf();

  int ret = vitabrightOledGetLut(oledLut);

  // Wrong vitabright version installed.
  if (ret != 0) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
      vita2d_start_drawing();
      vita2d_clear_screen();
      vita2d_pgf_draw_text(pgf,
                           5,
                           30,
                           COLOR_WHITE,
                           1.15f,
                           "Cannot communicate with vitabright. Make sure that vitabright v1.1 or "
                           "later is installed.");

      vita2d_end_drawing();
      vita2d_swap_buffers();
    }
#pragma clang diagnostic pop
  }

  /*
   * Load the statically compiled image.png file.
   */

  vita2d_texture *textures[] = {vita2d_load_PNG_file("app0:/resources/colorbars.png"),
                                vita2d_load_PNG_file("app0:/resources/matrix.png"),
                                vita2d_load_JPEG_file("app0:/resources/tv.jpg")};

  unsigned int textureIndex = 0;

  SceCtrlData pad = {0};
  SceCtrlData oldPad = {0};

  currentLevel = vitabrightOledGetLevel();
  screenLevel = vitabrightOledGetLevel();
  uint32_t lastGetLevelTime = sceKernelGetProcessTimeLow();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
  while (1) {
    sceCtrlPeekBufferPositive(0, &pad, 1);

    if ((pad.buttons & SCE_CTRL_CIRCLE) && !(oldPad.buttons & SCE_CTRL_CIRCLE)) {
      vitabrightReload();
      vitabrightOledGetLut(oledLut);
    }

    if ((pad.buttons & SCE_CTRL_CROSS) && !(oldPad.buttons & SCE_CTRL_CROSS))
      textureIndex = (textureIndex + 1) % (sizeof(textures) / sizeof(*textures));

    if ((pad.buttons & SCE_CTRL_SQUARE) && !(oldPad.buttons & SCE_CTRL_SQUARE)) {
      currentLevel = MIN(currentLevel + 1, 16);
    }

    if ((pad.buttons & SCE_CTRL_TRIANGLE) && !(oldPad.buttons & SCE_CTRL_TRIANGLE)) {
      currentLevel = MAX(currentLevel - 1, 0);
    }

    if ((pad.buttons & SCE_CTRL_LEFT) && !(oldPad.buttons & SCE_CTRL_LEFT)) {
      curEditGroup = MAX(curEditGroup - 1, 0);
    }

    if ((pad.buttons & SCE_CTRL_RIGHT) && !(oldPad.buttons & SCE_CTRL_RIGHT)) {
      curEditGroup = MIN(curEditGroup + 1, 20);
    }

    if ((pad.buttons & SCE_CTRL_UP)) {
      int isStart = 0;
      if (!(oldPad.buttons & SCE_CTRL_UP)) {
        isStart = 1;
      }

      changeLevel(isStart, 1);
    }

    if ((pad.buttons & SCE_CTRL_DOWN)) {
      int isStart = 0;
      if (!(oldPad.buttons & SCE_CTRL_DOWN)) {
        isStart = 1;
      }

      changeLevel(isStart, -1);
    }

    if ((pad.buttons & SCE_CTRL_SELECT) && !(oldPad.buttons & SCE_CTRL_SELECT)) {
      vitabrightOledSetLevel(currentLevel);
      screenLevel = vitabrightOledGetLevel();
    }

    if ((pad.buttons & SCE_CTRL_START) && !(oldPad.buttons & SCE_CTRL_START)) {
      writeLut();
    }

    vita2d_start_drawing();
    vita2d_clear_screen();

    vita2d_draw_texture(textures[textureIndex], 0, 0);

    int x = 120;

    char editing[30] = {0};
    sprintf(editing, "Editing: %u", currentLevel);
    vita2d_pgf_draw_text(pgf, 0, 500, COLOR_WHITE, 1.0f, editing);

    for (int i = 0; i < 21; ++i) {
      char value[3] = {0};
      sprintf(value, "%02X", (char)oledLut[currentLevel * 21 + i]);

      vita2d_pgf_draw_text(pgf, x, 500, COLOR_WHITE, 1.0f, value);

      int width = 0;
      int height = 0;
      vita2d_pgf_text_dimensions(pgf, 1.0f, value, &width, &height);

      if (i == curEditGroup) {
        vita2d_draw_rectangle(x, 500 + height / 2, width, 5, COLOR_WHITE);
      }

      x += width + 10;
    }

    char active[30] = {0};
    sprintf(active, "Screen brightness level: %u", screenLevel);
    vita2d_pgf_draw_text(pgf, 0, 540, COLOR_WHITE, 1.0f, active);

    vita2d_end_drawing();
    vita2d_swap_buffers();

    if (sceKernelGetProcessTimeLow() - lastGetLevelTime > 500 * 1000) {
      screenLevel = vitabrightOledGetLevel();
    }

    oldPad = pad;
  }
#pragma clang diagnostic pop

  vita2d_fini();

  for (int i = 0; i < sizeof(textures) / sizeof(*textures); ++i) {
    vita2d_free_texture(textures[i]);
  }
  vita2d_free_pgf(pgf);
  vita2d_free_pvf(pvf);

  sceKernelExitProcess(0);
  return 0;
}