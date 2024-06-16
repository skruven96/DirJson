//
// dirjson.h - v0.0
//
// Copyright (c) Jesper Jansson and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for details.
//
//
// USAGE
//
// Include this header where ever you want to use it, but in one translation unit (i.e. c/c++ file) define the macro
//   #define DIR_JSON_IMPLEMENTATION
// before including this file. This will expand the actual implementation of this library.  
//
// READING
//
// To read you need a context, there are 3 ways of getting a context:
//   djReadFile(File)                // Reads from an already open file handle (the whole file is read into RAM)
//   djReadOpenAndReadFile(FilePath) // Opens a file and reads the whole file into RAM
//   djReadFromString(JsonString)    // Reads from a string containing json, needs to be null terminated.
//                                      And kept alive while the context is alive.
// Now one have a context and can read the json data, once done reading one should do:
//   djReadError(Context) // Returns a pointer to any error that has occured, null otherwise.
//                           Instead of checking for errors while parsing one can delay that until the end and assume
//                           that everything is great in the meantime. In case a error occurs while parsing all the
//                           reading functions will just return 0. Only the first error will be kept.
//   djReadDestroyContext(Context) // Frees up any resources used
//
// Reading integers, floating points, strings, booleans
//   djReadBool(Context) // Returns 1 if true and 0 if false, reports an error if neither
//   djReadS64(Context)  // Returns the integer value if a number, reports an error if not a whole number
//   djReadF64(Context)  // Returns the decimal value if a number, reports an error if not a number
//   djReadNull(Context) // Returns 1 if null, reports an error if it's not null
//   djReadEOF(Context)  // Returns 1 if eof is reach, reports an error otherwise
//
// Reading arrays is as simple as
//   while (djReadArray(Context)) {
//     // Use any djRead to read the value here
//   }
// djReadArray returns 1 until ']' is reached, incase of an empty array it returns 0 directly.
//
// Reading objects can be done in multiple ways.
//
// Looping through all keys and manually checking the keys.
//   json_string Key;
//   while (djReadKey(Context, &Key)) {
//     // Check the key and read it's value using any djRead method
//   }
// djReadKey returns 1 until '}' is reached, incase of an empty object it returns 0 directly.
//
// Using callbacks
//   TODO: Improve this, maybe support macros to easier parse directly into the members
//
// Expecting the keys, if the keys comes in a known order they can easily be verified and parsed
//   struct vec_t { int X, int Y };
//   struct vec_t Vec;
//   djReadExpectKey(Context, "x");
//   Vec.X = djReadS64(Context);
//   djReadExpectKey(Context, "y");
//   Vec.X = djReadS64(Context);
//   djReadObjectEnd();
// djReadExpectKey reads the next key and verifies checks that it matches the expected. Returns 0 if anything went wrong.
// djReadObjectEnd checks that the end of the object is reached. Returns 0 if anything went wrong.
// 
// WRITING
//
// TODO: Write documentation
//

#ifndef DIR_JSON_H
#define DIR_JSON_H
#include <stdio.h>

#ifdef __cplusplus
#define DIR_JSON_EXTERN extern "C"
#else
#define DIR_JSON_EXTERN
#endif

typedef struct dj_read_context dj_read_context;
typedef struct dj_write_context dj_write_context;
typedef struct dj_callbacks_object dj_callbacks_object;

// ===============================================================================
// Data Types
// ===============================================================================

typedef signed long long dj_s64;
typedef double           dj_f64;
typedef struct {
  size_t Length;
  const char* Data;
} dj_string;


// ===============================================================================
// Object Callbacks
// ===============================================================================

#define djOPTIONAL  0
#define djMANDATORY 1

typedef void(*dj_member_callback)(dj_read_context* Context, void* Ptr);
typedef void(*dj_key_callback)(dj_read_context* Context, void* Ptr, dj_string Key);

typedef struct {
  const char* Key;
  dj_member_callback Callback;
  int Mandatory;
} dj_member;

DIR_JSON_EXTERN dj_callbacks_object* djInitializeObject(dj_member* Members, int MemberCount, dj_key_callback UnknownKeyCallback);
DIR_JSON_EXTERN void djDestroyObject(dj_callbacks_object* Object);

// ===============================================================================
// Reading
// ===============================================================================

DIR_JSON_EXTERN dj_read_context* djReadReadFile(FILE* File);
DIR_JSON_EXTERN dj_read_context* djReadOpenAndReadFile(const char* FilePath);
DIR_JSON_EXTERN dj_read_context* djReadFromString(const char* JsonString);
DIR_JSON_EXTERN void djReadDestroyContext(dj_read_context* Context);

DIR_JSON_EXTERN void djReadReportErrorIfNoErrorExists(dj_read_context* Context, const char* Start, const char* OnePastLast,
                                      const char* FormatString, ...);
DIR_JSON_EXTERN const char* djReadError(dj_read_context* Context);

