#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATAOFFSET 66817
#define BITMAPOFFSET 1281 //teraz mamy BajtMape - ka�da [ja] reprezentowana przez 1Bajt w mapie zaj�to�ci: 1-zajeta, 0-wolna

#define BITMAPSIZE 65536 //64KB
//#define ALLENCRYPTORSSIZE 1280
#define DATASIZE 4194304
#define ENCRYPTORSIZE 20
#define ALLOCATIONSIZE 64

void wypiszBitMape(char* bitmap){
    printf("BITMAPA: ");
    for(int i=0; i<BITMAPSIZE; ++i) printf("%d", bitmap[i]);
    printf("\n");
}
void clearEndLine(char *name){
    for(int i=0; i<15; ++i){
        if(name[i]=='\n'){
            name[i] = 0;
            break;
        }
    }
}

char *readFromUserFile(char *name, int *length) {
    /*char name[30];
    printf("Podaj nazwe pliku: ");
    scanf("%s", name);*/
    FILE *file = fopen(name, "rb");
    if(file==NULL) {
        printf("Nie moge otworzyc takiego pliku!\n");
        return NULL;
    }
    fseek(file, 0, 2); //przesuniecie na koniec pliku wskaznika
    fpos_t pos;
    fgetpos(file, &pos);
    *length = pos.__pos;
    fseek(file, 0, 0);//przesuniecie na poczatek pliku wskaznika
    char *content = malloc(sizeof(char)* (*length));
    fread(content, sizeof(char), *length, file);
    fclose(file);
    return content;
}

int writeToUserFile(char *name, int length, char *content){
    FILE *file = fopen(name, "wb");
    if(file==NULL) {
        printf("Nie moge otworzyc takiego pliku!\n");
        return 1;
    }
    //printf("Zapisuje na dysku ubuntu plik: %s, size: %d, dane:\n%s\n", name, length, content);
    fwrite(content, sizeof(char), length, file);
    fclose(file);
    return 0;
}

void makeEmptyDisk() {
    //Jednostka alokacji (ja) = 64B
    //partycja zawiera 2^16=65336=64Kja  == 4096KB=4MB
    FILE *disk = fopen("MyVirtualDisc", "wb");
    char zero[64]; //64B - do zerowania danych
    for(int i=0; i<64; ++i) zero[i]=0;
    char z=0; //1B
    char encryptor[20]; //20B
    for(int i=0; i<20; ++i) encryptor[i]=0;


    for(int i=0; i<65536; ++i) fwrite(zero, sizeof(char), 64, disk); //dane = 4096KB = 4194304B
    for(int i=0; i<65537; ++i) fwrite(&z, sizeof(char), 1, disk); //bitmapa + ilosc_plikow = 64KB + 1B =65537B
    for(int i=0; i<64; ++i) fwrite(encryptor, sizeof(char), 20, disk); //enktyptory_plikow = 1280B
    //razem rozmiar: 4261121B
    fclose(disk);
}

