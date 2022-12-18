//
// dirjson.h - v0.0
//
// Copyright (c) Jesper Jansson and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for details.
//

#ifndef DIR_JSON_H
#define DIR_JSON_H
#include <stdio.h>

typedef struct json_read_context json_read_context;
typedef struct json_callbacks_object json_callbacks_object;

// ===============================================================================
// Data Types
// ===============================================================================

typedef signed long long json_s64;
typedef double           json_f64;
typedef struct {
    size_t Length;
    const char* Data;
} json_string;


// ===============================================================================
// Object Callbacks
// ===============================================================================

#define JSON_MANDATORY 1

typedef void(*json_member_callback)(json_read_context* Context, void* Ptr);
typedef void(*json_unknown_key_callback)(json_read_context* Context, void* Ptr, json_string Key);

typedef struct {
    const char* Key;
    json_member_callback Callback;
    int Mandatory; // If
} json_member;

json_callbacks_object* JsonInitializeObject(json_member* Members, int MemberCount, json_unknown_key_callback UnknownKeyCallback);
void JsonDestroyObject(json_callbacks_object* Object);


// ===============================================================================
// Reading
// ===============================================================================

json_read_context* JsonReadReadFile(FILE* File);
json_read_context* JsonReadOpenAndReadFile(const char* FilePath);
json_read_context* JsonReadFromString(const char* JsonString);
void JsonReadDestroyContext(json_read_context* Context);

void JsonReadReportErrorIfNoErrorExists(json_read_context* Context, const char* Start, const char* OnePastLast,
                                    const char* FormatString, ...);
const char* JsonReadError(json_read_context* Context);

void        JsonReadObjectUsingCallbacks(json_read_context* Context, json_callbacks_object* Object, void* Ptr);
int         JsonReadArray( json_read_context* Context);
int         JsonReadBool(  json_read_context* Context);
json_s64    JsonReadS64(   json_read_context* Context);
json_f64    JsonReadF64(   json_read_context* Context);
json_string JsonReadString(json_read_context* Context);
void        JsonReadNull(  json_read_context* Context);
void        JsonReadEOF(   json_read_context* Context);

// Returns true if the next value is of the respective type.
// Doesn't check that the value is legally formatted. For example JsonReadNextIsNull will return 1 for noll.
int JsonReadNextIsObject(json_read_context* Context);
int JsonReadNextIsArray( json_read_context* Context);
int JsonReadNextIsBool(  json_read_context* Context);
int JsonReadNextIsNumber(json_read_context* Context);
int JsonReadNextIsString(json_read_context* Context);
int JsonReadNextIsNull(  json_read_context* Context);


// ===============================================================================
// Writing
// ===============================================================================

typedef struct json_write_context json_write_context;
typedef void(*json_write_callback)(json_write_context*, char* Data, int Size);

json_write_context* JsonWriteInitializeContextTargetString(int StartBufferSize);
json_write_context* JsonWriteInitializeContextTargetFile(FILE* File, int BufferSize);
json_write_context* JsonWriteInitializeContextTargetFilePath(const char* FilePath, int BufferSize);
json_write_context* JsonWriteInitializeContextTargetCustom(json_write_callback Callback, int BufferSize);

void JsonWriteSetPrettyPrint(json_write_context* Context, int ShouldPrettyPrint);

char* JsonWriteFinalize(      json_write_context* Context);
void  JsonWriteDestroyContext(json_write_context* Context);

void JsonWriteStartObject(json_write_context* Context);
void JsonWriteKey(        json_write_context* Context, const char* Key);
void JsonWriteEndObject(  json_write_context* Context);
void JsonWriteStartArray( json_write_context* Context);
void JsonWriteEndArray(   json_write_context* Context);
void JsonWriteBool(       json_write_context* Context, int Value);
void JsonWriteS64(        json_write_context* Context, json_s64 Value);
void JsonWriteF64(        json_write_context* Context, json_f64 Value);
void JsonWriteString(     json_write_context* Context, const char* Str);
void JsonWriteNull(       json_write_context* Context);


// ===============================================================================
// Implementation
// ===============================================================================

#ifdef DIR_JSON_IMPLEMENTATION

// ===============================================================================
// Customization macros
// ===============================================================================
#ifndef DIR_JSON_ERROR_PREFIX_STRING
#define DIR_JSON_ERROR_PREFIX_STRING "ERROR(Line %l, Col %c): "
#endif

#ifndef DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT
#define DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT 80
#endif

#ifndef DIR_JSON_ERROR_HIGHLIGHT_CARROT
#define DIR_JSON_ERROR_HIGHLIGHT_CARROT '^'
#endif

#ifndef DIR_JSON_WRITE_INDENTION_SPACE_COUNT
#define DIR_JSON_WRITE_INDENTION_SPACE_COUNT 4
#endif


// ===============================================================================
// Includes
// ===============================================================================

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>


// ===============================================================================
// Struct declerations
// ===============================================================================

struct json_callbacks_object {
    int SlotsCount, MandatoryMemberCount;
    json_unknown_key_callback UnknownKeyCallback;
    json_member_callback* MemberCallbacks;
    int* MemberKeys;
};

