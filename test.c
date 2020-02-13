#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
int main() {
	char s[100] = "aaaaaaaaaaaaaaaaaaaa";
	printf("%s\n", s);
	// char *t = strcpy(s, "bb");
	memcpy(s, "bb", 3);

	// printf("%d\n", sizeof(t));
	
	printf("%s\n", s);

	int b = 10, c = 20;
	const int *a =  &b;
	a = &c;
	printf("%d\n", *a);
	return 0;
}