DIR_JSON_EXTERN void      djReadObjectUsingCallbacks(dj_read_context* Context, dj_callbacks_object* Object, void* Ptr);
DIR_JSON_EXTERN int       djReadKey(      dj_read_context* Context, dj_string* KeyOut);
DIR_JSON_EXTERN int       djReadExpectKey(dj_read_context* Context, const char* Key);
DIR_JSON_EXTERN int       djReadObjectEnd(dj_read_context* Context);

DIR_JSON_EXTERN int       djReadArray( dj_read_context* Context);
DIR_JSON_EXTERN int       djReadBool(  dj_read_context* Context);
DIR_JSON_EXTERN dj_s64    djReadS64(   dj_read_context* Context);
DIR_JSON_EXTERN dj_f64    djReadF64(   dj_read_context* Context);
DIR_JSON_EXTERN dj_string djReadString(dj_read_context* Context);
DIR_JSON_EXTERN void      djReadNull(  dj_read_context* Context);
DIR_JSON_EXTERN void      djReadEOF(   dj_read_context* Context);

// Returns true if the next value is of the respective type.
// Doesn't check that the value is legally formatted. For example djReadNextIsNull will return 1 for noll.
DIR_JSON_EXTERN int djReadNextIsObject(dj_read_context* Context);
DIR_JSON_EXTERN int djReadNextIsArray( dj_read_context* Context);
DIR_JSON_EXTERN int djReadNextIsBool(  dj_read_context* Context);
DIR_JSON_EXTERN int djReadNextIsNumber(dj_read_context* Context);
DIR_JSON_EXTERN int djReadNextIsString(dj_read_context* Context);
DIR_JSON_EXTERN int djReadNextIsNull(  dj_read_context* Context);


// ===============================================================================
// Writing
// ===============================================================================

typedef void(*dj_write_callback)(dj_write_context*, char* Data, int Size);

DIR_JSON_EXTERN dj_write_context* djWriteInitializeContextTargetString(int StartBufferSize);
DIR_JSON_EXTERN dj_write_context* djWriteInitializeContextTargetFile(FILE* File, int BufferSize);
DIR_JSON_EXTERN dj_write_context* djWriteInitializeContextTargetFilePath(const char* FilePath, int BufferSize);
DIR_JSON_EXTERN dj_write_context* djWriteInitializeContextTargetCustom(dj_write_callback Callback, int BufferSize);

DIR_JSON_EXTERN void djWriteSetPrettyPrint(dj_write_context* Context, int ShouldPrettyPrint);

DIR_JSON_EXTERN char* djWriteFinalize(      dj_write_context* Context);
DIR_JSON_EXTERN void  djWriteDestroyContext(dj_write_context* Context);

DIR_JSON_EXTERN void djWriteStartObject(dj_write_context* Context);
DIR_JSON_EXTERN void djWriteKey(        dj_write_context* Context, const char* Key);
DIR_JSON_EXTERN void djWriteEndObject(  dj_write_context* Context);
DIR_JSON_EXTERN void djWriteStartArray( dj_write_context* Context);
DIR_JSON_EXTERN void djWriteEndArray(   dj_write_context* Context);
DIR_JSON_EXTERN void djWriteBool(       dj_write_context* Context, int Value);
DIR_JSON_EXTERN void djWriteS64(        dj_write_context* Context, dj_s64 Value);
DIR_JSON_EXTERN void djWriteF64(        dj_write_context* Context, dj_f64 Value);
DIR_JSON_EXTERN void djWriteString(     dj_write_context* Context, const char* Str);
DIR_JSON_EXTERN void djWriteNull(       dj_write_context* Context);


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

struct dj_callbacks_object {
  int SlotsCount, MandatoryMemberCount;
  dj_key_callback UnknownKeyCallback;
  dj_member_callback* MemberCallbacks;
  int* MemberKeys;
};

struct dj_read_context {
  char* JsonDataOwnagePtr;
  const char* JsonData;
  const char* CurrentChar;
  
  const char* StartOfCurrentLine;
  int LineNumber;
  int ShouldReadValueNext; // NOTE: If false a comma, }, ] or EOF should be read. Else a value.
  
  const char* Error;
  
  int StringBufferSize;
  char* StringBuffer;
};

