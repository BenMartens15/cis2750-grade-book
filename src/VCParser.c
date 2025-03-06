// Author: Ben Martens (1349551)

#define _GNU_SOURCE
#include "VCParser.h"

static ssize_t readNextLine(char** currentLine, char** nextLine, FILE* fp, size_t* len);
static Property* createProperty(Card* card, const char* currentLine);
static void parsePropertyValues(List* valueList, const char* name, char* valueString);
static DateTime* createDateTime(char* inputString);
static bool validateDateTime(DateTime* dateTime);

// ************* Card parser ***********************************************
VCardErrorCode createCard(char* fileName, Card** obj) {
    VCardErrorCode error = OK;
    FILE* fp;
    char* currentLine = NULL;
    char* nextLine = NULL;
    size_t len = 0;
    Card* newCard = NULL;

    if (obj == NULL) {
        return OTHER_ERROR;
    }

    if (fileName == NULL) {
        return INV_FILE;
    }

    // check the file extension
    char* extension = strrchr(fileName, '.');
    if (extension == NULL || (strcmp(extension, ".vcf") != 0 && strcmp(extension, ".vcard") != 0)) {
        return INV_FILE;
    }

    fp = fopen(fileName, "r");
    if (fp == NULL) {
        return INV_FILE;
    }

    *obj = (Card*)malloc(sizeof(Card));
    newCard = *obj;
    newCard->fn = NULL;
    newCard->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);
    newCard->birthday = NULL;
    newCard->anniversary = NULL;

    // read the first line and make sure it is the BEGIN:VCARD property
    if (readNextLine(&currentLine, &nextLine, fp, &len) == -1) {
        error = INV_PROP;
        goto EXIT;
    }
    if (strcasecmp(currentLine, "BEGIN:VCARD") != 0) {
        error = INV_CARD;
        goto EXIT;
    }

    // read the second line and make sure it is the VERSION:4.0 property
    if (readNextLine(&currentLine, &nextLine, fp, &len) == -1) {
        error = INV_PROP;
        goto EXIT;
    }
    if (strcasecmp(currentLine, "VERSION:4.0") != 0) {
        error = INV_CARD;
        goto EXIT;
    }

    // read the file line-by-line (unfolding any folded lines)
    while (readNextLine(&currentLine, &nextLine, fp, &len) != -1) {
        if (createProperty(newCard, currentLine) == NULL) {
            error = INV_PROP;
            goto EXIT;
        }
    }

    // make sure the file contains the FN property
    if (newCard->fn == NULL) {
        error = INV_CARD;
        goto EXIT;
    }

    // make sure the file ends with the END:VCARD property
    if (strcasecmp(currentLine, "END:VCARD") != 0) {
        error = INV_CARD;
        goto EXIT;
    }

EXIT:
    if (error != OK) {
        deleteCard(newCard);
        newCard = NULL;
    }
    if (fp) {
        fclose(fp);
    }
    free(currentLine);
    free(nextLine);
    return error;
}

void deleteCard(Card* obj) {
    if (obj == NULL) {
        return;
    }

    deleteProperty(obj->fn);
    deleteDate(obj->birthday);
    deleteDate(obj->anniversary);
    freeList(obj->optionalProperties);
    free(obj);
}

char* cardToString(const Card* obj) {
    char* cardString = NULL;
    char* fullName = NULL;
    char* birthday = NULL;
    char* anniversary = NULL;
    char* optionalProperties = NULL;

    if (obj == NULL) {
        cardString = (char*)malloc(5);
        strcpy(cardString, "null");
        return cardString;
    }

    fullName = propertyToString(obj->fn);
    birthday = dateToString(obj->birthday);
    anniversary = dateToString(obj->anniversary);
    optionalProperties = toString(obj->optionalProperties);

    cardString = (char*)malloc(strlen(fullName) + 1);
    strcpy(cardString, fullName);
    if (birthday) {
        if (obj->birthday->isText) {
            cardString = (char*)realloc(cardString, strlen(cardString) + 16 + strlen(birthday) + 1);
            strcat(cardString, "BDAY;VALUE=text:");
            strcat(cardString, birthday);
        } else {
            cardString = (char*)realloc(cardString, strlen(cardString) + 5 + strlen(birthday) + 1);
            strcat(cardString, "BDAY:");
            strcat(cardString, birthday);
        }
    }
    if (anniversary) {
        if (obj->anniversary->isText) {
            cardString = (char*)realloc(cardString, strlen(cardString) + 23 + strlen(anniversary) + 1);
            strcat(cardString, "ANNIVERSARY;VALUE=text:");
            strcat(cardString, anniversary);
        } else {
            cardString = (char*)realloc(cardString, strlen(cardString) + 12 + strlen(anniversary) + 1);
            strcat(cardString, "ANNIVERSARY:");
            strcat(cardString, anniversary);
        }
    }
    cardString = (char*)realloc(cardString, strlen(cardString) + strlen(optionalProperties) + 1);
    strcat(cardString, optionalProperties);

    free(fullName);
    free(birthday);
    free(anniversary);
    free(optionalProperties); 

    return cardString;
}

