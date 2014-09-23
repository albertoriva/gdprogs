#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "utils.h"

void check_args(int n, char *args[]) {
  if (n < 7) {
    fprintf(stderr, "%s - crop images.\n\n", args[0]);
    fprintf(stderr, "Usage: gdcrop src dest x y dx dy\n\n");
    fprintf(stderr, "  src, dest: filenames of the source and destination images respectively.\n");
    fprintf(stderr, "             The image format is automatically detected from the filename\n");
    fprintf(stderr, "             extension, that should be one of png, jpg, jpeg, gif (case insensitive).\n");
    fprintf(stderr, "  x, y:      Coordinates of the top-left corner of the rectangle to be cropped.\n");
    fprintf(stderr, "  dx, dy:    Coordinates of the bottom-right corner of the rectangle to be cropped.\n");
    fprintf(stderr, "             If these arguments are preceded by a +, the value represents the width\n");
    fprintf(stderr, "             and height of the rectangle to be cropped, respectively.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n\n");
    fprintf(stderr, "  gdcrop in.png out.png 10 12 31 48\n");
    fprintf(stderr, "    Copies the rectangle from (10,12) to (13,48) from in.png and writes it to out.png\n\n");
    fprintf(stderr, "  gdcrop in.png out.png 10 12 +21 +36\n");
    fprintf(stderr, "    Copies the rectangle starting at (10,12) with dimensions (21, 36) from in.png and\n");
    fprintf(stderr, "    writes it to out.png (produces the same output as the previous example).\n");
    exit(1);
  }
}
      
int main(int argc, char *argv[]) {
  gdImagePtr im_in;
  gdImagePtr im_out;
  char *ext;
  FILE *in;
  FILE *out;

  int x, y, w, h;
  int transp, red, green, blue;

  check_args(argc, argv);

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