struct json_read_context {
    char* JsonDataOwnagePtr;
    const char* JsonData;
    const char* CurrentChar;
    const char* EOFLocation;
    
    const char* StartOfCurrentLine;
    int LineNumber;
    
    const char* Error;
    
    int StringBufferSize;
    char* StringBuffer;
};

struct json_write_context {
    int ShouldCloseFile;
    FILE* TargetFile;
    json_write_callback Callback;
    
    const char* Error;
    
    int PrettyPrint;
    int IsRootValue;
    int Indention;
    int ContextClue;
    
    int Used, Size;
    char* Buffer;
};


// ===============================================================================
// Helper Functions
// ===============================================================================

static unsigned int __JsonHashString(const char* String) {
    unsigned int Hash = 5381;
    int Char;
    
    while (Char = *(String++))
        Hash = ((Hash << 5) + Hash) + Char;
    
    return Hash;
}

// ===============================================================================
// Object Callbacks Implementation
// ===============================================================================

const int __Json_Mandatory_Flag = (1 << 31);

static void ReportUnkownMemberCallback(json_read_context* Context, void* Ptr, json_string Key) {
    const char* KeyEndPtr = Context->CurrentChar - 1;
    while (*KeyEndPtr   != '"') --KeyEndPtr;
    const char* KeyStartPtr = KeyEndPtr - 1;
    while (*KeyStartPtr != '"') --KeyStartPtr;
    
    char* KeyCopy = malloc(Key.Length + 1);
    memcpy(KeyCopy, Key.Data, Key.Length + 1);
    JsonReadReportErrorIfNoErrorExists(Context, KeyStartPtr, KeyEndPtr + 1,
                                       "Unkown member encountered (Key '%s'. )", KeyCopy);
    free(KeyCopy);
}

json_callbacks_object* JsonInitializeObject(json_member* Members, int MemberCount, json_unknown_key_callback UnknownKeyCallback) {
    int SlotsCount = (MemberCount * 10) / 8 + 1;
    int MandatoryMemberCount = 0;
    size_t StringsByteCount = 0;
    for (int MemberIndex = 0; MemberIndex < MemberCount; MemberIndex++) {
        assert(Members[MemberIndex].Key && "Key can't be null. ");
        assert(Members[MemberIndex].Callback && "Callback can't be null. ");
        StringsByteCount += strlen(Members[MemberIndex].Key) + 1;
        if (Members[MemberIndex].Mandatory) {
            MandatoryMemberCount += 1;
        }
    }
    
    size_t TotalBytes = sizeof(json_callbacks_object);
    TotalBytes += sizeof(json_member_callback) * SlotsCount;
    TotalBytes += sizeof(int)                  * SlotsCount;
    TotalBytes += StringsByteCount;
    
    json_callbacks_object* Result = calloc(TotalBytes, 1);
    Result->SlotsCount = SlotsCount;
    Result->MandatoryMemberCount = MandatoryMemberCount;
    Result->UnknownKeyCallback = UnknownKeyCallback ? UnknownKeyCallback : ReportUnkownMemberCallback;
    Result->MemberCallbacks = (json_member_callback*)((char*)Result + sizeof(json_callbacks_object));
    Result->MemberKeys      = (int*                 )&Result->MemberCallbacks[SlotsCount];
    assert(((intptr_t)Result->MemberCallbacks % sizeof(Result->MemberCallbacks[0])) == 0);
    assert(((intptr_t)Result->MemberKeys      % sizeof(Result->MemberKeys[0])) == 0);
    
    char* StringCopyCurrentChar = (char*)&Result->MemberKeys[SlotsCount];
    
    for (int MemberIndex = 0; MemberIndex < MemberCount; MemberIndex++) {
        json_member* Member = &Members[MemberIndex];
        unsigned int Hash = __JsonHashString(Member->Key);
        
        while (1) {
            unsigned int Index = Hash % SlotsCount;
            
            if (!Result->MemberKeys[Index]) {
                int Offset = (int)(StringCopyCurrentChar - (char*)Result);
                
                size_t Length = strlen(Member->Key);
                memcpy(StringCopyCurrentChar, Member->Key, Length + 1);
                StringCopyCurrentChar += Length + 1;
                
                Result->MemberKeys[Index]= Offset | (Member->Mandatory ? __Json_Mandatory_Flag : 0);
                Result->MemberCallbacks[Index] = Member->Callback;
                
                break;
            }
            
            Hash += 1;
        }
    }
    
    assert(StringCopyCurrentChar == ((char*)Result + TotalBytes));
    
    return Result;
}

void JsonDestroyObject(json_callbacks_object* Object) {
    free(Object);
}


// ===============================================================================
// Read Implementation
// ===============================================================================

static void __JsonEatWhiteSpaces(json_read_context* Context) {
    const char* CurrentChar = Context->CurrentChar;
    char Char;
    while (Char = *CurrentChar) {
        if (Char == '\n') {
            Context->LineNumber += 1;
        } else if (Char != '\r' && Char != '\t' && Char != ' ') {
            break;
        }
        ++CurrentChar;
    }
    Context->CurrentChar = CurrentChar;
}