int addFileToDisk(char *content, int length, char *fileName) { //return: 2-error open disk, 3-disk is full, 1-fragmetnacja zewnetrzna-nie ma miejsca, 0-udalo sie dodac
    FILE *disk = fopen("MyVirtualDisc", "r+b");
    if(disk==NULL) {
        printf("Nie udalo sie otworzyc dysku!\n");
        return 2;
    }

    char fileQuantity;
    char readName[15];
    char bitmap[BITMAPSIZE];

    fseek(disk, 0, 0);
    fread(&fileQuantity, sizeof(char), 1, disk);
    if((int)fileQuantity>=64) {
        printf("Dysk pelny!\n");
        fclose(disk);
        return 3;
    }
    
    for(int i=0; i<64; ++i){ //sprawdzenie niepowtarzalnosci nazwy pliku
        fseek(disk, 1+i*ENCRYPTORSIZE, 0);
        fread(readName, sizeof(char), 15, disk);
        //printf("%s, ", readName);
        if(strcmp(readName, fileName)==0){
            printf("Plik o takiej nazwie znaduje sie juz na dysku!");
            return 4;
        }
    }
    
    fseek(disk, BITMAPOFFSET, 0);
    int r = fread(bitmap, sizeof(char), BITMAPSIZE, disk); //odczyt bitmapy
    //printf("ILOSC WCZYTANYCH BITMAPA: %d\n", r);
    //wypiszBitMape(bitmap);

    //znalezienie odpowiedniego fragmentu rozmiarowo - First Fit
    unsigned int bitmapOffset=0, consistent=0, starthole=0, rightPlace=0;
    for(int i=0; i<BITMAPSIZE; ++i){
        if(bitmap[i]==0 && starthole==0){//poczatek dziory
            starthole=1;
            bitmapOffset=i;
            consistent=1;
            if(consistent*ALLOCATIONSIZE>=length){ // wystarczajaco miejsca w tej dziorze
                rightPlace=1;
                break;
            }
        }else if(bitmap[i]==0 && starthole!=0){ //kontynuacja dziory
            consistent++;
            if(consistent*ALLOCATIONSIZE>=length){ // wystarczajaco miejsca w tej dziorze
                rightPlace=1;
                break;
            }
        }else if(bitmap[i]==1){ //zajete
            starthole=0;
        }
    }
    if(rightPlace==0){ //nie znalezniono odpowiedniej dziory
        printf("Nie ma miejsca...\n");
        return 1;
    }
    //znalazlem miejsce dla danych
    fseek(disk, DATAOFFSET+bitmapOffset*ALLOCATIONSIZE, 0);//ustawienie wskaznika w miejscu gdzie bede pisal dane;
    fwrite(content, sizeof(char), length, disk); //zapisanie danych
    //printf("DANE: %s\n", content);

    for(int i=bitmapOffset; i<consistent+bitmapOffset; ++i) bitmap[i]=1;
    //wypiszBitMape(bitmap);

    fseek(disk, BITMAPOFFSET, 0);
    fwrite(bitmap, sizeof(char), BITMAPSIZE, disk); //zmiana bitmapy

    fileQuantity++;
    fseek(disk, 0, 0);
    fwrite(&fileQuantity, sizeof(char), 1, disk);//zwiekszenie ilosci plikow
    //printf("\nILOSC PLIKOW: %d\n", fileQuantity);

    //dodanie enkryptora - ustawienie go
    char flags, name[15];
    unsigned short offset, size;
    int i=0;
    fseek(disk, 1, 0);
    for(i=0; i<64; ++i){
        //fseek(disk, 1+ENCRYPTORSIZE*i, 0);
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((unsigned short)flags==0){ //pusty enkryptor
            break;
        }
    }
    fseek(disk, 1+ENCRYPTORSIZE*i, 0);
    offset = bitmapOffset;
    size = length;
    flags = 1;
    fwrite(fileName, sizeof(char), 15, disk);
    fwrite(&offset, sizeof(unsigned short), 1, disk);
    fwrite(&size, sizeof(unsigned short), 1, disk);
    fwrite(&flags, sizeof(char), 1, disk);
    //printf("ENKRYPTOR: %s, %u, %u, %d\n", fileName, offset, size, flags);

    fclose(disk);
    return 0;
}

int removeFileFromDisk(char *fileName) { //fileName to char[15] ///TODO: dopisac zmiane bitmapy
    FILE *disk = fopen("MyVirtualDisc", "r+b");
    if(disk==NULL) {
        printf("Nie udalo sie otworzyc dysku!\n");
        return 2;
    }
    char fileQuantity, flags, name[15], *content;char bitmap[BITMAPSIZE];
    unsigned short offset, size;
    int foundFile = 0, encryptorNumber=0;

    fread(&fileQuantity, sizeof(char), 1, disk);
    if((int)fileQuantity == 0) {
        printf("Pusty dysk!\n");
        return 2;
    }

    for(int i=0; i<64; ++i) {
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((int)flags==1 && strcmp(fileName, name)==0) { //znalazlem enkrytptor danego pliku
            foundFile=1;
            break;
        }
        encryptorNumber++;
    }
    if(foundFile==0) {
        printf("Nie ma takiego pliku na dysku!\n");
        return 2;
    }
    //mam dane o pliku w zmiennych
    //wyzerowac enkryptor, wyzerowac bajty BitMapy, zmniejszyc ilosc plikow
    char zeroEncryptor[20];
    for(int i=0; i<20; ++i) zeroEncryptor[i]=0;
    fseek(disk, 1+encryptorNumber*20, 0);
    fwrite(zeroEncryptor, sizeof(char), 20, disk); //zerowanie enkryptora

    fseek(disk, 0, 0);
    fileQuantity--;
    fwrite(&fileQuantity, sizeof(char), 1, disk); //zmniejszenie ilosc plikow

    fseek(disk, BITMAPOFFSET, 0);
    //char bitmap[BITMAPSIZE];
    fread(bitmap, sizeof(char), BITMAPSIZE, disk);
    int bitmapLength;
    if(size%ALLOCATIONSIZE==0)
        bitmapLength = size/ALLOCATIONSIZE;
    else
        bitmapLength = size/ALLOCATIONSIZE+1;
    for(int i=offset; i<bitmapLength; ++i) bitmap[i]=0;
    fseek(disk, BITMAPOFFSET, 0);
    fwrite(bitmap, sizeof(char), BITMAPSIZE, disk); //zapisanie nowej bitmapy

    fclose(disk);
    return 0;
}