char* errorToString(VCardErrorCode err) {
    char* err_string = NULL;

    switch (err)
    {
    case OK:
        err_string = (char*)malloc(3);
        strcpy(err_string, "OK");
        break;
    case INV_FILE:
        err_string = (char*)malloc(9);
        strcpy(err_string, "INV_FILE");
        break;
    case INV_CARD:
        err_string = (char*)malloc(9);
        strcpy(err_string, "INV_CARD");
        break;
    case INV_PROP:
        err_string = (char*)malloc(9);
        strcpy(err_string, "INV_PROP");
        break;
    case INV_DT:
        err_string = (char*)malloc(7);
        strcpy(err_string, "INV_DT");
        break;
    case WRITE_ERROR:
        err_string = (char*)malloc(12);
        strcpy(err_string, "WRITE_ERROR");
        break;
    case OTHER_ERROR:
        err_string = (char*)malloc(12);
        strcpy(err_string, "OTHER_ERROR");
        break;   
    default:
        err_string = (char*)malloc(19);
        strcpy(err_string, "Invalid error code");
        break;
    }
    
    return err_string;
}

VCardErrorCode writeCard(const char* fileName, const Card* obj) {
    FILE* fp;
    char* cardString = NULL;
    char* fullName = NULL;
    char* birthday = NULL;
    char* anniversary = NULL;
    char* optionalProperties = NULL;
    
    if (obj == NULL) {
        return WRITE_ERROR;
    }

    if (fileName == NULL) {
        return WRITE_ERROR;
    }

    // check the file extension
    char* extension = strrchr(fileName, '.');
    if (extension == NULL || (strcmp(extension, ".vcf") != 0 && strcmp(extension, ".vcard") != 0)) {
        return WRITE_ERROR;
    }

    fp = fopen(fileName, "w");
    if (fp == NULL) {
        return WRITE_ERROR;
    }

    fullName = propertyToString(obj->fn);
    birthday = dateToString(obj->birthday);
    anniversary = dateToString(obj->anniversary);
    optionalProperties = toString(obj->optionalProperties);

    cardString = (char*)malloc(27);
    strcpy(cardString, "BEGIN:VCARD\r\nVERSION:4.0\r\n");

    cardString = (char*)realloc(cardString, strlen(cardString) + strlen(fullName) + 1);
    strcat(cardString, fullName);
    if (birthday) {
        if (obj->birthday->isText) {
            cardString = (char*)realloc(cardString, strlen(cardString) + 16 + strlen(birthday) + 1);
            strcat(cardString, "BDAY;VALUE=text:");
            strcat(cardString, birthday);
        } else {
            cardString = (char*)realloc(cardString, strlen(cardString) + 5 + strlen(birthday) + 1);
            strcat(cardString, "BDAY:");
            strcat(cardString, birthday);
        }
        cardString[strlen(cardString) - 1] = '\r'; // replace the newline character with a carriage return
        cardString = (char*)realloc(cardString, strlen(cardString) + 2); // make room for the newline character
        strcat(cardString, "\n"); // add back the newline character after the carriage return
    }
    if (anniversary) {
        if (obj->anniversary->isText) {
            cardString = (char*)realloc(cardString, strlen(cardString) + 23 + strlen(anniversary) + 1);
            strcat(cardString, "ANNIVERSARY;VALUE=text:");
            strcat(cardString, anniversary);
        } else {
            cardString = (char*)realloc(cardString, strlen(cardString) + 12 + strlen(anniversary) + 1);
            strcat(cardString, "ANNIVERSARY:");
            strcat(cardString, anniversary);
        }
        cardString[strlen(cardString) - 1] = '\r'; // replace the newline character with a carriage return
        cardString = (char*)realloc(cardString, strlen(cardString) + 2); // make room for the newline character
        strcat(cardString, "\n"); // add back the newline character after the carriage return
    }
    cardString = (char*)realloc(cardString, strlen(cardString) + strlen(optionalProperties) + 1);
    strcat(cardString, optionalProperties);

    cardString = (char*)realloc(cardString, strlen(cardString) + 12); // make room for "END:VCARD\r\n"
    strcat(cardString, "END:VCARD\r\n");

    fputs(cardString, fp); // write the string to the file

    free(fullName);
    free(birthday);
    free(anniversary);
    free(optionalProperties);
    free(cardString);
    fclose(fp);

    return OK;
}

