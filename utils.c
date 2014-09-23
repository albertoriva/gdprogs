#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"

int jpg_quality = -1;

char *extension_of(char *s) {
  int i, pos = 0;

  for (i = 0; s[i] != '\0'; i++) {
    if (s[i] == '.')
      pos = i+1;
  }
  return s+pos;
}

/* convert a two-digit hex number into an integer */
int decodeHex(char h1, char h2) {
  return 16 * (h1 - '0') + (h2 - '0');
}

gdImagePtr gdOpenByExt(char *filename) {
  FILE *in;
  char *ext;
  gdImagePtr im_in;

  in = fopen(filename, "rb");
  ext = extension_of(filename);
  if (!strcasecmp(ext, "png")) {
    im_in = gdImageCreateFromPng(in);
  } else if (!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg")) {
    im_in = gdImageCreateFromJpeg(in);
  } else if (!strcasecmp(ext, "gif")) {
    im_in = gdImageCreateFromGif(in);
  } else {
    fprintf(stderr, "Unrecognized file type: %s\n", ext);
    exit(1);
  }
  fclose(in);
  return im_in;
}

void gdSaveByExt(gdImagePtr im_out, char *filename) {
  FILE *out;
  char *ext;

  out = fopen(filename, "wb");
  ext = extension_of(filename);
  if (!strcasecmp(ext, "png")) {
    gdImagePng(im_out, out);
  } else if (!strcasecmp(ext, "jpg") || !strcasecmp(ext, "jpeg")) {
    gdImageJpeg(im_out, out, jpg_quality);
  } else if (!strcasecmp(ext, "gif")) {
    gdImageGif(im_out, out);
  } else {
    fprintf(stderr, "Unrecognized file type: %s\n", ext);
    exit(1);
  }
  fclose(out);
}