struct dj_write_context {
  int ShouldCloseFile;
  FILE* TargetFile;
  dj_write_callback Callback;
  
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

static unsigned int _djHashString(const char* String) {
  unsigned int Hash = 5381;
  int Char;
  
  while (Char = *(String++))
    Hash = ((Hash << 5) + Hash) + Char;
  
  return Hash;
}

// ===============================================================================
// Object Callbacks Implementation
// ===============================================================================

const int _dj_Mandatory_Flag = (1 << 31);

static void ReportUnkownMemberCallback(dj_read_context* Context, void* Ptr, dj_string Key) {
  const char* KeyEndPtr = Context->CurrentChar - 1;
  while (*KeyEndPtr   != '"') --KeyEndPtr;
  const char* KeyStartPtr = KeyEndPtr - 1;
  while (*KeyStartPtr != '"') --KeyStartPtr;
  
  char* KeyCopy = malloc(Key.Length + 1);
  memcpy(KeyCopy, Key.Data, Key.Length + 1);
  djReadReportErrorIfNoErrorExists(Context, KeyStartPtr, KeyEndPtr + 1,
                                   "Unkown member encountered (Key '%s'. )", KeyCopy);
  free(KeyCopy);
}

dj_callbacks_object* djInitializeObject(dj_member* Members, int MemberCount, dj_key_callback UnknownKeyCallback) {
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
  
  size_t TotalBytes = sizeof(dj_callbacks_object);
  TotalBytes += sizeof(dj_member_callback) * SlotsCount;
  TotalBytes += sizeof(int)                  * SlotsCount;
  TotalBytes += StringsByteCount;
  
  dj_callbacks_object* Result = calloc(TotalBytes, 1);
  Result->SlotsCount = SlotsCount;
  Result->MandatoryMemberCount = MandatoryMemberCount;
  Result->UnknownKeyCallback = UnknownKeyCallback ? UnknownKeyCallback : ReportUnkownMemberCallback;
  Result->MemberCallbacks = (dj_member_callback*)((char*)Result + sizeof(dj_callbacks_object));
  Result->MemberKeys      = (int*                 )&Result->MemberCallbacks[SlotsCount];
  assert(((intptr_t)Result->MemberCallbacks % sizeof(Result->MemberCallbacks[0])) == 0);
  assert(((intptr_t)Result->MemberKeys      % sizeof(Result->MemberKeys[0])) == 0);
  
  char* StringCopyCurrentChar = (char*)&Result->MemberKeys[SlotsCount];
  
  for (int MemberIndex = 0; MemberIndex < MemberCount; MemberIndex++) {
    dj_member* Member = &Members[MemberIndex];
    unsigned int Hash = _djHashString(Member->Key);
    
    while (1) {
      unsigned int Index = Hash % SlotsCount;
      
      if (!Result->MemberKeys[Index]) {
        int Offset = (int)(StringCopyCurrentChar - (char*)Result);
        
        size_t Length = strlen(Member->Key);
        memcpy(StringCopyCurrentChar, Member->Key, Length + 1);
        StringCopyCurrentChar += Length + 1;
        
        Result->MemberKeys[Index]= Offset | (Member->Mandatory ? _dj_Mandatory_Flag : 0);
        Result->MemberCallbacks[Index] = Member->Callback;
        
        break;
      }
      
      Hash += 1;
    }
  }
  
  assert(StringCopyCurrentChar == ((char*)Result + TotalBytes));
  
  return Result;
}

void djDestroyObject(dj_callbacks_object* Object) {
  free(Object);
}


// ===============================================================================
// Read Implementation
// ===============================================================================

static void _djEatWhiteSpaces(dj_read_context* Context) {
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

static int _djEatCharacter(dj_read_context* Context, char Character) {
  if (*Context->CurrentChar == Character) {
    Context->CurrentChar += 1;
    return 1;
  }
  return 0;
}

static void _djIncreaseStringBufferSize(dj_read_context* Context) {
  Context->StringBufferSize *= 2;
  Context->StringBuffer = realloc(Context->StringBuffer, Context->StringBufferSize);
  assert(Context->StringBuffer && "JSON: Out of memory. ");
  // TODO: Handle this?
}

static int _djPutCharInBuffer(dj_read_context* Context, int LengthBefore, char Char) {
  if (LengthBefore >= Context->StringBufferSize) {
    _djIncreaseStringBufferSize(Context);
  }
  Context->StringBuffer[LengthBefore] = Char;
  return LengthBefore + 1;
}

static int _djPutStringInBuffer(dj_read_context* Context, int LengthBefore, const char* String) {
  while (*String) {
    LengthBefore = _djPutCharInBuffer(Context, LengthBefore, *String);
    String += 1;
  }
  return LengthBefore;
}

static void _djInitializationOutOfMemoryError(dj_read_context* Context, const char* Error) {
  Context->Error = Error;
  Context->JsonData    = "\0";
  Context->CurrentChar = Context->JsonData;
}

static dj_read_context* _djCreateReadContext() {
  dj_read_context* Context = malloc(sizeof(dj_read_context));
  assert(Context);
  
  Context->JsonDataOwnagePtr = 0;
  Context->JsonData     = "\0";
  Context->CurrentChar  = Context->JsonData;
  
  Context->StartOfCurrentLine = Context->JsonData;
  Context->LineNumber = 1;
  Context->ShouldReadValueNext = 1;
  
  Context->Error = 0;
  
  Context->StringBufferSize = 256;
  Context->StringBuffer = malloc(Context->StringBufferSize);
  if (!Context->StringBuffer) {
    _djInitializationOutOfMemoryError(Context, "Couldn't allocate string buffer. ");
    return Context;
  }
  
  _djEatWhiteSpaces(Context);
  return Context;
}

static void ReadFile(dj_read_context* Context, FILE* File) {
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
    _djInitializationOutOfMemoryError(Context, "Couldn't read file. ");
    free(Data);
    return;
  }
  Data[FileSize] = '\0';
  
  Context->JsonDataOwnagePtr  = Data;
  Context->JsonData           = Data;
  Context->CurrentChar        = Data;
  Context->StartOfCurrentLine = Data;
  
  _djEatWhiteSpaces(Context);
}

dj_read_context* djReadReadFile(FILE* File) {
  dj_read_context* Context = _djCreateReadContext();
  if (!djReadError(Context))
    ReadFile(Context, File);
  return Context;
}

dj_read_context* djReadOpenAndReadFile(const char* FilePath) {
  dj_read_context* Context = _djCreateReadContext();
  
  if (djReadError(Context))
    return Context;
  
  FILE* File = fopen(FilePath, "r");
  if (!File) {
    _djInitializationOutOfMemoryError(Context, "Failed to open file. ");
    return Context;
  }
  
  ReadFile(Context, File);
  
  if (fclose(File)) {
    Context->Error = "Failed to close file. ";
  }
  
  return Context;
}

dj_read_context* djReadFromString(const char* JsonString) {
  dj_read_context* Context = _djCreateReadContext();
  
  if (djReadError(Context))
    return Context;
  
  Context->JsonData           = JsonString;
  Context->CurrentChar        = JsonString;
  Context->StartOfCurrentLine = JsonString;
  
  _djEatWhiteSpaces(Context);
  
  return Context;
}

void djReadDestroyContext(dj_read_context* Context) {
  free(Context->StringBuffer);
  free(Context->JsonDataOwnagePtr);
}

void djReadReportErrorIfNoErrorExists(dj_read_context* Context, const char* Start, const char* OnePastLast, 
                                      const char* FormatString, ...) {
  if (Context->Error)
    return;
  
  int Line   = !Start ? -1 : Context->LineNumber;
  int Column = !Start ? -1 : (int)(Start - Context->StartOfCurrentLine) + 1;
  
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
        AmountWritten = _djPutCharInBuffer(Context, AmountWritten, '\\');
        AmountWritten = _djPutCharInBuffer(Context, AmountWritten, '%');
        PrefixIterator += 2;
      } else {
        AmountWritten = _djPutCharInBuffer(Context, AmountWritten, *PrefixIterator);
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
    
    _djIncreaseStringBufferSize(Context);
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
    
    if (AmountSearchedForwards + AmountSearchedBackwards > DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT) {
      if (AmountSearchedForwards > DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT / 2) {
        EndOneBefore = OnePastLast + DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT / 2;
      }
      if (AmountSearchedBackwards > DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT / 2) {
        EndOneBefore = Start - DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT / 2;
      }
    }
    
    AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "\n > ");
    if (AmountSearchedBackwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
      AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "...");
    
