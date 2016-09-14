#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "gdfontg.h"
#include "utils.h"

char *progname = "";
char *infile = "";
char *outfile = "";
char *credits = "FTL MODA at Mercedes Benz Fashion Week A/W 2014";
char *font = "/usr/share/fonts/truetype/msttcorefonts/Georgia_Bold.ttf";
float size = 12.0;
int hor_margin = 50;   /* Positive = from left, negative = from right */
int ver_margin = 50;   /* Positive = from top, negative = from bottom */
int textposition = 0;  /* 0 = NW, 1 = NE, 2 = SW, 3 = SE */
char *RGB = "FFFFFF";
char *BKG = "000000";
int doBKG = 0;
int bm = 5;			/* background margin */

void usage() {
  fprintf(stderr, "Usage: %s [options] infile [outfile]\n", progname);
  fprintf(stderr, "\nAdd text to an existing image. The new image is written back to infile, or to outfile if specified.\n");
  fprintf(stderr, "Input and output file formats are automatically determined on the basis of the file extension.\n");
  fprintf(stderr, "Known extensions: png, PNG, jpg, JPG, gif, GIF.\n\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -t  text to print (use double quotes if it contains spaces; unicode allowed)\n");
  fprintf(stderr, "  -f  font to use (should be the path of a .ttf file)\n");
  fprintf(stderr, "  -s  text point size (default: 12pt)\n");
  fprintf(stderr, "  -c  text color as RGB (e.g. FF12A8)\n");
  fprintf(stderr, "  -bg  background color\n");
  fprintf(stderr, "  -bm  background margin\n");
  fprintf(stderr, "  -mx text position from left side (from right if negative)\n");
  fprintf(stderr, "  -my text position from top of image (from bottom if negative)\n");
  fprintf(stderr, "  -q  jpeg quality (0-100, default: -1)\n");
  fprintf(stderr, "  -h  print help\n");
  fprintf(stderr, "\n");
}

void parse_args(int argc, char *argv[]) {
  int i = 1;
  char *arg;
  int filenames = 0;

  while(i < argc) {
    arg = argv[i];
    if (!strcmp(arg, "-h")) {
      usage();
      exit(0);
    } else if (!strcmp(arg, "-f")) {
      font = argv[++i];
    } else if (!strcmp(arg, "-s")) {
      size = atof(argv[++i]);
    } else if (!strcmp(arg, "-t")) {
      credits = argv[++i];
    } else if (!strcmp(arg, "-c")) {
      RGB = argv[++i];
    } else if (!strcmp(arg, "-bc")) {
      BKG = argv[++i];
      doBKG = 1;
    } else if (!strcmp(arg, "-bm")) {
      bm = atoi(argv[++i]);
    } else if (!strcmp(arg, "-mx")) {
      hor_margin = atoi(argv[++i]);
    } else if (!strcmp(arg, "-my")) {
      ver_margin = atoi(argv[++i]);
    } else if (!strcmp(arg, "-q")) {
      jpg_quality = atoi(argv[++i]);
    } else if (filenames == 0) {
      infile = argv[i];
      outfile = infile; /* if no outfile is specified, write back to input file */
      filenames = 1;
    } else {
      outfile = argv[i];
    }
    i++;
  }
  if (filenames == 0) {
    usage();
    exit(0);
  }
  /* printf("In: %s\nOut: %s\nText: %s\nFont: %s\nSize: %f\n", infile, outfile, credits, font, size); */
}

void determine_position() {
  if (hor_margin < 0) {
    textposition = 1;
  }
  if (ver_margin < 0) {
    textposition = textposition + 2;
  }
  hor_margin = abs(hor_margin);
  ver_margin = abs(ver_margin);
}

int main(int argc, char *argv[]) {
  gdImagePtr im_in;
  FILE *out;
  int w, h;			/* Image width and height */
  int posx, posy;		/* X and Y coordinates where text should be printed */
  int textcol, bkgcol;
  int brect[8];
  int i;

  progname = argv[0];
  parse_args(argc, argv);
  determine_position();

  im_in = gdOpenByExt(infile);

  w = im_in->sx;
  h = im_in->sy;
  printf("Input: %s (%dx%d).\n", infile, w, h);

  /* allocate color for text */
  textcol = gdImageColorClosest(im_in, decodeHex(RGB[0], RGB[1]), decodeHex(RGB[2], RGB[3]), decodeHex(RGB[4], RGB[5]));

  /* determine text dimensions */
  gdImageStringFT(NULL, &brect[0], textcol, font, size, 0, 0, 0, credits);

  /* determine string position */
  switch (textposition) {
  case 0: posx = hor_margin; posy = ver_margin + (brect[1] - brect[7]); break;
  case 1: posx = w - hor_margin - (brect[4] - brect[6]); posy = ver_margin + (brect[1] - brect[7]); break;
  case 2: posx = hor_margin; posy = h - ver_margin; break;
  case 3: posx = w - hor_margin - (brect[4] - brect[6]); posy = h - ver_margin; break;
  }

  /* get real bounding box */
  gdImageStringFT(NULL, &brect[0], textcol, font, size, 0, posx, posy, credits);

  /* Draw background if requested */
  if (doBKG) {
    bkgcol = gdImageColorClosest(im_in, decodeHex(BKG[0], BKG[1]), decodeHex(BKG[2], BKG[3]), decodeHex(BKG[4], BKG[5]));
    gdImageFilledRectangle(im_in, brect[6]-bm, brect[7]-bm, brect[2]+bm, brect[3]+bm, bkgcol);
  }

  /* Draw string */
  gdImageStringFT(im_in, &brect[0], textcol, font, size, 0, posx, posy, credits);

  /*  gdImageString(im_out, smallfont, neww-10-credlen, newh-(smallfont->h)-10, credits, white); */
  /*
  printf("String bounding box:");
  for (i = 0; i < 8; i++) {
    printf(" %d", brect[i]);
  }
  printf("\n");
  */

  /*
  printf("%d, %d, %d\n", textposition, posx, posy);
  printf("String position: %d, %d\n", posx, posy); 
  */

  gdSaveByExt(im_in, outfile);
  gdImageDestroy(im_in);
}

