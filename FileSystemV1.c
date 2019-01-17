#include <stdio.h>
#include <stdlib.h>

char *readFromUserFile(){
    char name[30];
    printf("Podaj nazwe pliku: ");
    scanf("%s", name);
    FILE *file = fopen(name, "rb");
    if(file==NULL){
        printf("Nie moge otworzyc takiego pliku!\n");
        return NULL;
    }
    fseek(file, 0, 2); //przesuniecie na koniec pliku wskaznika
    fpos_t pos;
    fgetpos(file, &pos);
    int length = pos.__pos;
    fseek(file, 0, 0);//przesuniecie na poczatek pliku wskaznika
    char *content = malloc(sizeof(char)*length);
    fread(content, sizeof(char), length, file);
    fclose(file);
    return content;
}

void makeEmptyDisk(){
    //Jednostka alokacji (ja) = 64B
    //partycja zawiera 2^16=65336=64Kja  == 4096KB=4MB
    FILE *disk = fopen("MyVirtualDisc", "wb");
    char zero[64]; //64B
    for(int i=0; i<64; ++i) zero[i]=0;
    char z=0; //1B
    char encryptor[20]; //20B
    for(int i=0; i<20; ++i) encryptor[i]=0;


    for(int i=0; i<65536; ++i) fwrite(zero, sizeof(char), 64, disk); //dane = 4096KB = 4194304B
    for(int i=0; i<8192; ++i) fwrite(&z, sizeof(char), 1, disk); //bitmapa + ilosc_plikow = 64Kb + 1B = 8KB+1B=8192B
    for(int i=0; i<64; ++i) fwrite(encryptor, sizeof(char), 20, disk); //enktyptory_plikow = 1280B
    //razem rozmiar: 4203776B
    fclose(disk);
}
int addFileToDisk(char *content, int length, char *fileName){ //napisac!!!!!
    FILE *disk = fopen("MyVirtualDisc", "wb");
    if(disk==NULL){
        printf("Nie udalo sie otworzyc dysku!\n");
        return 2;
    }


    char fileQuantity;
    char allEncryptors[1280];
    char encryptor[20];
    char bitmap[8192];

    fread(&fileQuantity, sizeof(char), 1, disk);
    if(fileQuantity>=64){
        printf("Dysk pelny!\n");
        fclose(disk);
        return 3;
    }
    fread(allEncryptors, sizeof(char), 1280, disk);
}

char *readFileFromDisk(char *name){

}
void printFilesInDisk(){
    FILE *disk = fopen("MyVirtualDisc", "wb");
    if(disk==NULL){
        printf("Nie udalo sie otworzyc dysku!\n");
        return;
    }
    char fileQuantity;
    char name[15];
    unsigned short offset;
    unsigned short size;
    char flags;

    fread(&fileQuantity, sizeof(char), 1, disk);
    printf("Ilosc plikow na dysku: %d\n", (int)fileQuantity);
    if((int)fileQuantity==0){ //dysk pusty
        fclose(disk);
        return;
    }
    for(int i=0; i<64; ++i){
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((int)flags>=128){ //plik istnieje (enkryptor) - najstarszy bit flagi=1 (flags>=128)
            printf("Name: %s Size: %uB\n", name, size);
        }
    }
    fclose(disk);
}
int main(){
    /*char *c = readFromUserFile();
    printf("%s", c);
    if(c!=NULL)
        free(c);*/
    makeEmptyDisk();
    printFilesInDisk();

}