    const char* Iterator = StartFrom;
    while (*Iterator && Iterator < EndOneBefore) {
      AmountWritten = _djPutCharInBuffer(Context, AmountWritten, *Iterator);
      Iterator += 1;
    }
    
    if (AmountSearchedForwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
      AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "...");
    
    if (DIR_JSON_ERROR_HIGHLIGHT_CARROT) {
      AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "\n > ");
      if (AmountSearchedBackwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
        AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "...");
      
      const char* Iterator = StartFrom;
      while (Iterator < EndOneBefore) {
        if (Iterator >= Start && Iterator < OnePastLast) {
          AmountWritten = _djPutCharInBuffer(Context, AmountWritten, DIR_JSON_ERROR_HIGHLIGHT_CARROT);
        } else {
          AmountWritten = _djPutCharInBuffer(Context, AmountWritten, *Iterator == '\t' ? '\t' : ' ');
        }
        Iterator += 1;
      }
      
      if (AmountSearchedForwards == DIR_JSON_ERROR_MAX_SHOWN_CONTENT_COUNT)
        AmountWritten = _djPutStringInBuffer(Context, AmountWritten, "...");
    }
  }
  
  AmountWritten = _djPutCharInBuffer(Context, AmountWritten, '\0');
  Context->Error = Context->StringBuffer;
  Context->JsonData    = "\0";
  Context->CurrentChar = Context->JsonData;
}

const char* djReadError(dj_read_context* Context) {
  return Context->Error;
}

