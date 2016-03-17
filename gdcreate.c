#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "utils.h"

#define BUFSIZE 1024
char buffer[BUFSIZE];
char cmdresult[BUFSIZE];

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

// Font support

extern gdFontPtr gdFontTiny;
extern gdFontPtr gdFontSmall;
extern gdFontPtr gdFontMediumBold;
extern gdFontPtr gdFontLarge;
extern gdFontPtr gdFontGiant;

int currentFontIdx;		/* If 0, font is FT */
gdFontPtr currentFont;
char currentFontFT[BUFSIZE];

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

/* Font management */

gdPoint fontSize(int f) {
  // Returns the X and Y size of font `f' as a gdPoint
  gdPoint s;
  switch (f) {
  case 1: s.x = 5; s.y = 8; break;
  case 2: s.x = 6; s.y = 12; break;
  case 3: s.x = 7; s.y = 13; break;
  case 4: s.x = 8; s.y = 16; break;
  case 5: s.x = 9; s.y = 15; break;
  }
  return s;
}

gdPoint stringSize(char *s, int f) {
  // Returns the X and Y dimensions of string `s' when printed with font `f' (a GD font index).
  gdPoint fsize;
  gdPoint result;

  fsize = fontSize(f);
  //printf("%dx%d %d\n", fsize.x, fsize.y, strlen(s));

  result.x = strlen(s) * fsize.x;
  result.y = fsize.y;
  return result;
}

gdPoint stringAnchor(gdPoint point, gdPoint dimensions, int anchor, int vertical) {
  /* Returns the point at which a string of the specified `dimensions' should
     be drawn so that its anchor point `anchor' is at `point'. Anchor points:

     1--------2--------3
     |                 |
     4        5        6
     |                 |
     7--------8--------9
  
     When `vertical' is 1, returns coordinate suitable for gdStringUp with the
     following pattern:

     7--------8--------9
     |                 |
     4        5        6
     |                 |
     1--------2--------3

  */

  gdPoint result;
  int w, h, dir;

  if (vertical) {
    w = dimensions.y;
    h = dimensions.x;
    printf("w=%d, h=%d\n", w, h);
    dir = 1;
  } else {
    w = dimensions.x;
    h = dimensions.y;
    dir = -1;
  }
  int halfw = w / 2;
  int halfh = h / 2;

  switch (anchor) {
  case 1: result.x = point.x;         result.y = point.y; break;
  case 2: result.x = point.x - halfw; result.y = point.y; break;
  case 3: result.x = point.x - w;     result.y = point.y; break;
  case 4: result.x = point.x;         result.y = point.y + (dir * halfh); break;
  case 5: result.x = point.x - halfw; result.y = point.y + (dir * halfh); break;
  case 6: result.x = point.x - w;     result.y = point.y + (dir * halfh); break;
  case 7: result.x = point.x;         result.y = point.y + (dir * h); break;
  case 8: result.x = point.x - halfw; result.y = point.y + (dir * h); break;
  case 9: result.x = point.x - w;     result.y = point.y + (dir * h); break;
  }
  printf("moved to: %d, %d\n", result.x, result.y);
  return result;
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
    return round((x / (vpx2 - vpx1)) * bbw);
  } else {
    return x;
  }
}

int scaley(float y) {
  if (viewportOn) {
    return round((y / (vpy2 - vpy1)) * bbh);
  } else {
    return round(y);
  }
}

/* Drawing functions */

gdFontPtr getFont(int f) {
  switch (f) {
  case 1: return gdFontTiny; break;
  case 2: return gdFontSmall; break;
  case 3: return gdFontMediumBold; break;
  case 4: return gdFontLarge; break;
  case 5: return gdFontGiant; break;
  }
  currentFontIdx = 2;
  return gdFontSmall;  // default
}

int getColor(int idx) {
  if (idx == -1) {
    return gdStyled;
  } else {
    return colors[idx];
  }
}

/* Commands */

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
  //printf("(image saved to %s) ", buffer);
}