VCardErrorCode validateCard(const Card* obj) {
    if (obj == NULL ||
            obj->fn == NULL ||
            obj->optionalProperties == NULL) {
        return INV_CARD;
    }

    if (obj->birthday != NULL && !validateDateTime(obj->birthday)) {
        return INV_DT;
    }

    if (obj->anniversary != NULL && !validateDateTime(obj->anniversary)) {
        return INV_DT;
    }

    // counters for all properties with cardinality of *1 (exactly one instance may be present)
    int kindCounter = 0;
    int nCounter = 0;
    int genderCounter = 0;
    int prodidCounter = 0;
    int revCounter = 0;
    int uidCounter = 0;

    void* propElement;
    ListIterator propertyIterator = createIterator(obj->optionalProperties);
    while ((propElement = nextElement(&propertyIterator)) != NULL) {
        Property* property = (Property*)propElement;

        // make sure all required properties are present
        if (property->name == NULL ||
                property->group == NULL ||
                property->parameters == NULL ||
                property->values == NULL) {
            return INV_PROP;
        }

        // make sure no parameters are empty strings
        void* paramElement;
        ListIterator paramIter = createIterator(property->parameters);
        while((paramElement = nextElement(&paramIter)) != NULL) {
            Parameter* param = (Parameter*)paramElement;
            if (param->name == NULL || param->value == NULL || strlen(param->name) == 0 || strlen(param->value) == 0) {
                return INV_PROP;
            }
        }

        if (strcasecmp(property->name, "VERSION") == 0) {
            return INV_CARD;
        }

        if (strcasecmp(property->name, "BEGIN") == 0 || // all properties that have a cardinality of 1
                strcasecmp(property->name, "END") == 0 ||
                strcasecmp(property->name, "BDAY") == 0 ||
                strcasecmp(property->name, "ANNIVERSARY") == 0) {
            // already checked above, so if present in the optional properties, invalid
            return INV_PROP;
        } else if (strcasecmp(property->name, "SOURCE") == 0 || // all properties that require a single value
                strcasecmp(property->name, "XML") == 0 ||
                strcasecmp(property->name, "FN") == 0 ||
                strcasecmp(property->name, "NICKNAME") == 0 ||
                strcasecmp(property->name, "PHOTO") == 0 ||
                strcasecmp(property->name, "EMAIL") == 0 ||
                strcasecmp(property->name, "IMPP") == 0 ||
                strcasecmp(property->name, "LANG") == 0 ||
                strcasecmp(property->name, "TZ") == 0 ||
                strcasecmp(property->name, "GEO") == 0 ||
                strcasecmp(property->name, "TITLE") == 0 ||
                strcasecmp(property->name, "ROLE") == 0 ||
                strcasecmp(property->name, "LOGO") == 0 ||
                strcasecmp(property->name, "MEMBER") == 0 ||
                strcasecmp(property->name, "RELATED") == 0 ||
                strcasecmp(property->name, "CATEGORIES") == 0 ||
                strcasecmp(property->name, "NOTE") == 0 ||
                strcasecmp(property->name, "SOUND") == 0 ||
                strcasecmp(property->name, "URL") == 0 ||
                strcasecmp(property->name, "KEY") == 0 ||
                strcasecmp(property->name, "FBURL") == 0 ||
                strcasecmp(property->name, "CALADRURI") == 0 ||
                strcasecmp(property->name, "CALURI") == 0) {
            if (getLength(property->values) != 1) {
                return INV_PROP;
            }
        } else if (strcasecmp(property->name, "KIND") == 0) {
            if (kindCounter > 0 || getLength(property->values) != 1) {
                return INV_PROP;
            }
            kindCounter++;
        } else if (strcasecmp(property->name, "PRODID") == 0) {
            if (prodidCounter > 0 || getLength(property->values) != 1) {
                return INV_PROP;
            }
            prodidCounter++;
        } else if (strcasecmp(property->name, "REV") == 0) {
            if (revCounter > 0 || getLength(property->values) != 1) {
                return INV_PROP;
            }
            revCounter++;
        } else if (strcasecmp(property->name, "UID") == 0) {
            if (uidCounter > 0 || getLength(property->values) != 1) {
                return INV_PROP;
            }
            uidCounter++;
        } else if (strcasecmp(property->name, "GENDER") == 0) {
            if (genderCounter > 0) {
                return INV_PROP;
            }
            genderCounter++;
            if (getLength(property->values) != 1 && getLength(property->values) != 2) {
                return INV_PROP;
            }
        } else if (strcasecmp(property->name, "CLIENTPIDMAP") == 0) {
            if (getLength(property->values) != 2) {
                return INV_PROP;
            }
        }  else if (strcasecmp(property->name, "N") == 0) {
            if (nCounter > 0) {
                return INV_PROP;
            }
            nCounter++;
            if (getLength(property->values) != 5) {
                return INV_PROP;
            }
        } else if (strcasecmp(property->name, "ADR") == 0) {
            if (getLength(property->values) != 7) {
                return INV_PROP;
            }
        } else if (strcasecmp(property->name, "ORG") == 0 ||
                strcasecmp(property->name, "TEL") == 0) { // ORG and TEL can have 1 or more values
            if (getLength(property->values) == 0) {
                return INV_PROP;
            }
        } else { // property is not in the list of properties in 6.1 - 6.9.3 in the vCard specification
            return INV_PROP;
        }
    }

    return OK;
}
// *************************************************************************

