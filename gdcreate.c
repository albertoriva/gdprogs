#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "utils.h"

#define BUFSIZE 1024
char buffer[BUFSIZE];

typedef void (*fp) (FILE *);
int ncommands = 0;
fp cmdArray[BUFSIZE];
char *tagArray[BUFSIZE];
char *dscArray[BUFSIZE];
int viewportOn = 0; // is viewport active?
int bbx1, bby1, bbx2, bby2; // coordinates of viewport bounding box (real)
int bbw, bbh;               // size of viewport bounding box
int vpx1, vpy1, vpx2, vpy2; // coordinates of viewport (virtual)

#define MAXCOLOR 256
int colors[MAXCOLOR];
int colorptr = 0;

gdImagePtr image;
extern gdFontPtr gdFontTiny;
extern gdFontPtr gdFontSmall;
extern gdFontPtr gdFontMediumBold;
extern gdFontPtr gdFontLarge;
extern gdFontPtr gdFontGiant;

/* Utils */

void string_lower(char *s) {
  int i;

  for(i = 0; s[i]; i++) {
    s[i] = tolower(s[i]);
  }
}

void reportError(char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

void getLine(FILE *stream) {
  int len;

  if (fgets(buffer, BUFSIZE, stream) == NULL)
    reportError("Error reading from command stream.");
  len = strlen(buffer) - 1;
  if (buffer[len] == '\n')
    buffer[len] = '\0';
}

int getNumber(FILE *stream) {
  getLine(stream);
  return atoi(buffer);
}

float getFloat(FILE *stream) {
  getLine(stream);
  return strtof(buffer, NULL);  // TODO: error checking here
}

/* Viewport management */

int viewx(float x) {
  if (viewportOn) {
    return round(bbx1 + ((x - vpx1) / (vpx2 - vpx1)) * bbw);
  } else {
    return round(x);
  }
}

int viewy(float y) {
  if (viewportOn) {
    return round(bby2 - ((y - vpy1) / (vpy2 - vpy1)) * bbh);
  } else {
    return round(y);
  }
}

int scalex(float x) {
  if (viewportOn) {
    return round((x / (vpx2 - vpx1)) * (bbx2 - bbx1));
  } else {
    return x;
  }
}

int scaley(float y) {
  if (viewportOn) {
    return round((y / (vpy2 - vpy1)) * (bby2 - bby1));
  } else {
    return round(y);
  }
}

/* Commands:

CR - create
x
y

LD - load
filename

SA - save
filename

CA - color allocate
red
green
blue

CC - color closest
red
green
blue

CE - color exact
red
green
blue

CR - color resolve
red
green
blue

PI - set pixel
x
y
c

LI - line
x1 
y1
x2
y2
c

RE - rectangle
x1 
y1
x2
y2
c

RF - rectangle filled
x1 
y1
x2
y2
c

ST - string
font
x
y
c
string

*/

/* Drawing functions */
gdFontPtr getFont(int f) {
  switch (f) {
  case 0: return gdFontTiny; break;
  case 1: return gdFontSmall; break;
  case 2: return gdFontMediumBold; break;
  case 3: return gdFontLarge; break;
  case 4: return gdFontGiant; break;
  }
  return gdFontSmall;  // default
}

int getColor(int idx) {
  return colors[idx];
}

void doCreate(FILE *stream) {
  int x, y;
  x = getNumber(stream);
  y = getNumber(stream);
  image = gdImageCreate(x, y);
}

void doLoad(FILE *stream) {
  getLine(stream); // read filename
  image = gdOpenByExt(buffer);
}

void doSave(FILE *stream) {
  getLine(stream); // read filename
  gdSaveByExt(image, buffer);
  gdImageDestroy(image);
  printf("(image saved to %s) ", buffer);
}

void doColorAllocate(FILE *stream) {
  int r, g, b, idx;

  r = getNumber(stream);
  g = getNumber(stream);
  b = getNumber(stream);
  idx = gdImageColorAllocate(image, r, g, b);
  colors[colorptr] = idx;
  colorptr++;
}

void doPixel(FILE *stream) {
  float x, y;
  int c;
  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  gdImageSetPixel(image, viewx(x), viewy(y), c);
}

void doLine(FILE *stream) {
  float x1, y1, x2, y2;
  int c;

  x1 = getFloat(stream);
  y1 = getFloat(stream);
  x2 = getFloat(stream);
  y2 = getFloat(stream);
  c = getColor(getNumber(stream));
  gdImageLine(image, viewx(x1), viewy(y1), viewx(x2), viewy(y2), c);
}

void doRectangle(FILE *stream) {
  float x1, y1, x2, y2;
  int c;

  x1 = getFloat(stream);
  y1 = getFloat(stream);
  x2 = getFloat(stream);
  y2 = getFloat(stream);
  c = getColor(getNumber(stream));
  gdImageRectangle(image, viewx(x1), viewy(y1), viewx(x2), viewy(y2), c);
}

void doFilledRectangle(FILE *stream) {
  float x1, y1, x2, y2;
  int c;

  x1 = getFloat(stream);
  y1 = getFloat(stream);
  x2 = getFloat(stream);
  y2 = getFloat(stream);
  c = getColor(getNumber(stream));
  gdImageFilledRectangle(image, viewx(x1), viewy(y1), viewx(x2), viewy(y2), c);
}

void doString(FILE *stream) {
  float x, y;
  int font, a, c;

  font = getNumber(stream);
  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  a = getNumber(stream);
  getLine(stream);

  // compute anchor position here

  gdImageString(image, getFont(font), 
		viewx(x), viewy(y), buffer, c);
}

void doStringUp(FILE *stream) {
  float x, y;
  int font, a, c;

  font = getNumber(stream);
  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  a = getNumber(stream);
  getLine(stream);

  // compute anchor position here

  gdImageStringUp(image, getFont(font), 
		  viewx(x), viewy(y), buffer, c);
}

void doPolygon(FILE *stream) {
  gdPoint points[10];
  int n, i, c;

  n = getNumber(stream);
  for (i = 0; i < n; i++) {
    points[i].x = viewx(getNumber(stream));
    points[i].y = viewy(getNumber(stream));
  }
  c = getColor(getNumber(stream));
  gdImagePolygon(image, points, n, c);
}

void doFilledPolygon(FILE *stream) {
  gdPoint points[10];
  int n, i, c;

  n = getNumber(stream);
  for (i = 0; i < n; i++) {
    points[i].x = viewx(getNumber(stream));
    points[i].y = viewy(getNumber(stream));
  }
  c = getColor(getNumber(stream));
  gdImageFilledPolygon(image, points, n, c);
}

void doFill(FILE *stream) {
  float x, y;
  int c;

  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));

  gdImageFill(image, viewx(x), viewy(y), c);
}