static int __JsonEatCharacter(json_read_context* Context, char Character) {
    if (*Context->CurrentChar == Character) {
        Context->CurrentChar += 1;
        return 1;
    }
    return 0;
}

static void __JsonIncreaseStringBufferSize(json_read_context* Context) {
    Context->StringBufferSize *= 2;
    Context->StringBuffer = realloc(Context->StringBuffer, Context->StringBufferSize);
    assert(Context->StringBuffer && "JSON: Out of memory. ");
    // TODO: Handle this?
}

static int __JsonPutCharInBuffer(json_read_context* Context, int LengthBefore, char Char) {
    if (LengthBefore >= Context->StringBufferSize) {
        __JsonIncreaseStringBufferSize(Context);
    }
    Context->StringBuffer[LengthBefore] = Char;
    return LengthBefore + 1;
}

static int __JsonPutStringInBuffer(json_read_context* Context, int LengthBefore, const char* String) {
    while (*String) {
        LengthBefore = __JsonPutCharInBuffer(Context, LengthBefore, *String);
        String += 1;
    }
    return LengthBefore;
}

static void __JsonInitializationOutOfMemoryError(json_read_context* Context, const char* Error) {
    Context->Error = Error;
    Context->JsonData    = "\0";
    Context->CurrentChar = Context->JsonData;
}

static json_read_context* __JsonCreateReadContext() {
    json_read_context* Context = malloc(sizeof(json_read_context));
    assert(Context);
    
    Context->JsonDataOwnagePtr = 0;
    Context->JsonData     = "\0";
    Context->CurrentChar  = Context->JsonData;
    
    Context->StartOfCurrentLine = Context->JsonData;
    Context->LineNumber = 1;
    
    Context->Error = 0;
    
    Context->StringBufferSize = 256;
    Context->StringBuffer = malloc(Context->StringBufferSize);
    if (!Context->StringBuffer) {
        __JsonInitializationOutOfMemoryError(Context, "Couldn't allocate string buffer. ");
        return Context;
    }
    
    __JsonEatWhiteSpaces(Context);
    return Context;
}

static void ReadFile(json_read_context* Context, FILE* File) {
    size_t FileSize;
    {
        int ErrorSeekingEnd = fseek(File, 0, SEEK_END);
        FileSize = ftell(File);
        int ErrorSeekingSet = fseek(File, 0, SEEK_SET);
        if (ErrorSeekingEnd || ErrorSeekingSet || FileSize == -1L) {
            Context->Error = "Couldn't read file size. ";
            return;
        }
    }
    
    char* Data = malloc(FileSize + 1);
    if (!Data) {
        Context->Error = "Couldn't allocate data for the file content. ";
        return;
    }
    
    size_t AmountRead = fread(Data, 1, FileSize, File);
    if (AmountRead != FileSize) {
        __JsonInitializationOutOfMemoryError(Context, "Couldn't read file. ");
        free(Data);
        return;
    }
    Data[FileSize] = '\0';
    
    Context->JsonDataOwnagePtr  = Data;
    Context->JsonData           = Data;
    Context->CurrentChar        = Data;
    Context->StartOfCurrentLine = Data;
}

json_read_context* JsonReadReadFile(FILE* File) {
    json_read_context* Context = __JsonCreateReadContext();
    if (!JsonReadError(Context))
        ReadFile(Context, File);
    return Context;
}

json_read_context* JsonReadOpenAndReadFile(const char* FilePath) {
    json_read_context* Context = __JsonCreateReadContext();
    
    if (JsonReadError(Context))
        return Context;
    
    FILE* File = fopen(FilePath, "r");
    if (!File) {
        __JsonInitializationOutOfMemoryError(Context, "Failed to open file. ");
        return Context;
    }
    
    ReadFile(Context, File);
    
    if (fclose(File)) {
        Context->Error = "Failed to close file. ";
    }
    
    return Context;
}

json_read_context* JsonReadFromString(const char* JsonString) {
    json_read_context* Context = __JsonCreateReadContext();
    
    if (JsonReadError(Context))
        return Context;
    
    Context->JsonData           = JsonString;
    Context->CurrentChar        = JsonString;
    Context->StartOfCurrentLine = JsonString;
    
    return Context;
}

void JsonReadDestroyContext(json_read_context* Context) {
    free(Context->StringBuffer);
    free(Context->JsonDataOwnagePtr);
}

