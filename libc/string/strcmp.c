#include <string.h>

int strcmp(char *s1, char *s2) {
  size_t i = 0;
  int status = 0;

  while(s1[i] != '\0' && s2[i] != '\0') {
	if(s1[i] != s2[i]) {
	  status = 1;
	  break;
	}
	i++;
  }

  if((s1[i] == '\0' && s2[i] != '\0') || (s1[i] != '\0' && s2[i] == '\0')) {
	status = 1;
  }

  return status;
}