void doFillToBorder(FILE *stream) {
  float x, y;
  int b, c;

  x = getFloat(stream);
  y = getFloat(stream);
  b = getNumber(stream);
  c = getColor(getNumber(stream));

  gdImageFillToBorder(image, viewx(x), viewy(y), b, c);
}
  
void doCircle(FILE *stream) {
  float cx, cy, r;
  int c;

  cx = getFloat(stream);
  cy = getFloat(stream);
  r = getFloat(stream);
  c = getColor(getNumber(stream));

  gdImageArc(image, viewx(cx), viewy(cy), 
	     scalex(r)*2, scaley(r)*2, 0, 360, c);
}

void doArc(FILE *stream) {
  float cx, cy, w, h;
  int s, e, c;

  cx = getFloat(stream);  // center x
  cy = getFloat(stream);  // center y
  w = getFloat(stream);   // width
  h = getFloat(stream);   // height
  s = getNumber(stream);   // start angle
  e = getNumber(stream);   // end angle
  c = getColor(getNumber(stream));   // color

  gdImageArc(image, viewx(cx), viewy(cy), 
	     scalex(w), scaley(h), s, e, c);
}

void doFilledArc(FILE *stream) {
  float cx, cy, w, h, s;
  int e, c, style;

  cx = getFloat(stream);  // center x
  cy = getFloat(stream);  // center y
  w = getFloat(stream);   // width
  h = getFloat(stream);   // height
  s = getNumber(stream);   // start angle
  e = getNumber(stream);   // end angle
  c = getColor(getNumber(stream));   // color
  style = getNumber(stream);

  gdImageFilledArc(image, viewx(cx), viewy(cy), 
		   scalex(w), scaley(h), s, e, c, style);
}