char * readFileFromDisk(char *searchName, int *fileSize) { //przekazana nazwa ma byc char[15]; do wskaznika content zostanie wstwiony plik, do size bedzie wstawiony rozmiar pliku
    //content = NULL; //jesli nie znaleziono to content==NULL oraz fileSize==0
    *fileSize=0;
    FILE *disk = fopen("MyVirtualDisc", "r+b");
    if(disk==NULL) {
        printf("Nie udalo sie otworzyc dysku!\n");
        return NULL;
    }
    char fileQuantity, flags, name[15];
    unsigned short offset, size;
    char *content;

    fread(&fileQuantity, sizeof(char), 1, disk);
    if((int)fileQuantity == 0) {
        printf("Pusty dysk!\n");
        return NULL;
    }
    fseek(disk, 1, 0);
    for(int i=0; i<64; ++i) {
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((int)flags==1 && strcmp(searchName, name)==0) { //znalazlem enkrytptor danego pliku
            //printf("Znalazlem plik. Rozmiar: %d, Offset: %d\n", size, offset);
            content = malloc(sizeof(char)*size); //alokacja pamieci na dane
            fseek(disk, DATAOFFSET+offset*ALLOCATIONSIZE, 0);
            fread(content, sizeof(char), size, disk);
            *fileSize = size;
            //printf("INSIDE: Name: %s, Size: %d, Data:\n%s\n", name, size, content);
            fclose(disk);
            return content;
        }
    }
    fclose(disk);
    return NULL;
}

