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
int viewportOn = 0; // is viewport active?
int bbx1, bby1, bbx2, bby2; // coordinates of viewport bounding box (real)
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
    return round(bbx1 + ((x - vpx1) / (vpx2 - vpx1)) * (bbx2 - bbx1));
  } else {
    return round(x);
  }
}

int viewy(float y) {
  if (viewportOn) {
    return round(bby2 - ((y - vpy1) / (vpy2 - vpy1)) * (bby2 - bby1));
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
  int font, c;

  font = getNumber(stream);
  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  getLine(stream);

  gdImageString(image, getFont(font), 
		viewx(x), viewy(y), buffer, c);
}

void doStringUp(FILE *stream) {
  float x, y;
  int font, c;

  font = getNumber(stream);
  x = getFloat(stream);
  y = getFloat(stream);
  c = getColor(getNumber(stream));
  getLine(stream);

  gdImageStringUp(image, getFont(font), 
		  viewx(x), viewy(y), buffer, c);
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

  tagArray[1] = "SA";
  cmdArray[1] = doSave;

  tagArray[2] = "CA";
  cmdArray[2] = doColorAllocate;

  tagArray[3] = "PI";
  cmdArray[3] = doPixel;

  tagArray[4] = "LI";
  cmdArray[4] = doLine;

  tagArray[5] = "RE";
  cmdArray[5] = doRectangle;

  tagArray[6] = "RF";
  cmdArray[6] = doFilledRectangle;
  
  tagArray[7] = "ST";
  cmdArray[7] = doString;

  tagArray[8] = "SU";
  cmdArray[8] = doStringUp;
  
  tagArray[9] = "FI";
  cmdArray[9] = doFill;

  tagArray[10] = "FB";
  cmdArray[10] = doFillToBorder;

  tagArray[11] = "CI";
  cmdArray[11] = doCircle;

  tagArray[12] = "AR";
  cmdArray[12] = doArc;

  tagArray[13] = "AF";
  cmdArray[13] = doFilledArc;

  tagArray[14] = "EF";
  cmdArray[14] = doFilledEllipse;

  tagArray[15] = "VI";
  cmdArray[15] = doViewportOn;

  tagArray[16] = "VO";
  cmdArray[16] = doViewportOff;

  tagArray[17] = "LD";
  cmdArray[17] = doLoad;

  ncommands = 18;
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

int main(int argc, char *argv[]) {
  char *filename;
  FILE *input;

  initFunctions();
  // printf("Functions initialized.\n");
  if (argc == 2) {
    filename = argv[1];
    input = fopen(filename, "r");
    gdMainLoop(input);
    fclose(input);
  } else {
    gdMainLoop(stdin);
  }
}