// ************* List helper functions ************************************* 
void deleteProperty(void* toBeDeleted) {
    Property* property = NULL;

    if (toBeDeleted == NULL) {
        return;
    }
    
    property = (Property*)toBeDeleted;
    free(property->name);
    if (property->group && strlen(property->group) > 0) {
        free(property->group);
    }
    freeList(property->parameters);
    freeList(property->values);
    free(property);
}

int compareProperties(const void* first, const void* second) {
    Property* firstProperty = NULL;
    Property* secondProperty = NULL;
    int ret = 0;

    if (first == NULL || second == NULL) {
        return 1;
    }

    firstProperty = (Property*)first;
    secondProperty = (Property*)second;

    ret += strcasecmp(firstProperty->name, secondProperty->name);
    ret += strcasecmp(firstProperty->group, secondProperty->group);

    if (getLength(firstProperty->parameters) != getLength(secondProperty->parameters)) {
        ret++;
    }

    void* elemFirst;
    void* elemSecond;
    ListIterator iter1 = createIterator(firstProperty->parameters);
    ListIterator iter2 = createIterator(secondProperty->parameters);
    while ((elemFirst = nextElement(&iter1)) != NULL && (elemSecond = nextElement(&iter2)) != NULL) {
        Parameter* paramFirst = (Parameter*)elemFirst;
        Parameter* paramSecond = (Parameter*)elemSecond;
        ret += compareParameters(paramFirst, paramSecond);
    }

    if (getLength(firstProperty->values) != getLength(secondProperty->values)) {
        ret++;
    }

    elemFirst = NULL;
    elemSecond = NULL;
    iter1 = createIterator(firstProperty->values);
    iter2 = createIterator(secondProperty->values);
    while ((elemFirst = nextElement(&iter1)) != NULL && (elemSecond = nextElement(&iter2)) != NULL) {
        char* valueFirst = (char*)elemFirst;
        char* valueSecond = (char*)elemSecond;
        ret += compareValues(valueFirst, valueSecond);
    }

    return ret;
}

