#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "utils.h"

char *infile, *outfile;
int x = 60, y = 60;
int sizeonly = 1;
int shortFormat = 0;
float xscale = 0.0, yscale = 0.0, scale = 0.0;

float is_perc(char *s) {
  size_t l;

  l = strlen(s);
  if (s[l-1] == '%') {
    s[l-1] = '\0';
    return atoi(s)/100.0;
  } else {
    return 0.0;
  }
}

int is_dash(char *s) {
  return !strcmp(s, "-");
}

void usage(char *progname) {
  fprintf(stderr, "Usage: %s [options] in.png <out.png> <x-dim> <y-dim>\n", progname);
  fprintf(stderr, "\nWhen no output file is specified, prints the X and Y dimensions of an existing image, in the following format:\n");
  fprintf(stderr, "  in.png width height ratio\n");
  fprintf(stderr, "  With the -x option, the dimensions are printed in the form WxH (e.g., 640x400).\n");
  fprintf(stderr, "New dimensions can be specified as:\n");
  fprintf(stderr, "  an integer number;\n");
  /* fprintf(stderr, "  a floating point number between 0 and 1 (used as scale factor);\n"); */
  fprintf(stderr, "  a percentage in the range 0%% - 100%% (used as scale factor);\n");
  fprintf(stderr, "  '-' (new size will be computed using the same scale factor as the other coordinate).\n\n");
  fprintf(stderr, "Image file format is automatically determined from the file extension. Known extensions: png, PNG, jpg, JPG, gif, GIF.\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -q  jpeg quality (0-100, default: -1)\n");
  fprintf(stderr, "  -h  print help\n");
}

void parse_args(int argc, char *argv[]) {
  char *arg;
  int i = 1;
  int na = 1;  /* 1 = in, 2 = out, 3 = xdim, 4 = ydim */

  while (i < argc) {
    arg = argv[i];
    if (!strcmp(arg, "-h")) {
      usage(argv[0]);
      exit(0);
    } else if (!strcmp(arg, "-q")) {
      jpg_quality = atoi(argv[++i]);
    } else if (!strcmp(arg, "-x")) {
      shortFormat = 1;
    } else {
      switch (na) {
      case 1: 
	infile = arg;
	outfile = arg;
	na++;
	break;
      case 2:
	outfile = arg;
	na++;
	break;
      case 3:
	sizeonly = 0; /* we actually want rescaling */
	xscale = is_perc(arg);
	yscale = xscale;
	if (xscale == 0.0) {
	  if (is_dash(arg)) {
	    x = 0;
	  } else {
	    x = atoi(arg);
	    y = x;
	  }
	}
	na++;
	break;
      case 4:
	yscale = is_perc(arg);
	if (yscale == 0.0) {
	  if (is_dash(arg)) {
	    y = 0;
	  } else {
	    y = atoi(arg);
	  }
	}
	na++;
	break;
      }
    }
    i++;
  }
  /*
  printf("x = %d\n", x);
  printf("xscale = %f\n", xscale);
  printf("y = %d\n", y);
  printf("yscale = %f\n", yscale);
  printf("size = %d\n", sizeonly);
  */
}

int main(int argc, char *argv[]) {
  gdImagePtr im_in;
  gdImagePtr im_out;
  char *ext;
  FILE *in;
  FILE *out;

  if (argc == 1) {
    usage(argv[0]);
    exit(0);
  } else {
    parse_args(argc, argv);
  }

  /*
  if (argc == 5) {
    
    xscale = is_perc(argv[3]);
    if (xscale == 0.0) {
      if (is_dash(argv[3])) {
	x = 0;
      } else {
	x = atoi(argv[3]);
      }
    }
    yscale = is_perc(argv[4]);
    if (yscale == 0.0) {
      if (is_dash(argv[4])) {
	y = 0;
      } else {
	y = atoi(argv[4]);
      }
    }
  } else if (argc == 4) {
    xscale = is_perc(argv[3]);
    if (xscale == 0.0) {
      x = atoi(argv[3]);
      y = x;
    } else {
      yscale = xscale;
    }
  } else if (argc == 2) {
    sizeonly = 1;
  } else if (argc != 3) {
    exit(1);
  }
  */

  /* printf("%d,%d\n", x, y); */
  /* printf("%f,%f\n", xscale, yscale); */
  /* Load the original png or jpg */

  im_in = gdOpenByExt(infile);

  /* printf("input read.\n"); */
  if (xscale > 0.0) {
    x = (int)roundf(im_in->sx * xscale);
  }
  if (yscale > 0.0) {
    y = (int)roundf(im_in->sy * yscale);
  }
  if (x == 0) {
    scale = 1.0 * y / im_in->sy;
    x = (int)roundf(im_in->sx * scale);
  }
  if (y == 0) {
    scale = 1.0 * x / im_in->sx;
    y = (int)roundf(im_in->sy * scale);
  }

  if (sizeonly) {
    if (shortFormat) {
      printf("%dx%d\n", im_in->sx, im_in->sy);
    } else {
      printf("%s: %d %d %f\n", infile, im_in->sx, im_in->sy, (float)im_in->sx/im_in->sy);
    }
    exit(0);
  } else {
    printf("Resizing %s (%d x %d) to %s (%d x %d)\n", infile, im_in->sx, im_in->sy, outfile, x, y);
  }

  im_out = gdImageCreateTrueColor(x, y);
  gdImageCopyResampled(im_out, im_in, 0, 0, 0, 0,
		       x, y,
		       im_in->sx, im_in->sy);	

  gdSaveByExt(im_out, outfile);
  gdImageDestroy(im_in);
  gdImageDestroy(im_out);
}