void JsonReadReportErrorIfNoErrorExists(json_read_context* Context, const char* Start, const char* OnePastLast, 
                                        const char* FormatString, ...) {
    if (Context->Error)
        return;
    
    int Line   = !Start ? -1 : Context->LineNumber;
    int Column = !Start ? -1 : (int)(Context->CurrentChar - Context->StartOfCurrentLine) + 1;
    
    int AmountWritten = 0;
    if (DIR_JSON_ERROR_PREFIX_STRING) { // Write prefix string
        const char* PrefixIterator = DIR_JSON_ERROR_PREFIX_STRING;
        while (*PrefixIterator) {
            if ((*PrefixIterator == '%' && PrefixIterator[1] == 'l') ||
                (*PrefixIterator == '%' && PrefixIterator[1] == 'c')) {
                int Value = PrefixIterator[1] == 'l' ? Line : Column;
                AmountWritten += snprintf(Context->StringBuffer + AmountWritten, Context->StringBufferSize - AmountWritten,
                                          "%d", Value);
                PrefixIterator += 2;
            } else if (*PrefixIterator == '\\' && PrefixIterator[1] == '%') {
                AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, '\\');
                AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, '%');
                PrefixIterator += 2;
            } else {
                AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, *PrefixIterator);
                PrefixIterator += 1;
            }
        }
    }
    
    va_list VariableArguments;
    va_start(VariableArguments, FormatString);
    while (1) {
        va_list VariableArgumentsCopy;
        va_copy(VariableArgumentsCopy, VariableArguments);
        
        int TotalWritten = (int)vsnprintf(Context->StringBuffer + AmountWritten, Context->StringBufferSize - AmountWritten, 
                                          FormatString, VariableArgumentsCopy) + AmountWritten;
        if ((TotalWritten + 1) < Context->StringBufferSize) {
            AmountWritten = TotalWritten;
            break;
        }
        
        __JsonIncreaseStringBufferSize(Context);
    }
    va_end(VariableArguments);
    
    if (DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT) {
        const char* StartFrom    = Start;
        const char* EndOneBefore = OnePastLast;
        int AmountSearchedBackwards = 0;
        int AmountSearchedForwards  = 0;
        
        for (; AmountSearchedBackwards < DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT; AmountSearchedBackwards++) {
            if (StartFrom == Context->JsonData)
                break;
            if (*StartFrom == '\r' || *StartFrom == '\n') {
                StartFrom += 1;
                break;
            }
            StartFrom -= 1;
        }
        
        if (*Context->CurrentChar != '\0') {
            for (; AmountSearchedForwards < DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT; AmountSearchedForwards++) {
                if (*EndOneBefore == '\0' || *EndOneBefore == '\r' || *EndOneBefore == '\n') {
                    break;
                }
                EndOneBefore += 1;
            }
        }
        
        AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "\n > ");
        if (AmountSearchedBackwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
            AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "...");
        
        const char* Iterator = StartFrom;
        while (Iterator < EndOneBefore) {
            if (*Iterator)
                AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, *Iterator);
            else 
                AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "<eof>");
            Iterator += 1;
        }
        
        if (AmountSearchedForwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
            AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "...");
        
        if (DIR_JSON_ERROR_HIGHLIGHT_CARROT) {
            AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "\n > ");
            if (AmountSearchedBackwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
                AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "...");
            
            const char* Iterator = StartFrom;
            while (Iterator < EndOneBefore) {
                if (Iterator >= Start && Iterator < OnePastLast) {
                    AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, DIR_JSON_ERROR_HIGHLIGHT_CARROT);
                    if (*Iterator == '\0')
                        AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "    ");
                } else if (*Iterator == '\0')  {
                    AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "     ");
                } else {
                    AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, *Iterator == '\t' ? '\t' : ' ');
                }
                Iterator += 1;
            }
            
            if (AmountSearchedForwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
                AmountWritten = __JsonPutStringInBuffer(Context, AmountWritten, "...");
        }
    }
    
    AmountWritten = __JsonPutCharInBuffer(Context, AmountWritten, '\0');
    Context->Error = Context->StringBuffer;
    Context->JsonData    = "\0";
    Context->CurrentChar = Context->JsonData;
}

const char* JsonReadError(json_read_context* Context) {
    return Context->Error;
}