char* propertyToString(void* prop) {
    Property* property = NULL;
    char* propertyString = NULL;

    if (prop == NULL) {
        return NULL;
    }

    property = (Property*)prop;
    size_t length = strlen(property->name) + 1;

    propertyString = (char*)malloc(length + 1);
    snprintf(propertyString, length + 1, "%s", property->name);

    void* paramElem;
    ListIterator paramIter = createIterator(property->parameters);
    while ((paramElem = nextElement(&paramIter)) != NULL) {
        Parameter* param = (Parameter*)paramElem;
        size_t length = 1 + strlen(param->name) + 1 + strlen(param->value) + 1;
        propertyString = (char*)realloc(propertyString, strlen(propertyString) + length);
        strcat(propertyString, ";");
        strcat(propertyString, param->name);
        strcat(propertyString, "=");
        strcat(propertyString, param->value);
    }
    propertyString = (char*)realloc(propertyString, strlen(propertyString) + 2);
    strcat(propertyString, ":");

    void* valueElem;
    ListIterator valueIter = createIterator(property->values);
    while ((valueElem = nextElement(&valueIter)) != NULL) {
        char* value = (char*)valueElem;
        propertyString = (char*)realloc(propertyString, strlen(propertyString) + strlen(value) + 2);
        strcat(propertyString, property->values->printData(value));
        strcat(propertyString, ";");
    }
    propertyString = (char*)realloc(propertyString, strlen(propertyString) + 2); // make room for the \r
    propertyString[strlen(propertyString) - 1] = '\r'; // replace the last ';' with '\r' (just used for writing back to a file)
    strcat(propertyString, "\n"); // add the newline

    return propertyString;
}

void deleteParameter(void* toBeDeleted) {
    Parameter* param = NULL;
    
    if (toBeDeleted == NULL) {
        return;
    }

    param = (Parameter*)toBeDeleted;
    free(param->name);
    free(param->value);
    free(param);
}

int compareParameters(const void* first, const void* second) {
    Parameter* firstParam = NULL;
    Parameter* secondParam = NULL;
    int ret = 0;

    if (first == NULL || second == NULL) {
        return 1;
    }

    firstParam = (Parameter*)first;
    secondParam = (Parameter*)second;

    ret += strcasecmp(firstParam->name, secondParam->name);
    ret += strcasecmp(firstParam->value, secondParam->value);

    return ret;
}

char* parameterToString(void* param) {
    Parameter* parameter = NULL;
    char* parameterString = NULL;

    if (param == NULL) {
        return NULL;
    }

    parameter = (Parameter*)param;
    size_t length = strlen(parameter->name) + 1 + strlen(parameter->value) + 1;
    parameterString = (char*)malloc(length);
    snprintf(parameterString, length, "%s=%s", parameter->name, parameter->value);

    return parameterString;
}

void deleteValue(void* toBeDeleted) {
    char* value = (char*)toBeDeleted;
    free(value);
}

int compareValues(const void* first, const void* second) {
    char* firstValue = NULL;
    char* secondValue = NULL;
    int ret = 0;

    if (first == NULL || second == NULL) {
        return 1;
    }

    firstValue = (char*)first;
    secondValue = (char*)second;

    ret += strcmp(firstValue, secondValue);

    return ret;
}

char* valueToString(void* val) {
    char* outString = (char*)val;

    return outString;
}

void deleteDate(void* toBeDeleted) {
    DateTime* dateTime = NULL;

    if (toBeDeleted == NULL) {
        return;
    }

    dateTime = (DateTime*)toBeDeleted;
    if (dateTime->date[0] != '\0') {
        free(dateTime->date);
    }
    if (dateTime->time[0] != '\0') {
        free(dateTime->time);
    }
    if (dateTime->text[0] != '\0') {
        free(dateTime->text);
    }
    free(dateTime);
}

int compareDates(const void* first, const void* second) {
    return 0;
}

