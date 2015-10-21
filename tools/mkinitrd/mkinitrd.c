#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct initrd_header {
  unsigned char magic;  // magic number for consistency check
  char name[64];        // filename
  unsigned int offset;  // offset in the initrd file
  unsigned int length;  // length of the file
};

int main(int argc, char** argv) {
  int nheaders = (argc - 1) / 2;
  struct initrd_header headers[64];

  unsigned int offset = sizeof(struct initrd_header) * 64 + sizeof(int);
  int i;
  for (i = 0; i < nheaders; i++) {
    char* location = argv[i * 2 + 1];
    char* name = argv[i * 2 + 2];
    printf("writing file %s->%s at 0x%x\n", location, name, offset);
    strcpy(headers[i].name, name);
    headers[i].offset = offset;
    FILE* stream = fopen(location, "r");
    if (stream == 0) {
      printf("File not found: %s\n", location);
      return 1;
    }
    fseek(stream, 0, SEEK_END);
    headers[i].length = ftell(stream);
    offset += headers[i].length;
    fclose(stream);
    headers[i].magic = 0xBF;
  }

  FILE* wstream = fopen("./initrd", "w");
  printf("Creating ./initrd\n");
  unsigned char* data = (unsigned char*)malloc(offset);
  fwrite(&nheaders, sizeof(int), 1, wstream);
  fwrite(headers, sizeof(struct initrd_header), 64, wstream);

  for (i = 0; i < nheaders; i++) {
    char* location = argv[i * 2 + 1];
    FILE* stream = fopen(location, "r");
    unsigned char* buf = (unsigned char*)malloc(headers[i].length);

    int read = fread(buf, 1, headers[i].length, stream);
    printf("read %d bytes from %s\n", read, location);

    fwrite(buf, 1, headers[i].length, wstream);
    fclose(stream);
    free(buf);
  }

  fclose(wstream);
  free(data);

  printf("done\n");
  return 0;
}