void doColorAllocate(FILE *stream) {
  int r, g, b, idx;

  r = getNumber(stream);
  g = getNumber(stream);
  b = getNumber(stream);
  idx = gdImageColorAllocate(image, r, g, b);
  colors[colorptr] = idx;
  colorptr++;
  sprintf(cmdresult, "%d", idx);
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

void doSetFont(FILE *stream) {
  int f;

  f = getNumber(stream);
  currentFontIdx = f;
  if (f == 0) {
    getLine(stream);
    strcpy(currentFontFT, buffer);
  } else {
    currentFont = getFont(f);
  }
}

void doString(FILE *stream) {
  float x, y;
  int a, c;
  gdPoint size, point;

  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  a = getNumber(stream);
  getLine(stream);

  if (currentFontIdx == 0) {
    gdImageStringFT(image, NULL, c, currentFontFT, 10.0, 0.0, viewx(x), viewy(y), buffer);
  } else {
    size = stringSize(buffer, currentFontIdx);
    point.x = viewx(x);
    point.y = viewy(y);
    point = stringAnchor(point, size, a, 0);
    gdImageString(image, currentFont, point.x, point.y, buffer, c);
  }
}

void doFTString(FILE *stream) {
  float x, y, p, d;
  int a, c;

  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  a = getNumber(stream);
  p = getFloat(stream);
  d = getFloat(stream);
  getLine(stream);

  // Convert angle to radians

  a = (a * (22.0 / 7.0)) / 180.0;

  // compute anchor position here

  if (currentFontIdx == 0) {
    gdImageStringFT(image, NULL, c, currentFontFT, p, d, viewx(x), viewy(y), buffer);
  }
}

void doStringUp(FILE *stream) {
  float x, y;
  int a, c, tmp;
  gdPoint size, point;

  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  a = getNumber(stream);
  getLine(stream);

  // compute anchor position here

  size = stringSize(buffer, currentFontIdx);
  point.x = viewx(x);
  point.y = viewy(y);
  point = stringAnchor(point, size, a, 1);
  gdImageStringUp(image, currentFont, point.x, point.y, buffer, c);
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

void doSetStyle(FILE *stream) {
  int n, i, c;
  int style[100];
  
  n = getNumber(stream);
  for (i = 0; i < n; i++) {
    c = getNumber(stream);
    if (c == -1) {
      c = gdTransparent;
    }
    style[i] = c;
  }
  gdImageSetStyle(image, style, n);
}

void doSetThickness(FILE *stream) {
  int t;

  t = getNumber(stream);
  gdImageSetThickness(image, t);
}

/* Codes and functions */

void initFunctions() {
  int idx = 0;

  tagArray[idx] = "CR";
  cmdArray[idx] = doCreate;
  dscArray[idx] = "Create new image with specified dimensions.\n W - image width\n H - image height\n";
  idx++;

  tagArray[idx] = "SA";
  cmdArray[idx] = doSave;
  dscArray[idx] = "Save current image to specified file.\n S - filename of output image\n";
  idx++;

  tagArray[idx] = "CA";
  cmdArray[idx] = doColorAllocate;
  dscArray[idx] = "Allocate a color with the specified R, G, and B components.\n R - red component (0-255)\n G - green component (0-255)\n B - blue component (0-255)\n";
  idx++;

  tagArray[idx] = "PI";
  cmdArray[idx] = doPixel;
  dscArray[idx] = "Draw a pixel.\n X - x coordinate of pixel\n Y - y coordinate of pixel\n C - pixel color\n";
  idx++;

  tagArray[idx] = "LI";
  cmdArray[idx] = doLine;
  dscArray[idx] = "Draw a line from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - line color.\n";
  idx++;

  tagArray[idx] = "RE";
  cmdArray[idx] = doRectangle;
  dscArray[idx] = "Draw a rectangle from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - rectangle color.\n";
  idx++;

  tagArray[idx] = "RF";
  cmdArray[idx] = doFilledRectangle;
  dscArray[idx] = "Draw a filled rectangle from (x1, y1) to (x2, y2).\n X1 - x1 coordinate\n Y1 - y1 coordinate\n X2 - x2 coordinate\n Y2 - y2 coordinate\n C - rectangle color.\n";
  idx++;

  tagArray[idx] = "SF";
  cmdArray[idx] = doSetFont;
  dscArray[idx] = "Select current font.\n F - font id (0-5)\n N - TF font name (if F=0)\n";
  idx++;

  tagArray[idx] = "ST";
  cmdArray[idx] = doString;
  dscArray[idx] = "Draw a string.\n X - x coordinate of string\n Y - y coordinate of string\n C - string color\n A - anchor point\n S - string to be drawn.\n";
  idx++;

  tagArray[idx] = "S*";
  cmdArray[idx] = doFTString;
  dscArray[idx] = "Draw a string using a FreeType font.\n X - x coordinate of string\n Y - y coordinate of string\n C - string color\n A - anchor point\n P - ptsize\n D - angle\n S - string to be drawn.\n";
  idx++;

  tagArray[idx] = "SU";
  cmdArray[idx] = doStringUp;
  dscArray[idx] = "Draw a string with vertical orientation.\n X - x coordinate of string\n Y - y coordinate of string\n C - string color\n A - anchor point\n S - string to be drawn.\n";
  idx++;

  tagArray[idx] = "PO";
  cmdArray[idx] = doPolygon;
  dscArray[idx] = "Draw a polygon with the specified vertices.\n N - number of points\n X1\n Y1 - coordinates of first point\n ...\n C - color\n";
  idx++;

  tagArray[idx] = "PF";
  cmdArray[idx] = doFilledPolygon;
  dscArray[idx] = "Draw a filled polygon with the specified vertices.\n N - number of points\n X1\n Y1 - coordinates of first point\n ...\n C - color\n";
  idx++;

  tagArray[idx] = "FI";
  cmdArray[idx] = doFill;
  dscArray[idx] = "Fill a region with the specified color.\n X - x coordinate of seed point\n Y - y coordinate of seed point\n C - fill color.\n";
  idx++;

  tagArray[idx] = "FB";
  cmdArray[idx] = doFillToBorder;
  dscArray[idx] = "Fill to border.\n X - x coordinate of seed point\n Y - y coordinate of seed point\n B - border color\n C - fill color.\n";
  idx++;

  tagArray[idx] = "CI";
  cmdArray[idx] = doCircle;
  dscArray[idx] = "Draw a circle.\n X - x coordinate of center\n Y - y coordinate of center\n R - circle radius\n C - circle color.\n";
  idx++;

  tagArray[idx] = "AR";
  cmdArray[idx] = doArc;
  dscArray[idx] = "Draw an arc.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n A - start angle\n B - end angle\n C - arc color.\n";
  idx++;

  tagArray[idx] = "AF";
  cmdArray[idx] = doFilledArc;
  dscArray[idx] = "Draw a filled arc.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n A - start angle\n B - end angle\n C - arc color\n S - style index.\n";
  idx++;

  tagArray[idx] = "EF";
  cmdArray[idx] = doFilledEllipse;
  dscArray[idx] = "Draw a filled ellipse.\n X - x coordinate of center\n Y - y coordinate of center\n W - width\n H - height\n C - ellipse color.\n";
  idx++;

  tagArray[idx] = "VI";
  cmdArray[idx] = doViewportOn;
  dscArray[idx] = "Enable viewport (all successive operations will use viewport coordinates).\n BBX1\n BBY1 - upper left corner of bounding box\n BBX2\n BBY2 - lower right corner of bounding box\n VPX1\n VPY1 - upper left corner of viewport\n VPX2\n VPY2 - lower right corner of viewport.\n";
  idx++;

  tagArray[idx] = "VO";
  cmdArray[idx] = doViewportOff;
  dscArray[idx] = "Disable viewport (revert to image coordinates).\n";
  idx++;

  tagArray[idx] = "LD";
  cmdArray[idx] = doLoad;
  dscArray[idx] = "Load image from a file.\n S - filename of image to be read.\n";
  idx++;

  tagArray[idx] = "SS";
  cmdArray[idx] = doSetStyle;
  dscArray[idx] = "Set line style (-1 = transparent).\n N - number of colors in style\n C1 - first color\n C2 - second color\n ...\n";
  idx++;

  tagArray[idx] = "TH";
  cmdArray[idx] = doSetThickness;
  dscArray[idx] = "Set line thickness.\n T - thickness in pixels.\n";
  idx++;

  ncommands = idx;
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
  char *res;

  i = findCode(code);
  if (i >= 0) {
    sprintf(cmdresult, "ok");
    // printf("%s: ", code);
    cmdArray[i](stream);
    printf("%s\n", cmdresult);
  } else {
    printf("bad\n");
  }
}

void gdMainLoop(FILE *stream) {
  int cont = 1;
  
  while (cont) {
    getLine(stream);
    // printf("Read: %s\n", buffer);
    if (!strcmp(buffer, "ZZ")) {
      cont = 0;
    } else if (!strcmp(buffer, "")) {
      ;;
    } else if (buffer[0] == '#') {
      ;;
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
  fprintf(stderr, "the image to the desired dimensions or LD (load) to load an existing image.\n");
  fprintf(stderr, "The last command will typicall be SA (save) to write the image to a file.\n");
  fprintf(stderr, "Multiple images can be created and saved in the same session. Use the ZZ code\n");
  fprintf(stderr, "to terminate the session and exit the program.\n\n");
  fprintf(stderr, "The '-l' option prints all available commands with a short description for each.\n");
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

void verifyCommands() {
  int i, j;
  for (i = 0; i < ncommands; i++) {
    for (j = i+1; j < ncommands; j++) {
      // printf("%s %s\n", tagArray[i], tagArray[j]);
      if (!strcmp(tagArray[i], tagArray[j])) {
	fprintf(stderr, "Error: duplicate code %s\n", tagArray[i]);
	exit(-1);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  char *filename;
  FILE *input;

  initFunctions();
  verifyCommands();
  currentFontIdx = 2;
  currentFont = getFont(currentFontIdx);

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