void JsonReadObjectUsingCallbacks(json_read_context* Context, json_callbacks_object* Object, void* Ptr) {
    static const char   NULL_STR[]      = "null";
    static const size_t NULL_STR_LENGTH = sizeof(NULL_STR) - 1;
    if (memcmp(Context->CurrentChar, NULL_STR, NULL_STR_LENGTH) == 0) {
        Context->CurrentChar += NULL_STR_LENGTH;
        __JsonEatWhiteSpaces(Context);
        
        if (Object->MandatoryMemberCount != 0) {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar - NULL_STR_LENGTH, Context->CurrentChar,
                                               "Got null parsing object when the object has mandatory members. ");
        }
        
        return;
    } 
    
    if (__JsonEatCharacter(Context, '{')) {
        __JsonEatWhiteSpaces(Context);
        const char* EndOfObject = Context->CurrentChar;
        
        int MandatoryMembersFound = 0;
        if (!__JsonEatCharacter(Context, '}')) {
            while (1) {
                json_string Key = JsonReadString(Context);
                if (!Key.Data) return;
                unsigned int Hash = __JsonHashString(Key.Data);
                
                unsigned int SlotIndex =  Hash % Object->SlotsCount;
                
                int Found = 0;
                int IsMandatory;
                while (Object->MemberKeys[SlotIndex]) {
                    IsMandatory = Object->MemberKeys[SlotIndex] & __Json_Mandatory_Flag;
                    char* SlotKey = (char*)Object + (Object->MemberKeys[SlotIndex] & ~__Json_Mandatory_Flag);
                    
                    if (strcmp(Key.Data, SlotKey) == 0) {
                        Found = 1;
                        break;
                    }
                    
                    SlotIndex = (SlotIndex + 1) % Object->SlotsCount;
                }
                
                if (!__JsonEatCharacter(Context, ':')) {
                    JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                                       "A colon needs to follow the key for each member.");
                    return;
                } else {
                    __JsonEatWhiteSpaces(Context);
                    if (__JsonEatCharacter(Context, ',')) {
                        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar - 1, Context->CurrentChar,
                                                           "A value needs to follow the colon, got a ','.");
                        return;
                    } else {
                        if (Found) {
                            if (IsMandatory)
                                MandatoryMembersFound += 1;
                            Object->MemberCallbacks[SlotIndex](Context, Ptr);
                        } else {
                            Object->UnknownKeyCallback(Context, Ptr, Key);
                        }
                    }
                }
                
                if (__JsonEatCharacter(Context, ',')) {
                    __JsonEatWhiteSpaces(Context);
                } else if (__JsonEatCharacter(Context, '}')) {
                    __JsonEatWhiteSpaces(Context);
                    EndOfObject = Context->CurrentChar - 1;
                    break;
                } else {
                    JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                                       "Expected a ',' or '}'. ");
                    return;
                }
            }
        }
        if (MandatoryMembersFound != Object->MandatoryMemberCount) {
            JsonReadReportErrorIfNoErrorExists(Context, EndOfObject, EndOfObject + 1, 
                                               "Not all mandatory members where found. ");
            return;
        }
    } else {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                           "Expected a object. ");
    }
}

int JsonReadArray(json_read_context* Context) {
    int Result;
    switch (*Context->CurrentChar) {
        case '[': Result = 1; break;
        case ',': Result = 1; break;
        case ']': Result = 0; break;
        default: {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                               "Expected a '[' or ',' or ']'. ");
            return 0;
        }
    }
    
    Context->CurrentChar += 1;
    __JsonEatWhiteSpaces(Context);
    
    if (Result && *Context->CurrentChar == ',') {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected a value. ");
        return 0;
    }
    
    return Result;
}

int JsonReadBool(json_read_context* Context) {
    int Result;
    static const char TRUE_STR[]  = "true";
    static const char FALSE_STR[] = "false";
    if (memcmp(Context->CurrentChar, TRUE_STR, sizeof(TRUE_STR) - 1) == 0) {
        Context->CurrentChar += sizeof(TRUE_STR) - 1;
        Result = 1;
    } else if (memcmp(Context->CurrentChar, FALSE_STR, sizeof(FALSE_STR) - 1) == 0) {
        Context->CurrentChar += sizeof(FALSE_STR) - 1;
        Result = 0;
    } else {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected a boolean ('true' or 'false'. )");
        Result = 0;
    }
    __JsonEatWhiteSpaces(Context);
    return Result;
}

json_s64 JsonReadS64(json_read_context* Context) {
    const char* CurrentChar = Context->CurrentChar;
    json_s64 Value = 0;
    int IsNegative = *CurrentChar == '-';
    if (IsNegative) ++CurrentChar;
    
    if (*CurrentChar < '0' || *CurrentChar > '9') {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected a integer, needs to start with a digit (0-9). ");
        return 0;
    }
    
    while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
        Value = Value * 10 + (*CurrentChar - '0');
        CurrentChar += 1;
    }
    
    if (*CurrentChar == '.') {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected a integer but got a decimal point. ");
        return 0;
    }
    
    if (*CurrentChar == 'e' || *CurrentChar == 'E') {
        CurrentChar += 1;
        if (*CurrentChar == '-') {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                               "Expected a integer, negative exponent is not allowed for integers. ");
            return 0;
        } else if (*CurrentChar == '+') {
            CurrentChar += 1;
        }
        
        if (*CurrentChar < '0' || *CurrentChar > '9') {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                               "The exponent needs to contain atleast one digit (0-9). ");
            return 0;
        }
        
        unsigned Exponent = 0;
        
        while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
            Exponent = Exponent * 10 + (*CurrentChar - '0');
            ++CurrentChar;
        }
        
        json_s64 Base = 10;
        while (Exponent) {
            if (Exponent % 2)
                Value *= Base;
            Exponent /= 2;
            Base *= Base;
        }
    }
    
    Context->CurrentChar = CurrentChar;
    __JsonEatWhiteSpaces(Context);
    return IsNegative ? -Value : Value;
}