int djReadKey(dj_read_context* Context, dj_string* KeyOut) {
  if (Context->ShouldReadValueNext) {
    if (!_djEatCharacter(Context, '{')) {
      djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                       "Expected a object. ");
      return 0;
    }
    _djEatWhiteSpaces(Context);
    
    if (_djEatCharacter(Context, '}')) {
      Context->ShouldReadValueNext = 0;
      _djEatWhiteSpaces(Context);
      return 0;
    }
    
    // Context->ShouldReadValueNext = 0; // This is overwritten below to allow djReadString to function
  } else if (_djEatCharacter(Context, '}')) {
    _djEatWhiteSpaces(Context);
    return 0;
  } else if (!_djEatCharacter(Context, ',')) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                     "Expected a ',' or '}'. ");
    return 0;
  }
  _djEatWhiteSpaces(Context);
  
  Context->ShouldReadValueNext = 1;
  *KeyOut = djReadString(Context);
  if (!KeyOut->Data) {
    return 0;
  }
  
  if (!_djEatCharacter(Context, ':')) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                     "A colon needs to follow the key for each member.");
    return 0;
  }
  _djEatWhiteSpaces(Context);
  
  Context->ShouldReadValueNext = 1;
  return 1;
}

int djReadExpectKey(dj_read_context* Context, const char* ExpectedKey) {
  dj_string Key;
  if (djReadKey(Context, &Key) == 0) {
    return 0;
  }
  
  if (strcmp(Key.Data, ExpectedKey) != 0) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                     "Unexpected key found, expected '%s' got '%s'.", ExpectedKey, Key.Data);
    return 0;
  }
  
  return 1;
}

int djReadObjectEnd(dj_read_context* Context) {
  if (!_djEatCharacter(Context, '}')) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                     "Expected end of object.");
    return 0;
  }
  
  _djEatWhiteSpaces(Context);
  return 1;
}

void djReadObjectUsingCallbacks(dj_read_context* Context, dj_callbacks_object* Object, void* Ptr) {
  int MandatoryMembersFound = 0;
  dj_string Key;
  while (djReadKey(Context, &Key)) {
    unsigned int Hash = _djHashString(Key.Data);
    
    unsigned int SlotIndex =  Hash % Object->SlotsCount;
    
    int Found = 0;
    int IsMandatory;
    while (Object->MemberKeys[SlotIndex]) {
      IsMandatory = Object->MemberKeys[SlotIndex] & _dj_Mandatory_Flag;
      char* SlotKey = (char*)Object + (Object->MemberKeys[SlotIndex] & ~_dj_Mandatory_Flag);
      
      if (strcmp(Key.Data, SlotKey) == 0) {
        Found = 1;
        break;
      }
      
      SlotIndex = (SlotIndex + 1) % Object->SlotsCount;
    }
    
    if (Found) {
      if (IsMandatory)
        MandatoryMembersFound += 1;
      Object->MemberCallbacks[SlotIndex](Context, Ptr);
    } else {
      Object->UnknownKeyCallback(Context, Ptr, Key);
    }
  }
  
  if (MandatoryMembersFound != Object->MandatoryMemberCount) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar - 1, Context->CurrentChar, 
                                     "Not all mandatory members where found. ");
    return;
  }
}

int djReadArray(dj_read_context* Context) {
  if (Context->ShouldReadValueNext) {
    if (!_djEatCharacter(Context, '[')) {
      djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                       "Expected an array. ");
      return 0;
    }
    _djEatWhiteSpaces(Context);
    if (_djEatCharacter(Context, ']')) {
      Context->ShouldReadValueNext = 0;
      _djEatWhiteSpaces(Context);
      return 0;
    }
  } else if (_djEatCharacter(Context, ']')) {
    _djEatWhiteSpaces(Context);
    return 0;
  } else if (!_djEatCharacter(Context, ',')) {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1,
                                     "Expected a ',' or ']'. ");
    return 0;
  }
  
  Context->ShouldReadValueNext = 1;
  _djEatWhiteSpaces(Context);
  return 1;
}

int djReadBool(dj_read_context* Context) {
  assert(Context->ShouldReadValueNext);
  Context->ShouldReadValueNext = 0;
  
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
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                     "Expected a boolean ('true' or 'false'. )");
    Result = 0;
  }
  _djEatWhiteSpaces(Context);
  
  return Result;
}

