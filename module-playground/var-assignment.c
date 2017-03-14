#include "stdio.h"
int main() {
    char *char16bits = malloc(2);
    char char8bits = 0xFF;
    
    int i;
    printf("Uninitialized char16bits = ");
    for (i = 0; i < 2; i++) {
	print("%x ", char16bits[i]);
    }
    print("\n");
}