void doFilledEllipse(FILE *stream) {
  float cx, cy, w, h;
  int c;

  cx = getFloat(stream);
  cy = getFloat(stream);
  w = getFloat(stream);
  h = getFloat(stream);
  c = getColor(getNumber(stream));

  gdImageFilledEllipse(image, viewx(cx), viewy(cy), 
		       scalex(w), scaley(h), c);
}

void doViewportOn(FILE *stream) {

  bbx1 = getNumber(stream);
  bby1 = getNumber(stream);
  bbx2 = getNumber(stream);
  bby2 = getNumber(stream);
  bbw  = bbx2 - bbx1;
  bbh  = bby2 - bby1;
  vpx1 = getNumber(stream);
  vpy1 = getNumber(stream);
  vpx2 = getNumber(stream);
  vpy2 = getNumber(stream);
  viewportOn = 1;
}

void doViewportOff(FILE *stream) {
  viewportOn = 0;
}

/* Codes and functions */

void initFunctions() {
  tagArray[0] = "CR";
  cmdArray[0] = doCreate;
  dscArray[0] = "Create new image with specified dimensions.\n W - image width\n H - image height\n";

  tagArray[1] = "SA";
  cmdArray[1] = doSave;
  dscArray[1] = "Save current image to specified file.\n S - filename of output image\n";

  tagArray[2] = "CA";
  cmdArray[2] = doColorAllocate;
  dscArray[2] = "Allocate a color with the specified R, G, and B components.\n R - red component (0-255)\n G - green component (0-255)\n B - blue component (0-255)\n";

  tagArray[3] = "PI";
  cmdArray[3] = doPixel;
  dscArray[3] = "Draw a pixel.\n X - x coordinate of pixel\n Y - y coordinate of pixel\n C - pixel color\n";

  tagArray[4] = "LI";
  cmdArray[4] = doLine;
  dscArray[4] = "Draw a line from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - line color.\n";

  tagArray[5] = "RE";
  cmdArray[5] = doRectangle;
  dscArray[5] = "Draw a rectangle from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - rectangle color.\n";

  tagArray[6] = "RF";
  cmdArray[6] = doFilledRectangle;
  dscArray[6] = "Draw a filled rectangle from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - rectangle color.\n";

  tagArray[7] = "ST";
  cmdArray[7] = doString;
  dscArray[7] = "Draw a string.\n X - x coordinate of string\n Y - y coordinate of string\n C - string color\n S - string to be drawn.\n";

  tagArray[8] = "SU";
  cmdArray[8] = doStringUp;
  dscArray[8] = "Draw a string with vertical orientation.\n X - x coordinate of string\n Y - y coordinate of string\n C - string color\n S - string to be drawn.\n";

  tagArray[9] = "PO";
  cmdArray[9] = doPolygon;
  dscArray[9] = "Draw a polygon with the specified vertices.\n N - number of points\n X1\n Y1 - coordinates of first point\n ...\n C - color\n";

  tagArray[10] = "PF";
  cmdArray[10] = doFilledPolygon;
  dscArray[10] = "Draw a filled polygon with the specified vertices.\n N - number of points\n X1\n Y1 - coordinates of first point\n ...\n C - color\n";

  tagArray[11] = "FI";
  cmdArray[11] = doFill;
  dscArray[11] = "Fill a region with the specified color.\n X - x coordinate of seed point\n Y - y coordinate of seed point\n C - fill color.\n";

  tagArray[12] = "FB";
  cmdArray[12] = doFillToBorder;
  dscArray[12] = "Fill to border.\n";

  tagArray[13] = "CI";
  cmdArray[13] = doCircle;
  dscArray[13] = "Draw a circle.\n X - x coordinate of center\n Y - y coordinate of center\n R - circle radius\n C - circle color.\n";

  tagArray[14] = "AR";
  cmdArray[14] = doArc;
  dscArray[14] = "Draw an arc.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n A - start angle\n B - end angle\n C - arc color.\n";

  tagArray[15] = "AF";
  cmdArray[15] = doFilledArc;
  dscArray[15] = "Draw a filled arc.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n A - start angle\n B - end angle\n C - arc color\n S - style index.\n";

  tagArray[16] = "EF";
  cmdArray[16] = doFilledEllipse;
  dscArray[16] = "Draw a filled ellipse.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n C - ellipse color.\n";

  tagArray[17] = "VI";
  cmdArray[17] = doViewportOn;
  dscArray[17] = "Enable viewport (all successive operations will use viewport coordinates).\n BBX1\n BBY1 - upper left corner of bounding box\n BBX2\n BBY2 - lower right corner of bounding box\n VPX1\n VPY1 - upper left corner of viewport\n VPX2\n VPY2 - lower right corner of viewport.\n";

  tagArray[18] = "VO";
  cmdArray[18] = doViewportOff;
  dscArray[18] = "Disable viewport (revert to image coordinates).\n";

  tagArray[19] = "LD";
  cmdArray[19] = doLoad;
  dscArray[19] = "Load image from a file.\n S - filename of image to be read.";

  ncommands = 20;
}

