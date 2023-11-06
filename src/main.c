#include "dactylichexameter.h"
#define NOB_IMPLEMENTATION
#include "nob.h"

int main(void) {
    char sentence[128] = {0};
    char yesno[16] = {0};
    Nob_String_Builder sbElision = {0};
    Nob_String_Builder sbNumbers = {0};
    Nob_String_Builder sbScan = {0};
    Nob_String_Builder sbStrippedLine = {0};
    for (;;) {
        // Empty all the char buffers
        memset(yesno, 0, 16);
        memset(sentence, 0, 128);

        // Get the verse to scan
        printf("Intrare versum: ");
        fgets(sentence, 128, stdin);

        printf("\n");

        // Perform elision
        if (!dhElision(sentence, &sbElision)) return false;
        nob_sb_append_null(&sbElision);
        printf("Elision: %s\n", sbElision.items);

        printf("\n");

        // Scan the verse
        if (!dhScan(sbElision.items, &sbNumbers, &sbScan, &sbStrippedLine)) return false;
        nob_sb_append_null(&sbNumbers);
        nob_sb_append_null(&sbScan);
        nob_sb_append_null(&sbStrippedLine);
        printf("%s\n", sbNumbers.items);
        printf("%s\n", sbScan.items);
        printf("%s\n", sbStrippedLine.items);
        
        printf("\n");

        printf("Do you want to scan another verse? [Y/n] ");
        fgets(yesno, 16, stdin);
        if (tolower(yesno[0]) == 'n')
            break;
    }

    nob_sb_free(sbElision);
    nob_sb_free(sbNumbers);
    nob_sb_free(sbScan);
    nob_sb_free(sbStrippedLine);
    return 0;
}