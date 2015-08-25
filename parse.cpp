#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//struct {

//}

int main(int argc, char* const argv[]) {
    printf("input %d %s\n", argc, argv[1]);

    if (argc == 2) {
        FILE* pFile = NULL;
        pFile = fopen(argv[1],"r");

        char* mystring;
        char myline[200];
        while(!feof(pFile)) {
            if (fgets(myline, 200, pFile) != NULL) {
                mystring = strtok(myline, "\0");
                //printf("%ld\n", strlen(mystring));
                for (int i=0; i<strlen(mystring); i++) {
                    if (strncmp(mystring,"南港",6) == 0) {
                        puts(mystring);
                        break;
                    }
                }
                puts(mystring);
            }
        }

        fclose(pFile);
    }
    return 0;
}