json_f64 JsonReadF64(json_read_context* Context) {
    const char* CurrentChar = Context->CurrentChar;
    
    if (*CurrentChar == '-') {
        CurrentChar += 1;
    }
    
    if (*CurrentChar < '0' || *CurrentChar > '9') {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected a number, needs to start with a digit (0-9). ");
        return 0;
    }
    
    while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
        CurrentChar += 1;
    }
    
    if (*CurrentChar == '.') {
        CurrentChar += 1;
        
        if (*CurrentChar < '0' || *CurrentChar > '9') {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                               "Fraction is empty, needs to contain atleast one digit (0-9). ");
            return 0;
        }
        
        while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
            CurrentChar += 1;
        }
    }
    
    if (*CurrentChar == 'e' || *CurrentChar == 'E') {
        CurrentChar += 1;
        if (*CurrentChar == '-' || *CurrentChar == '+') {
            CurrentChar += 1;
        }
        
        if (*CurrentChar < '0' || *CurrentChar > '9') {
            JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                               "Exponent is empty, needs to contain atleast one digit (0-9). ");
            return 0;
        }
        
        while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
            ++CurrentChar;
        }
    }
    
    char* EndPtr;
    json_f64 Result = strtod(Context->CurrentChar, &EndPtr); 
    assert(EndPtr == CurrentChar); // TODO: Look up if this is valid, for now keep it to test. 
    Context->CurrentChar = CurrentChar;
    __JsonEatWhiteSpaces(Context);
    return Result;
}

json_string JsonReadString(json_read_context* Context) {
    const json_string ErrorResult = { 0, 0 };
    const char* CurrentChar = Context->CurrentChar;
    int Length = 0;
    
    if (*CurrentChar != '"') {
        JsonReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                           "Expected a string. ", Context->LineNumber);
        return ErrorResult;
    }
    CurrentChar += 1;
    
    int Char;
    while (Char = *CurrentChar) {
        if (Char == '"') {
            break;
        } else if (Char == '\\') {
            Char = *(++CurrentChar);
            if (Char == '"') {
                Char = '"';
            } else if (Char == '\\') {
                Char = '\\';
            } else if (Char == '/') {
                Char = '/';
            } else if (Char == 'b') {
                Char = '\b';
            } else if (Char == 'f') {
                Char = '\f';
            } else if (Char == 'n') {
                Char = '\n';
            } else if (Char == 'r') {
                Char = '\r';
            } else if (Char == 't') {
                Char = '\t';
            } else if (Char == 'u') {
                ++CurrentChar;
                unsigned int Value = 0;
                for (int Digit = 0; Digit < 4; Digit++) {
                    Char = *CurrentChar;
                    Value = Value << 4;
                    if (Char >= '0' && Char <= '9')
                        Value |= Char - '0';
                    else if (Char >= 'a' && Char <= 'f')
                        Value |= Char - 'a';
                    else if (Char >= 'A' && Char <= 'F')
                        Value |= Char - 'A';
                    else {
                        JsonReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                                            "A unicode escape sequnce needs to be followed with 4 hex digits. ");
                        return ErrorResult;
                    }
                    ++CurrentChar;
                }
                
                if (Value >= 0x10000) {
                    if (Value >= 0x200000) {
                        JsonReadReportErrorIfNoErrorExists(Context,CurrentChar - 4, CurrentChar,
                                                           "Given unicode was to large. ");
                        return ErrorResult;
                    }
                    
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0 ) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 6 ) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 12) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0xF0 | ((Char >> 18) & 0x07));
                } else if (Value >= 0x800) {
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0 ) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 6 ) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0xE0 | ((Char >> 12) & 0x0F));
                } else if (Value >= 0x80) {
                    Length = __JsonPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0) & 0x3F));
                    Length = __JsonPutCharInBuffer(Context, Length, 0xC0 | ((Char >> 6) & 0x1F));
                } else {
                    Length = __JsonPutCharInBuffer(Context, Length, Char);
                }
                continue;
            } else {
                JsonReadReportErrorIfNoErrorExists(Context, CurrentChar - 1, CurrentChar,
                                                   "Unrecognised escape character. ");
                return ErrorResult;
            }
        } else if (Char == '\n' || Char == '\r') {
            JsonReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                               "Reached end of the line before ending the string. ");
            return ErrorResult;
        }
        
        Length = __JsonPutCharInBuffer(Context, Length, Char);
        CurrentChar += 1;
    }
    
    if (Char != '"') {
        JsonReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                           "Reached end of the file before ending the string. ");
        return ErrorResult;
    }
    CurrentChar += 1;
    
    Context->CurrentChar = CurrentChar;
    __JsonEatWhiteSpaces(Context);
    
    Length = __JsonPutCharInBuffer(Context, Length, '\0');
    
    json_string Result;
    Result.Data = Context->StringBuffer;
    Result.Length = Length;
    return Result;
}

void JsonReadNull(json_read_context* Context) {
    static const char NULL_STR[]  = "null";
    if (memcmp(Context->CurrentChar, NULL_STR, sizeof(NULL_STR) - 1) == 0) {
        Context->CurrentChar += sizeof(NULL_STR) - 1;
    } else {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Expected 'null'. ", Context->LineNumber);
    }
    __JsonEatWhiteSpaces(Context);
}

void JsonReadEOF(json_read_context* Context) {
    if (*Context->CurrentChar != '\0') {
        JsonReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                           "Unexpected content at end of file. ");
    }
}

