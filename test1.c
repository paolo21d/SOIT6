#include <stdio.h>
#include <stdlib.h>

int main(){
    char name [30] = "plik.txt";
    FILE *file;
    //scanf("%s", name);

    if ((file=fopen(name, "rb"))==NULL) {
        printf ("Nie mogę otworzyć pliku test.txt do zapisu!\n");
        exit(1);
    }
    fpos_t length;
    fseek(file, 0 , SEEK_END);
    fgetpos(file, &length);
    int l = length.__pos;
    char *content = malloc(sizeof (char) * l);
    fseek(file, 0, 0);
    fread(content, sizeof(char), l, file);
    
    printf("Dlugosc plik: %d \n", l);
    printf("%s", content);

    FILE *fileWrite;
    fileWrite = fopen("zapis", "wb");
    fwrite(content, sizeof(char), l, fileWrite);

    fclose(file);
    fclose(fileWrite);
}