char* dateToString(void* date) {
    DateTime* dateTime = NULL;
    char* dateTimeString = NULL;

    if (date == NULL) {
        return NULL;
    }

    dateTime = (DateTime*)date;

    size_t length;
    if (dateTime->isText) {
        dateTimeString = (char*)malloc(strlen(dateTime->text) + 2);
        strcpy(dateTimeString, dateTime->text);
        strcat(dateTimeString, "\n");
    } else if (strlen(dateTime->time) > 0) {
        length = strlen(dateTime->date) + 1 + strlen(dateTime->time) + 1;
        dateTimeString = (char*)malloc(length + 1);
        snprintf(dateTimeString, length + 1, "%sT%s\n", dateTime->date, dateTime->time);
    } else {
        length = strlen(dateTime->date) + 1;
        dateTimeString = (char*)malloc(length + 1);
        snprintf(dateTimeString, length + 1, "%s\n", dateTime->date);
    }

    if (dateTime->UTC) {
        dateTimeString = (char*)realloc(dateTimeString, strlen(dateTimeString) + 2);
        dateTimeString[strlen(dateTimeString) - 1] = 'Z';
        strcat(dateTimeString, "\n");
    }
    
    return dateTimeString;
}
// **************************************************************************

// ************* Static helper functions ************************************
ssize_t readNextLine(char** currentLine, char** nextLine, FILE* fp, size_t* len) {
    ssize_t read_size = 0;

    if (*currentLine == NULL) {
        getline(currentLine, len, fp);
        if (*(*currentLine + strlen(*currentLine) - 2) != '\r') { // make sure line ends with "\r\n"
            return -1;
        }
        *(*currentLine + strlen(*currentLine) - 2) = '\0'; // remove the \r\n from the string
        read_size = getline(nextLine, len, fp);
        if (*(*nextLine + strlen(*nextLine) - 2) != '\r') { // make sure line ends with "\r\n"
            return -1;
        }
        *(*nextLine + strlen(*nextLine) - 2) = '\0'; // remove the \r\n from the string
        while (*nextLine[0] == ' ') {
            *currentLine = (char*)realloc(*currentLine, strlen(*currentLine) + strlen(*nextLine));
            strcat(*currentLine, *nextLine + 1);
            read_size = getline(nextLine, len, fp);
            if (*(*nextLine + strlen(*nextLine) - 2) != '\r') { // make sure line ends with "\r\n"
                return -1;
            }
            *(*nextLine + strlen(*nextLine) - 2) = '\0';
        }
    } else {
        // copy nextLine to currentLine
        *currentLine = (char*)realloc(*currentLine, strlen(*nextLine) + 1);
        if (*currentLine) {
            snprintf(*currentLine, strlen(*nextLine) + 1, "%s", *nextLine);
        } else {
            return -1;
        }

        read_size = getline(nextLine, len, fp);
        if (*(*nextLine + strlen(*nextLine) - 2) != '\r') { // make sure line ends with "\r\n"
            return -1;
        }
        *(*nextLine + strlen(*nextLine) - 2) = '\0'; // remove the \r\n from the string
        while (*nextLine[0] == ' ') {
            *currentLine = (char*)realloc(*currentLine, strlen(*currentLine) + strlen(*nextLine));
            strcat(*currentLine, *nextLine + 1);
            read_size = getline(nextLine, len, fp);
            if (*(*nextLine + strlen(*nextLine) - 2) != '\r') { // make sure line ends with "\r\n"
                return -1;
            }
            *(*nextLine + strlen(*nextLine) - 2) = '\0';
        }
    }

    return read_size;
}