int findCode(char *code) {
  int i;
  for (i = 0; i < ncommands; i++) {
    if (!strcmp(code, tagArray[i]))
      return i;
  }
  return -1;
}

void callFromCode(char *code, FILE *stream) {
  int i;

  i = findCode(code);
  if (i >= 0) {
    printf("%s: ", code);
    cmdArray[i](stream);
    printf("ok\n");
  }
}

void gdMainLoop(FILE *stream) {
  int cont = 1;
  
  while (cont) {
    getLine(stream);
    // printf("Read: %s\n", buffer);
    if (!strcmp(buffer, "ZZ")) {
      cont = 0;
    } else {
      callFromCode(buffer, stream);
    }
  }
}

void usage() {
  fprintf(stderr, "gdcreate - create images from simple text-based commands\n\n");
  fprintf(stderr, "Usage: gdcreate [-h] [-l] [input.txt]\n\n");
  fprintf(stderr, "Read commands from file input.txt (or from standard input if no file is\n");
  fprintf(stderr, "specified). Each command represents a graphical operation performed on an\n");
  fprintf(stderr, "image in memory. The first command will typically be CR (create) to initialize\n");
  fprintf(stderr, "the image to the desired dimensions, and the last command will typicall be SA\n");
  fprintf(stderr, "(save) to write the image to a file. Use the '-l' option to print the list of all\n");
  fprintf(stderr, "available commands with a short description for each.\n");
  exit(1);
}

void listCommands() {
  int i;
  fprintf(stderr, "Each command starts with a two-letter code, followed by one or more arguments. The\n");
  fprintf(stderr, "code and its arguments should all be on different, consecutive lines. An empty line\n");
  fprintf(stderr, "terminates a command. The following is the list of currently available commands,\n");
  fprintf(stderr, "followed by the arguments for each command.\n\n");

  for (i = 0; i < ncommands; i++) {
    fprintf(stderr, "%s - %s\n", tagArray[i], dscArray[i]);
  }
  exit(2);
}

int main(int argc, char *argv[]) {
  char *filename;
  FILE *input;

  initFunctions();
  // printf("Functions initialized.\n");
  if (argc == 2) {
    if (!strcmp(argv[1], "-h")) {
      usage();
    }
    if (!strcmp(argv[1], "-l")) {
      listCommands();
    }
    filename = argv[1];
    input = fopen(filename, "r");
    gdMainLoop(input);
    fclose(input);
  } else {
    gdMainLoop(stdin);
  }
}
