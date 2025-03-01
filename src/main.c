#include <stdio.h>
#include "VCParser.h"

int main() {
    Card* testCard;
    VCardErrorCode error;

    error = createCard("../testCard.vcf", &testCard);
    if (error == OK) {
        char* cardString = cardToString(testCard);
        printf("%s\n", cardString);
        //writeCard("testOut.vcf", testCard);
        free(cardString);
        deleteCard(testCard);
    } else {
        char* error_string = errorToString(error);
        printf("%s\n", error_string);
        free(error_string);
    }
}