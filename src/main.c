#include <stdio.h>
#include "VCParser.h"

int main() {
    Card* testCard;
    VCardErrorCode error;

    error = createCard("../testCard.vcf", &testCard);
    error = validateCard(testCard); 
    if (error == OK) {
        writeCard("testOut.vcf", testCard);
        char* cardString = cardToString(testCard);
        printf("%s\n", cardString);
        free(cardString);
        deleteCard(testCard);
    } else {
        char* error_string = errorToString(error);
        printf("%s\n", error_string);
        free(error_string);
    }
}