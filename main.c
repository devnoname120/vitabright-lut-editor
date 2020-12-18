#include <math.h>
#include <stdio.h>

#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>

#include <vita2d.h>

int main() {
  vita2d_pgf *pgf;
  vita2d_pvf *pvf;
  vita2d_texture *image;
  float rad = 0.0f;

  vita2d_init();
  vita2d_set_clear_color(RGBA8(0x40, 0x40, 0x40, 0xFF));

  pgf = vita2d_load_default_pgf();
  pvf = vita2d_load_default_pvf();

  /*
   * Load the statically compiled image.png file.
   */

  vita2d_texture *textures[] = {vita2d_load_PNG_file("app0:/resources/colorbars.png"),
                               vita2d_load_PNG_file("app0:/resources/matrix.png"),
                               vita2d_load_JPEG_file("app0:/resources/tv.jpg")};

  unsigned int textureIndex = 0;

  SceCtrlData pad = {0};
  SceCtrlData oldPad = {0};

  while (1) {
    sceCtrlPeekBufferPositive(0, &pad, 1);

    if (pad.buttons & SCE_CTRL_START)
      break;

    if ((pad.buttons & SCE_CTRL_CROSS) && !(oldPad.buttons & SCE_CTRL_CROSS))
      textureIndex = (textureIndex + 1) % (sizeof(textures)/sizeof(*textures));

    vita2d_start_drawing();
    vita2d_clear_screen();

    vita2d_draw_texture(textures[textureIndex], 0, 0);
    vita2d_pgf_draw_text(pgf, 120, 500, RGBA8(255, 255, 255, 255), 1.0f, "E1 E1 FF CF D7 CA C1 C9 BA E1 E3 DE D5 CF D3 FA ED E6 2F 00 2F");

    vita2d_end_drawing();
    vita2d_swap_buffers();

    oldPad = pad;
  }

  vita2d_fini();
  vita2d_free_texture(image);
  vita2d_free_pgf(pgf);
  vita2d_free_pvf(pvf);

  sceKernelExitProcess(0);
  return 0;
}