void printFilesInDisk() {
    FILE *disk = fopen("MyVirtualDisc", "rb");
    if(disk==NULL) {
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
    if((int)fileQuantity==0) { //dysk pusty
        fclose(disk);
        return;
    }
    for(int i=0; i<64; ++i) {
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((int)flags==1) { //plik istnieje (enkryptor) - najstarszy bit flagi=1 (flags>=128)
            printf("Name: %s Size: %uB, Offset: %u\n", name, size, offset);
        }
    }
    fclose(disk);
}

void printBitMap(){
    FILE *disk = fopen("MyVirtualDisc", "rb");
    if(disk==NULL) {
        printf("Nie udalo sie otworzyc dysku!\n");
        return;
    }
    char bitmap[BITMAPSIZE];
    fseek(disk, BITMAPOFFSET, 0);
    fread(bitmap, sizeof(char), BITMAPSIZE, disk);
    printf("Bitmapa: ");
    for(int i=0; i< BITMAPSIZE; ++i) printf("%d", bitmap[i]);
    printf("\n");
    fclose(disk);
}

void printFileFromDisk(char *fileName){
    FILE *disk = fopen("MyVirtualDisc", "rb");
    if(disk==NULL) {
        printf("Nie udalo sie otworzyc dysku!\n");
        return;
    }
    char fileQuantity;
    char name[15];
    unsigned short offset;
    unsigned short size;
    char flags;

    fread(&fileQuantity, sizeof(char), 1, disk);
    //printf("Ilosc plikow na dysku: %d\n", (int)fileQuantity);
    if((int)fileQuantity==0) { //dysk pusty
        fclose(disk);
        return;
    }
    for(int i=0; i<64; ++i) {
        fread(name, sizeof(char), 15, disk);
        fread(&offset, sizeof(unsigned short), 1, disk);
        fread(&size, sizeof(unsigned short), 1, disk);
        fread(&flags, sizeof(char), 1, disk);
        if((int)flags==1 && strcmp(name, fileName)==0) { //plik istnieje (enkryptor)
            printf("Name: %s Size: %uB, Offset: %u\n", name, size, offset);
            char *content = malloc(sizeof(char)*size);
            fseek(disk, DATAOFFSET+offset*ALLOCATIONSIZE, 0);
            fread(content, sizeof(char), size, disk);
            printf("DANE:\n%s\n", content);
            free(content);
            break;
        }
    }
    fclose(disk);
}
/////////////////////////////////////////////////////////////////
void comCreateDisk(){
    makeEmptyDisk();
    printf("Stworzono pusty dysk wirtualny.\n");
}
void comCopyFileToDisk(){
    printf("Podaj nazwe pliku ktory chcesz dodac (skopiowac) do dysku wirtualnego: ");
    char name[15];
    scanf("%s", name);
    clearEndLine(name);//////////////dodane
    int length;
    char *content = readFromUserFile(name, &length);
    if(content==NULL){
        printf("Nie mozna otwrzyc pliku o nazwie: %s", name);
        return;
    }
    //printf("PLIK size: %d, dane: %s\n", length, content);
    int result = addFileToDisk(content, length, name);
    free(content);
    if(result == 0)
        printf("Dodano plik\n");
}
void comCopyFileFromDisk(){
    printf("Podaj nazwe pliku ktory chcesz wyjac (skopiowac) z dysku wirtualnego: ");
    char name[15];
    scanf("%s", name);
    clearEndLine(name);
    char *content;
    int fileSize;
    content = readFileFromDisk(name, &fileSize);
    if(content==NULL){
        printf("ERROR\n");
        return;
    }
    //printf("OUTSIDE: Odczytalem dane z dysku wirtualnego.\nName: %s, Size: %d, Dane: %s\n", name, fileSize, content);
    if(writeToUserFile(name, fileSize, content)==0){
        printf("Udalo sie skopiowac\n");
    }
    free(content);
}
void comRemoveFile(){
    printf("Podaj nazwe pliku ktory chcesz usunac z dysku wirtualnego: ");
    char name[15];
    scanf("%s", name);
    clearEndLine(name);
    if(removeFileFromDisk(name)==0)
        printf("Usunieto plik.\n");
}
void comShowFileFromDisk(){
    printf("Podaj nazwe pliku z VD ktory chcesz wyswietlic: ");
    char name[15];
    scanf("%s", name);
    clearEndLine(name);
    printFileFromDisk(name);
}
int main(int argc, char *argv[]) {
    /*makeEmptyDisk();
    printFilesInDisk();
    printBitMap();*/
    if(argc>=2){
        int option = atoi(argv[1]);
        if(option==1) comCreateDisk();
        else if(option==2) comCopyFileToDisk();
        else if(option==3) comCopyFileFromDisk();
        else if(option==4) printFilesInDisk();
        else if(option==5) comRemoveFile();
        else if(option==6) printBitMap();
        else if(option==7) comShowFileFromDisk(); 
        return 0;
    }

    int whatDo=1;
    while(whatDo){
        printf("\n\n1. Stworzy pusty dysk wirtualny\n2. Kopiowanie pliku do dysku wirtualnego\n3. Kopiowanie pliku z dysku wirtualnego\n");
        printf("4. Wyświetlanie katalogu dysku wirtualnego\n5. Usuwniecie pliku z wirtualnego dysku\n6. Wyswietlenie bitmapy zajetosci\n7. Wypisz plik z VD\n");
        printf("0. Zakoncz\n");
        scanf("%d", &whatDo);
        if(whatDo==1) comCreateDisk();
        else if(whatDo==2) comCopyFileToDisk();
        else if(whatDo==3) comCopyFileFromDisk();
        else if(whatDo==4) printFilesInDisk();
        else if(whatDo==5) comRemoveFile();
        else if(whatDo==6) printBitMap();
        else if(whatDo==7) comShowFileFromDisk();
    }

}