dj_s64 djReadS64(dj_read_context* Context) {
  assert(Context->ShouldReadValueNext);
  Context->ShouldReadValueNext = 0;
  
  const char* CurrentChar = Context->CurrentChar;
  dj_s64 Value = 0;
  
  int IsNegative = *CurrentChar == '-';
  if (IsNegative) ++CurrentChar;
  
  if (*CurrentChar < '0' || *CurrentChar > '9') {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                     "Expected a integer, needs to start with a digit (0-9). ");
    return 0;
  }
  
  while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
    Value = Value * 10 + (*CurrentChar - '0');
    CurrentChar += 1;
  }
  
  if (*CurrentChar == '.') {
    djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
                                     "Expected a integer but got a decimal point. ");
    return 0;
  }
  
  if (*CurrentChar == 'e' || *CurrentChar == 'E') {
    CurrentChar += 1;
    if (*CurrentChar == '-') {
      djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
                                       "Expected a integer, negative exponent is not allowed for integers. ");
      return 0;
    } else if (*CurrentChar == '+') {
      CurrentChar += 1;
    }
    
    if (*CurrentChar < '0' || *CurrentChar > '9') {
      djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
                                       "The exponent needs to contain atleast one digit (0-9). ");
      return 0;
    }
    
    unsigned Exponent = 0;
    
    while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
      Exponent = Exponent * 10 + (*CurrentChar - '0');
      ++CurrentChar;
    }
    
    dj_s64 Base = 10;
    while (Exponent) {
      if (Exponent % 2)
        Value *= Base;
      Exponent /= 2;
      Base *= Base;
    }
  }
  
  Context->CurrentChar = CurrentChar;
  _djEatWhiteSpaces(Context);
  return IsNegative ? -Value : Value;
}

dj_f64 djReadF64(dj_read_context* Context) {
  assert(Context->ShouldReadValueNext);
  Context->ShouldReadValueNext = 0;
  
  const char* CurrentChar = Context->CurrentChar;
  
  if (*CurrentChar == '-') {
    CurrentChar += 1;
  }
  
  if (*CurrentChar < '0' || *CurrentChar > '9') {
    djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
                                     "Expected a number, needs to start with a digit (0-9). ");
    return 0;
  }
  
  while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
    CurrentChar += 1;
  }
  
  if (*CurrentChar == '.') {
    CurrentChar += 1;
    
    if (*CurrentChar < '0' || *CurrentChar > '9') {
      djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
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
      djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1, 
                                       "Exponent is empty, needs to contain atleast one digit (0-9). ");
      return 0;
    }
    
    while (*CurrentChar >= '0' && *CurrentChar <= '9')  {
      ++CurrentChar;
    }
  }
  
  char* EndPtr;
  dj_f64 Result = strtod(Context->CurrentChar, &EndPtr); 
  assert(EndPtr == CurrentChar); // TODO: Look up if this is valid, for now keep it to test. 
  Context->CurrentChar = CurrentChar;
  _djEatWhiteSpaces(Context);
  return Result;
}

dj_string djReadString(dj_read_context* Context) {
  assert(Context->ShouldReadValueNext);
  Context->ShouldReadValueNext = 0;
  
  const dj_string ErrorResult = { 0, 0 };
  const char* CurrentChar = Context->CurrentChar;
  int Length = 0;
  
  if (*CurrentChar != '"') {
    djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
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
            djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                             "A unicode escape sequence needs to be followed by 4 hex digits. ");
            return ErrorResult;
          }
          ++CurrentChar;
        }
        
        if (Value >= 0x1000) {
          if (Value >= 0x2000) {
            djReadReportErrorIfNoErrorExists(Context,CurrentChar - 4, CurrentChar,
                                             "Given unicode was to large. ");
            return ErrorResult;
          }
          
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0 ) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 6 ) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 12) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0xF0 | ((Char >> 18) & 0x07));
        } else if (Value >= 0x800) {
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0 ) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 6 ) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0xE0 | ((Char >> 12) & 0x0F));
        } else if (Value >= 0x80) {
          Length = _djPutCharInBuffer(Context, Length, 0x80 | ((Char >> 0) & 0x3F));
          Length = _djPutCharInBuffer(Context, Length, 0xC0 | ((Char >> 6) & 0x1F));
        } else {
          Length = _djPutCharInBuffer(Context, Length, Char);
        }
        continue;
      } else {
        djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                         "Unrecognised escape character. ");
        return ErrorResult;
      }
    } else if (Char == '\n' || Char == '\r') {
      djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                       "Reached end of the line before closing the string. ");
      return ErrorResult;
    }
    
    Length = _djPutCharInBuffer(Context, Length, Char);
    CurrentChar += 1;
  }
  
  if (Char != '"') {
    djReadReportErrorIfNoErrorExists(Context, CurrentChar, CurrentChar + 1,
                                     "Reached end of the file before closing the string. ");
    return ErrorResult;
  }
  CurrentChar += 1;
  
  Context->CurrentChar = CurrentChar;
  _djEatWhiteSpaces(Context);
  
  Length = _djPutCharInBuffer(Context, Length, '\0');
  
  dj_string Result;
  Result.Data = Context->StringBuffer;
  Result.Length = Length;
  return Result;
}

void djReadNull(dj_read_context* Context) {
  assert(Context->ShouldReadValueNext);
  Context->ShouldReadValueNext = 0;
  
  static const char NULL_STR[]  = "null";
  if (memcmp(Context->CurrentChar, NULL_STR, sizeof(NULL_STR) - 1) == 0) {
    Context->CurrentChar += sizeof(NULL_STR) - 1;
  } else {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                     "Expected 'null'. ", Context->LineNumber);
  }
  _djEatWhiteSpaces(Context);
}