int JsonReadNextIsObject(json_read_context* Context) { 
    return *Context->CurrentChar == '{';  
}

int JsonReadNextIsArray( json_read_context* Context) { 
    return *Context->CurrentChar == '[';  
}

int JsonReadNextIsBool(  json_read_context* Context) { 
    return *Context->CurrentChar == 't' || *Context->CurrentChar == 'f';  
}

int JsonReadNextIsNumber(json_read_context* Context) { 
    return (*Context->CurrentChar >= '0' && *Context->CurrentChar <= '9') || 
        *Context->CurrentChar == '-';  
}

int JsonReadNextIsString(json_read_context* Context) { 
    return *Context->CurrentChar == '"';  
}

int JsonReadNextIsNull(  json_read_context* Context) { 
    return *Context->CurrentChar == 'n';  
}

// ===============================================================================
// Write Implementation
// ===============================================================================

const char __Json_Spaces_Array[]= "                                                             ";
enum {
    __Json_Context_Clue_First_Item,
    __Json_Context_Clue_Member_Value,
    __Json_Context_Clue_Write_Comma
};

static void __JsonFlushBuffer(json_write_context* Context) {
    if (Context->TargetFile) {
        size_t AmountWritten = fwrite(Context->Buffer, 1, Context->Used, Context->TargetFile);
        if (AmountWritten != Context->Used && !Context->Error) {
            Context->Error = "Failed to write to file. ";
        }
        Context->Used = 0;
    } else if (Context->Callback) {
        Context->Callback(Context, Context->Buffer, Context->Used);
        Context->Used = 0;
    } else {
        Context->Size = max(128, Context->Size * 2);
        Context->Buffer = realloc(Context->Buffer, Context->Size);
        assert(Context->Buffer);
    }
}

static void __JsonWriteChar(json_write_context* Context, char Char) {
    if (Context->Used == Context->Size) {
        __JsonFlushBuffer(Context);
    }
    Context->Buffer[Context->Used++] = Char;
}

static void __JsonWriteN(json_write_context* Context, const char* Data, int Count) {
    int AmountWritten = 0;
    while (1) {
        int AmountToWrite = min(Count - AmountWritten, Context->Size - Context->Used);
        memcpy(Context->Buffer + Context->Used, Data + AmountWritten, AmountToWrite);
        Context->Used += AmountToWrite;
        AmountWritten += AmountToWrite;
        if (AmountWritten == Count) {
            break;
        } else {
            __JsonFlushBuffer(Context);
        }
    }
}

static void __JsonWriteNewItem(json_write_context* Context) {
    if (Context->PrettyPrint) {
        if (Context->IsRootValue) {
            Context->IsRootValue = 0;
        } else {
            if (Context->ContextClue != __Json_Context_Clue_Member_Value) {
                if (Context->ContextClue == __Json_Context_Clue_Write_Comma)
                    __JsonWriteChar(Context, ',');
                __JsonWriteChar(Context, '\n');
                int SpacesLeftToWrite = Context->Indention;
                while (SpacesLeftToWrite > 0) {
                    int SpacesWritten = min(SpacesLeftToWrite, sizeof(__Json_Spaces_Array) - 1);
                    __JsonWriteN(Context, __Json_Spaces_Array, SpacesWritten);
                    SpacesLeftToWrite -= SpacesWritten;
                }
            } else {
                __JsonWriteChar(Context, ' ');
            }
        }
    } else {
        switch (Context->ContextClue) {
            case __Json_Context_Clue_First_Item: {
            } break;
            case __Json_Context_Clue_Member_Value: {
            } break;
            case __Json_Context_Clue_Write_Comma: {
                __JsonWriteChar(Context, ',');
            } break;
        }
    }
    Context->ContextClue = __Json_Context_Clue_Write_Comma;
}

static json_write_context* __JsonCreateWriteContext(int BufferSize) {
    json_write_context* Context = calloc(1, sizeof(json_write_context));
    Context->ContextClue = __Json_Context_Clue_First_Item;
    Context->IsRootValue = 1;
    
    Context->Size   = BufferSize > 0 ? BufferSize : 512;
    Context->Buffer = malloc(Context->Size);
    
    return Context;
}

json_write_context* JsonWriteInitializeContextTargetString(int StartBufferSize) {
    json_write_context* Context = __JsonCreateWriteContext(StartBufferSize);
    
    Context->Size   = 128;
    Context->Buffer = malloc(Context->Size);
    
    return Context;
}

json_write_context* JsonWriteInitializeContextTargetFile(FILE* File, int BufferSize) {
    json_write_context* Context = __JsonCreateWriteContext(BufferSize);
    
    Context->TargetFile  = File;
    
    return Context;
}

json_write_context* JsonWriteInitializeContextTargetFilePath(const char* FilePath, int BufferSize) {
    FILE* File = fopen(FilePath, "w");
    
    json_write_context* Context = __JsonCreateWriteContext(BufferSize);
    
    Context->TargetFile      = File;
    Context->ShouldCloseFile = File != 0;
    Context->Error           = File != 0 ? 0 : "Could not open file. ";
    
    return Context;
}

