#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "utils.h"

int main(int argc, char *argv[]) {
  gdImagePtr im_in;
  gdImagePtr im_out;
  char *ext;
  FILE *in;
  FILE *out;

  int x, y, w, h;
  int transp, red, green, blue;

  x = atoi(argv[3]);
  y = atoi(argv[4]);
  if (argv[5][0] == '+') {
    w = atoi(argv[5]);
  } else {
    w = atoi(argv[5]) - x;
  }
  if (argv[6][0] == '+') {
    h = atoi(argv[6]);
  } else {
    h = atoi(argv[6]) - y;
  }

  im_in = gdOpenByExt(argv[1]);

  printf("Cropping (%d,%d) to (%d,%d) from %s\n", x, y, x+w, y+h, argv[1]);
  transp = gdImageGetTransparent(im_in);
  //im_out = gdImageCreateTrueColor(w, h);
  im_out = gdImageCreate(w, h);
  gdImageCopy(im_out, im_in, 0, 0, x, y, w, h);

  // Check if the original image had a transparent color
  if (transp != -1) {
    // If so, get components
    red = gdImageRed(im_in, transp);
    blue = gdImageBlue(im_in, transp);
    green = gdImageGreen(im_in, transp);
    printf("Color %d (%d,%d,%d) is transparent.\n", transp, red, green, blue);

    // Look for corresponding color in output image
    transp = gdImageColorClosest(im_out, red, green, blue);
    if (transp != -1) {
      printf("Setting color %d to transparent.\n", transp);
      gdImageColorTransparent(im_out, transp);
    }
  }

  gdSaveByExt(im_out, argv[2]);
  gdImageDestroy(im_in);
  gdImageDestroy(im_out);
}