void djReadEOF(dj_read_context* Context) {
  if (*Context->CurrentChar != '\0') {
    djReadReportErrorIfNoErrorExists(Context, Context->CurrentChar, Context->CurrentChar + 1, 
                                     "Unexpected content at end of file. ");
  }
}

int djReadNextIsObject(dj_read_context* Context) { 
  return *Context->CurrentChar == '{';  
}

int djReadNextIsArray( dj_read_context* Context) { 
  return *Context->CurrentChar == '[';  
}

int djReadNextIsBool(  dj_read_context* Context) { 
  return *Context->CurrentChar == 't' || *Context->CurrentChar == 'f';  
}

int djReadNextIsNumber(dj_read_context* Context) { 
  return (*Context->CurrentChar >= '0' && *Context->CurrentChar <= '9') || 
    *Context->CurrentChar == '-';  
}

int djReadNextIsString(dj_read_context* Context) { 
  return *Context->CurrentChar == '"';  
}

int djReadNextIsNull(  dj_read_context* Context) { 
  return *Context->CurrentChar == 'n';  
}

// ===============================================================================
// Write Implementation
// ===============================================================================

const char _dj_Spaces_Array[]= "                                                             ";
enum {
  _dj_Context_Clue_First_Item,
  _dj_Context_Clue_Member_Value,
  _dj_Context_Clue_Write_Comma
};