json_write_context* JsonWriteInitializeContextTargetCustom(json_write_callback Callback, int BufferSize)  {
    json_write_context* Context = __JsonCreateWriteContext(BufferSize);
    
    Context->Callback = Callback;
    
    return Context;
}

void JsonWriteSetPrettyPrint(json_write_context* Context, int ShouldPrettyPrint) {
    Context->PrettyPrint = ShouldPrettyPrint;
}

char* JsonWriteFinalize(json_write_context* Context) {
    char* Result = 0;
    if (Context->TargetFile) {
        __JsonFlushBuffer(Context);
        if (Context->ShouldCloseFile) {
            fclose(Context->TargetFile);
        }
    } else if (Context->Callback) {
        __JsonWriteChar(Context, '\0');
        Context->Callback(Context, Context->Buffer, Context->Used);
    } else {
        __JsonWriteChar(Context, '\0');
        Result = Context->Buffer;
        Context->Buffer = 0;
    }
    return Result;
}

void JsonWriteDestroyContext(json_write_context* Context) {
    free(Context->Buffer);
    free(Context);
}

void JsonWriteStartObject(json_write_context* Context) {
    __JsonWriteNewItem(Context);
    
    __JsonWriteChar(Context, '{');
    Context->ContextClue = __Json_Context_Clue_First_Item;
    Context->Indention += DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
}

void JsonWriteKey(json_write_context* Context, const char* Key) {
    JsonWriteString(Context, Key);
    __JsonWriteChar(Context, ':');
    Context->ContextClue = __Json_Context_Clue_Member_Value;
}

void JsonWriteEndObject(json_write_context* Context) {
    Context->Indention -= DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
    Context->ContextClue = __Json_Context_Clue_First_Item;
    __JsonWriteNewItem(Context);
    
    __JsonWriteChar(Context, '}');
}

void JsonWriteStartArray(json_write_context* Context) {
    __JsonWriteNewItem(Context);
    
    __JsonWriteChar(Context, '[');
    Context->ContextClue = __Json_Context_Clue_First_Item;
    Context->Indention += DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
}

void JsonWriteEndArray(json_write_context* Context) {
    Context->Indention -= DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
    Context->ContextClue = __Json_Context_Clue_First_Item;
    __JsonWriteNewItem(Context);
    
    __JsonWriteChar(Context, ']');
}

void JsonWriteBool(json_write_context* Context, int Value) {
    __JsonWriteNewItem(Context);
    
    static const char TRUE_STR[]  = "true";
    static const char FALSE_STR[] = "false";
    if (Value)
        __JsonWriteN(Context, TRUE_STR,  sizeof(TRUE_STR ) - 1);
    else
        __JsonWriteN(Context, FALSE_STR, sizeof(FALSE_STR) - 1);
}

void JsonWriteS64(json_write_context* Context, json_s64 Value) {
    __JsonWriteNewItem(Context);
    
    char Buffer[32];
    int BufferLeft = sizeof(Buffer);
    json_s64 ValueIterator = llabs(Value);
    
    do {
        int Digit = (int)(ValueIterator % 10);
        Buffer[--BufferLeft] = '0' + Digit;
        ValueIterator /= 10;
    } while (ValueIterator); 
    
    if (Value < 0) {
        Buffer[--BufferLeft] = '-';
    }
    
    int NumDigits = sizeof(Buffer) - BufferLeft;
    __JsonWriteN(Context, Buffer + BufferLeft, NumDigits);
}

void JsonWriteF64(json_write_context* Context, json_f64 Value) {
    __JsonWriteNewItem(Context);
    
    char Buffer[128];
    size_t Size = snprintf(Buffer, sizeof(Buffer), "%f", Value);
    assert(Size + 1 != sizeof(Buffer)); // TODO: Should this check be here?
    
    __JsonWriteN(Context, Buffer, (int)Size);
}

void JsonWriteString(json_write_context* Context, const char* Str) {
    __JsonWriteNewItem(Context);
    
    __JsonWriteChar(Context, '\"');
    
    while (*Str) {
        switch (*Str) {
            case '\"': __JsonWriteN(Context, "\\\"", 2); break;
            case '\\': __JsonWriteN(Context, "\\\\", 2); break;
            case '\b': __JsonWriteN(Context, "\\b",  2); break;
            case '\f': __JsonWriteN(Context, "\\f",  2); break;
            case '\n': __JsonWriteN(Context, "\\n",  2); break;
            case '\r': __JsonWriteN(Context, "\\r",  2); break;
            case '\t': __JsonWriteN(Context, "\\t",  2); break;
            default: __JsonWriteChar(Context, *Str);
        }
        ++Str;
    }
    
    __JsonWriteChar(Context, '\"');
}

void JsonWriteNull(json_write_context* Context) {
    __JsonWriteNewItem(Context);
    
    static const char NULL_STR[] = "null";
    __JsonWriteN(Context, NULL_STR, sizeof(NULL_STR) - 1);
}


#endif // DIR_JSON_IMPLEMENTATION
#endif // DIR_JSON_H