Property* createProperty(Card* card, const char* stringToParse) {
    char* propertyName = NULL;
    char* paramString = NULL;
    char* valueString = NULL;
    char* propertyString = NULL; // just a temporary string so that the original one isn't modified by strtok
    Property* newProperty = NULL;

    newProperty = (Property*)malloc(sizeof(Property));
    newProperty->name = NULL;
    newProperty->group = NULL;
    newProperty->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
    newProperty->values = initializeList(valueToString, deleteValue, compareValues);
    propertyString = (char*)malloc(strlen(stringToParse) + 1);
    snprintf(propertyString, strlen(stringToParse) + 1, "%s", stringToParse);
    
    valueString = strpbrk(propertyString, ":"); 
    if (valueString) {
        valueString = valueString + 1; // set valueString to everything after colon
    } else { // no colon in the string
        free(propertyString);
        deleteProperty(newProperty);
        return NULL;
    }
    paramString = strtok(propertyString, ":"); // set paramString to everything before colon
    propertyName = strtok(paramString, ";:");


    if (strlen(propertyName) == 0) {
        free(propertyString);
        deleteProperty(newProperty);
        return NULL;
    }

    // get parameters
    char* paramToken = strtok(NULL, ";");
    while (paramToken) {
        Parameter* newParam = (Parameter*)malloc(sizeof(Parameter));
        int paramNameLen = strcspn(paramToken, "=");
        newParam->name = (char*)malloc(paramNameLen + 1);
        newParam->value = (char*)malloc(strlen(paramToken) - paramNameLen);
        strncpy(newParam->name, paramToken, paramNameLen + 1);
        newParam->name[paramNameLen] = '\0';
        strncpy(newParam->value, paramToken + paramNameLen + 1, strlen(paramToken) - paramNameLen);
        if (strlen(newParam->value) == 0) {
            free(propertyString);
            deleteParameter(newParam);
            deleteProperty(newProperty);
            return NULL;
        }
        paramToken = strtok(NULL, ";");
        insertBack(newProperty->parameters, newParam);
    }

    // get group
    if (strchr(propertyName, '.')) {
        char* group = strtok(propertyName, ".");
        newProperty->group = (char*)malloc(strlen(group) + 1);
        strcpy(newProperty->group, group);
        propertyName = strtok(NULL, "."); // set propertyName to everthing after the '.'
    } else {
        newProperty->group = "";
    }
    
    // get values
    if (strcasecmp(propertyName, "FN") == 0) {
        char* token = strtok(valueString, ";"); // get the first value
        if (token == NULL) {
            free(propertyString);
            deleteProperty(newProperty);
            return NULL;
        }
        newProperty->name = (char*)malloc(strlen(propertyName) + 1);
        strcpy(newProperty->name, propertyName);
        char* value = (char*)malloc(strlen(token) + 1);
        strcpy(value, token);
        insertBack(newProperty->values, (void*)value);
        card->fn = newProperty;
    } else if (strcasecmp(propertyName, "BDAY") == 0) {
        bool isText = false;
        void* element;
        ListIterator iter = createIterator(newProperty->parameters);
        while ((element = nextElement(&iter)) != NULL) {
            Parameter* param = (Parameter*)element;
            if (strcasecmp(param->name, "VALUE") == 0 && strcasecmp(param->value, "text") == 0) {
                isText = true;
                break;
            }
        }

        if (isText) {
            card->birthday = (DateTime*)malloc(sizeof(DateTime));
            card->birthday->UTC = false;
            card->birthday->isText = true;
            card->birthday->date = "";
            card->birthday->time = "";
            card->birthday->text = (char*)malloc(strlen(valueString) + 1);
            strcpy(card->birthday->text, valueString);
        } else {
            card->birthday = createDateTime(valueString);
        }

        // free the property that was created since it didn't actually get used
        freeList(newProperty->parameters);
        freeList(newProperty->values);
        free(newProperty);
    } else if (strcasecmp(propertyName, "ANNIVERSARY") == 0) {
        bool isText = false;
        void* element;
        ListIterator iter = createIterator(newProperty->parameters);
        while ((element = nextElement(&iter)) != NULL) {
            Parameter* param = (Parameter*)element;
            if (strcasecmp(param->name, "VALUE") == 0 && strcasecmp(param->value, "text") == 0) {
                isText = true;
                break;
            }
        }

        if (isText) {
            card->anniversary = (DateTime*)malloc(sizeof(DateTime));
            card->anniversary->UTC = false;
            card->anniversary->isText = true;
            card->anniversary->date = "";
            card->anniversary->time = "";
            card->anniversary->text = (char*)malloc(strlen(valueString) + 1);
            strcpy(card->anniversary->text, valueString);
        } else {
            card->anniversary = createDateTime(valueString);
        }

        // free the property that was created since it didn't actually get used
        freeList(newProperty->parameters);
        freeList(newProperty->values);
        free(newProperty);
    } else if (strcasecmp(propertyName, "SOURCE") == 0 ||
            strcasecmp(propertyName, "KIND") == 0 ||
            strcasecmp(propertyName, "XML") == 0 ||
            strcasecmp(propertyName, "N") == 0 ||
            strcasecmp(propertyName, "NICKNAME") == 0 ||
            strcasecmp(propertyName, "PHOTO") == 0 ||
            strcasecmp(propertyName, "GENDER") == 0 ||
            strcasecmp(propertyName, "ADR") == 0 ||
            strcasecmp(propertyName, "TEL") == 0 ||
            strcasecmp(propertyName, "EMAIL") == 0 ||
            strcasecmp(propertyName, "IIMP") == 0 ||
            strcasecmp(propertyName, "LANG") == 0 ||
            strcasecmp(propertyName, "TZ") == 0 ||
            strcasecmp(propertyName, "GEO") == 0 ||
            strcasecmp(propertyName, "TITLE") == 0 ||
            strcasecmp(propertyName, "ROLE") == 0 ||
            strcasecmp(propertyName, "LOGO") == 0 ||
            strcasecmp(propertyName, "ORG") == 0 ||
            strcasecmp(propertyName, "MEMBER") == 0 ||
            strcasecmp(propertyName, "RELATED") == 0 ||
            strcasecmp(propertyName, "CATEGORIES") == 0 ||
            strcasecmp(propertyName, "NOTE") == 0 ||
            strcasecmp(propertyName, "PRODID") == 0 ||
            strcasecmp(propertyName, "REV") == 0 ||
            strcasecmp(propertyName, "SOUND") == 0 ||
            strcasecmp(propertyName, "UID") == 0 ||
            strcasecmp(propertyName, "CLIENTPIDMAP") == 0 ||
            strcasecmp(propertyName, "URL") == 0 ||
            strcasecmp(propertyName, "KEY") == 0 ||
            strcasecmp(propertyName, "FBURL") == 0 ||
            strcasecmp(propertyName, "CALADRURI") == 0 ||
            strcasecmp(propertyName, "CALURI") == 0) {
        newProperty->name = (char*)malloc(strlen(propertyName) + 1);
        strcpy(newProperty->name, propertyName);
        parsePropertyValues(newProperty->values, propertyName, valueString);
        insertBack(card->optionalProperties, (void*)newProperty);
    } else {
        freeList(newProperty->parameters);
        freeList(newProperty->values);
        free(newProperty);
        free(propertyString);
        return NULL;
    }

    free(propertyString);
    return newProperty;
}