static void _djFlushBuffer(dj_write_context* Context) {
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

static void _djWriteChar(dj_write_context* Context, char Char) {
  if (Context->Used == Context->Size) {
    _djFlushBuffer(Context);
  }
  Context->Buffer[Context->Used++] = Char;
}

static void _djWriteN(dj_write_context* Context, const char* Data, int Count) {
  int AmountWritten = 0;
  while (1) {
    int AmountToWrite = min(Count - AmountWritten, Context->Size - Context->Used);
    memcpy(Context->Buffer + Context->Used, Data + AmountWritten, AmountToWrite);
    Context->Used += AmountToWrite;
    AmountWritten += AmountToWrite;
    if (AmountWritten == Count) {
      break;
    } else {
      _djFlushBuffer(Context);
    }
  }
}

static void _djWriteNewItem(dj_write_context* Context) {
  if (Context->PrettyPrint) {
    if (Context->IsRootValue) {
      Context->IsRootValue = 0;
    } else {
      if (Context->ContextClue != _dj_Context_Clue_Member_Value) {
        if (Context->ContextClue == _dj_Context_Clue_Write_Comma)
          _djWriteChar(Context, ',');
        _djWriteChar(Context, '\n');
        int SpacesLeftToWrite = Context->Indention;
        while (SpacesLeftToWrite > 0) {
          int SpacesWritten = min(SpacesLeftToWrite, sizeof(_dj_Spaces_Array) - 1);
          _djWriteN(Context, _dj_Spaces_Array, SpacesWritten);
          SpacesLeftToWrite -= SpacesWritten;
        }
      } else {
        _djWriteChar(Context, ' ');
      }
    }
  } else {
    switch (Context->ContextClue) {
      case _dj_Context_Clue_First_Item: {
      } break;
      case _dj_Context_Clue_Member_Value: {
      } break;
      case _dj_Context_Clue_Write_Comma: {
        _djWriteChar(Context, ',');
      } break;
    }
  }
  Context->ContextClue = _dj_Context_Clue_Write_Comma;
}

static dj_write_context* _djCreateWriteContext(int BufferSize) {
  dj_write_context* Context = calloc(1, sizeof(dj_write_context));
  Context->ContextClue = _dj_Context_Clue_First_Item;
  Context->IsRootValue = 1;
  
  Context->Size   = BufferSize > 0 ? BufferSize : 512;
  Context->Buffer = malloc(Context->Size);
  
  return Context;
}

dj_write_context* djWriteInitializeContextTargetString(int StartBufferSize) {
  dj_write_context* Context = _djCreateWriteContext(StartBufferSize);
  
  Context->Size   = 128;
  Context->Buffer = malloc(Context->Size);
  
  return Context;
}

dj_write_context* djWriteInitializeContextTargetFile(FILE* File, int BufferSize) {
  dj_write_context* Context = _djCreateWriteContext(BufferSize);
  
  Context->TargetFile  = File;
  
  return Context;
}

dj_write_context* djWriteInitializeContextTargetFilePath(const char* FilePath, int BufferSize) {
  FILE* File = fopen(FilePath, "w");
  
  dj_write_context* Context = _djCreateWriteContext(BufferSize);
  
  Context->TargetFile      = File;
  Context->ShouldCloseFile = File != 0;
  Context->Error           = File != 0 ? 0 : "Could not open file. ";
  
  return Context;
}

dj_write_context* djWriteInitializeContextTargetCustom(dj_write_callback Callback, int BufferSize)  {
  dj_write_context* Context = _djCreateWriteContext(BufferSize);
  
  Context->Callback = Callback;
  
  return Context;
}

void djWriteSetPrettyPrint(dj_write_context* Context, int ShouldPrettyPrint) {
  Context->PrettyPrint = ShouldPrettyPrint;
}

char* djWriteFinalize(dj_write_context* Context) {
  char* Result = 0;
  if (Context->TargetFile) {
    _djFlushBuffer(Context);
    if (Context->ShouldCloseFile) {
      fclose(Context->TargetFile);
    }
  } else if (Context->Callback) {
    _djWriteChar(Context, '\0');
    Context->Callback(Context, Context->Buffer, Context->Used);
  } else {
    _djWriteChar(Context, '\0');
    Result = Context->Buffer;
    Context->Buffer = 0;
  }
  return Result;
}

void djWriteDestroyContext(dj_write_context* Context) {
  free(Context->Buffer);
  free(Context);
}

void djWriteStartObject(dj_write_context* Context) {
  _djWriteNewItem(Context);
  
  _djWriteChar(Context, '{');
  Context->ContextClue = _dj_Context_Clue_First_Item;
  Context->Indention += DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
}

void djWriteKey(dj_write_context* Context, const char* Key) {
  djWriteString(Context, Key);
  _djWriteChar(Context, ':');
  Context->ContextClue = _dj_Context_Clue_Member_Value;
}

void djWriteEndObject(dj_write_context* Context) {
  Context->Indention -= DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
  Context->ContextClue = _dj_Context_Clue_First_Item;
  _djWriteNewItem(Context);
  
  _djWriteChar(Context, '}');
}

void djWriteStartArray(dj_write_context* Context) {
  _djWriteNewItem(Context);
  
  _djWriteChar(Context, '[');
  Context->ContextClue = _dj_Context_Clue_First_Item;
  Context->Indention += DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
}

void djWriteEndArray(dj_write_context* Context) {
  Context->Indention -= DIR_JSON_WRITE_INDENTION_SPACE_COUNT;
  Context->ContextClue = _dj_Context_Clue_First_Item;
  _djWriteNewItem(Context);
  
  _djWriteChar(Context, ']');
}

void djWriteBool(dj_write_context* Context, int Value) {
  _djWriteNewItem(Context);
  
  static const char TRUE_STR[]  = "true";
  static const char FALSE_STR[] = "false";
  if (Value)
    _djWriteN(Context, TRUE_STR,  sizeof(TRUE_STR ) - 1);
  else
    _djWriteN(Context, FALSE_STR, sizeof(FALSE_STR) - 1);
}

void djWriteS64(dj_write_context* Context, dj_s64 Value) {
  _djWriteNewItem(Context);
  
  char Buffer[32];
  int BufferLeft = sizeof(Buffer);
  dj_s64 ValueIterator = llabs(Value);
  
  do {
    int Digit = (int)(ValueIterator % 10);
    Buffer[--BufferLeft] = '0' + Digit;
    ValueIterator /= 10;
  } while (ValueIterator); 
  
  if (Value < 0) {
    Buffer[--BufferLeft] = '-';
  }
  
  int NumDigits = sizeof(Buffer) - BufferLeft;
  _djWriteN(Context, Buffer + BufferLeft, NumDigits);
}

void djWriteF64(dj_write_context* Context, dj_f64 Value) {
  _djWriteNewItem(Context);
  
  char Buffer[128];
  size_t Size = snprintf(Buffer, sizeof(Buffer), "%f", Value);
  assert(Size + 1 != sizeof(Buffer)); // TODO: Should this check be here?
  
  _djWriteN(Context, Buffer, (int)Size);
}

void djWriteString(dj_write_context* Context, const char* Str) {
  _djWriteNewItem(Context);
  
  _djWriteChar(Context, '\"');
  
  while (*Str) {
    switch (*Str) {
      case '\"': _djWriteN(Context, "\\\"", 2); break;
      case '\\': _djWriteN(Context, "\\\\", 2); break;
      case '\b': _djWriteN(Context, "\\b",  2); break;
      case '\f': _djWriteN(Context, "\\f",  2); break;
      case '\n': _djWriteN(Context, "\\n",  2); break;
      case '\r': _djWriteN(Context, "\\r",  2); break;
      case '\t': _djWriteN(Context, "\\t",  2); break;
      default: _djWriteChar(Context, *Str);
    }
    ++Str;
  }
  
  _djWriteChar(Context, '\"');
}

void djWriteNull(dj_write_context* Context) {
  _djWriteNewItem(Context);
  
  static const char NULL_STR[] = "null";
  _djWriteN(Context, NULL_STR, sizeof(NULL_STR) - 1);
}


#endif // DIR_JSON_IMPLEMENTATION
#endif // DIR_JSON_H
