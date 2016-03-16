/* utils.c */

int jpg_quality;
char *extension_of(char *s);
gdImagePtr gdOpenByExt(char *filename);
void gdSaveByExt(gdImagePtr im_out, char *filename);
int decodeHex(char h1, char h2);