void parsePropertyValues(List* valueList, const char* name, char* valueString) {
    char* previousDelim = valueString;
    char* nextDelim = strpbrk(valueString, ";");

    while (nextDelim != NULL) {
        char* value = (char*)malloc(nextDelim - previousDelim + 1);
        strncpy(value, previousDelim, nextDelim - previousDelim + 1);
        value[nextDelim - previousDelim] = '\0';
        insertBack(valueList, (void*)value);
        previousDelim = nextDelim + 1;
        nextDelim = strpbrk(nextDelim + 1, ";");
    }

    // get the last value
    char* value = (char*)malloc(strlen(previousDelim) + 1);
    strcpy(value, previousDelim);
    insertBack(valueList, (void*)value);
}

DateTime* createDateTime(char* inputString) {    
    DateTime* dateTime = (DateTime*)malloc(sizeof(DateTime));
    char* date = NULL;
    char* time = NULL;

    dateTime->UTC = false;
    dateTime->date = "";
    dateTime->time = "";
    dateTime->isText = false; // this function should only be called for date-and-or-time inputs
    dateTime->text = "";

    if (inputString[strlen(inputString) - 1] == 'Z') {
        dateTime->UTC = true;
        inputString[strlen(inputString) - 1] = '\0'; // remove the Z
    }

    if (inputString[0] == 'T') {
        time = (char*)malloc(strlen(inputString + 1) + 1);
        strcpy(time, inputString + 1);
        dateTime->time = time;
    } else {
        char* token = strtok(inputString, "T");
        date = (char*)malloc(strlen(token) + 1);
        strcpy(date, token);
        dateTime->date = date;
        token = strtok(NULL, "");
        if (token) {
            time = (char*)malloc(strlen(token) + 1);
            strcpy(time, token);
            dateTime->time = time;
        }
    }

    return dateTime;
}

bool validateDateTime(DateTime* dateTime) {
    if (dateTime->date == NULL || dateTime->time == NULL || dateTime->text == NULL) {
        return false;
    } 

    if (dateTime->UTC && dateTime->isText) {
        return false;
    }

    if (dateTime->isText && (strlen(dateTime->date) > 0 || strlen(dateTime->time) > 0)) {
        return false;
    }

    if (!dateTime->isText && strlen(dateTime->text) > 0) {
        return false;
    }

    return true;
}
// **************************************************************************
