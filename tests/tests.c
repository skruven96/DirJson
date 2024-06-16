
#define DIR_JSON_IMPLEMENTATION
#include "../source/dirjson.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

static ReportErrorIfNotTrue(int IsTrue, const char* Code, int Line) {
  if (!IsTrue) {
    printf("Expected '%s' to be true, line %d\n", Code, Line);
  }
}

#define EXPECT_TRUE(Condition) ReportErrorIfNotTrue((Condition), #Condition , __LINE__)

typedef struct {
  const char* Name;
  const char* Json;
  const char* Carrot;
  const char* Message;
  void (*Function)(dj_read_context* Context);
} test_error;

typedef struct {
  const char* Name;
  const char* Json;
  void (*Function)(dj_read_context* Context);
} test_success;

static const char TestReadExpectedArray__Json[]    = "123";
static const char TestReadExpectedArray__Carrot[]  = "^  ";
static const char TestReadExpectedArray__Message[] = "Expected an array. ";
void TestReadExpectedArray(dj_read_context* Context) {
  djReadArray(Context);
}

static const char TestReadBool__Json[]    = "nah";
static const char TestReadBool__Carrot[]  = "^  ";
static const char TestReadBool__Message[] = "Expected a boolean ('true' or 'false'. )";
void TestReadBool(dj_read_context* Context) {
  djReadBool(Context);
}

static const char TestReadS64IllegalStart__Json[]    = ".123";
static const char TestReadS64IllegalStart__Carrot[]  = "^   ";
static const char TestReadS64IllegalStart__Message[] = "Expected a integer, needs to start with a digit (0-9). ";
void TestReadS64IllegalStart(dj_read_context* Context) {
  djReadS64(Context);
}

static const char TestReadS64GotDecimal__Json[]    = "1.23";
static const char TestReadS64GotDecimal__Carrot[]  = " ^  ";
static const char TestReadS64GotDecimal__Message[] = "Expected a integer but got a decimal point. ";
void TestReadS64GotDecimal(dj_read_context* Context) {
  djReadS64(Context);
}

static const char TestReadS64NegativeExponent__Json[]    = "123e-123";
static const char TestReadS64NegativeExponent__Carrot[]  = "    ^   ";
static const char TestReadS64NegativeExponent__Message[] = "Expected a integer, negative exponent is not allowed for integers. ";
void TestReadS64NegativeExponent(dj_read_context* Context) {
  djReadS64(Context);
}

static const char TestReadS64EmptyExponent__Json[]    = "123e";
static const char TestReadS64EmptyExponent__Carrot[]  = "    ^";
static const char TestReadS64EmptyExponent__Message[] = "The exponent needs to contain atleast one digit (0-9). ";
void TestReadS64EmptyExponent(dj_read_context* Context) {
  djReadS64(Context);
}

static const char TestReadF64IllegalStart__Json[]    = "-e12";
static const char TestReadF64IllegalStart__Carrot[]  = " ^  ";
static const char TestReadF64IllegalStart__Message[] = "Expected a number, needs to start with a digit (0-9). ";
void TestReadF64IllegalStart(dj_read_context* Context) {
  djReadF64(Context);
}

static const char TestReadF64EmptyFraction__Json[]    = "123.e123";
static const char TestReadF64EmptyFraction__Carrot[]  = "    ^   ";
static const char TestReadF64EmptyFraction__Message[] = "Fraction is empty, needs to contain atleast one digit (0-9). ";
void TestReadF64EmptyFraction(dj_read_context* Context) {
  djReadF64(Context);
}

static const char TestReadF64EmptyExponent__Json[]    = "123.123e-+";
static const char TestReadF64EmptyExponent__Carrot[]  = "         ^";
static const char TestReadF64EmptyExponent__Message[] = "Exponent is empty, needs to contain atleast one digit (0-9). ";
void TestReadF64EmptyExponent(dj_read_context* Context) {
  djReadF64(Context);
}

static const char TestReadStringNotAString__Json[]    = "'Hello, world!'";
static const char TestReadStringNotAString__Carrot[]  = "^              ";
static const char TestReadStringNotAString__Message[] = "Expected a string. ";
void TestReadStringNotAString(dj_read_context* Context) {
  djReadString(Context);
}

static const char TestReadStringTooFewHex__Json[]    = "\"Hello\\uABCK, world!\"";
static const char TestReadStringTooFewHex__Carrot[]  = "           ^         ";
static const char TestReadStringTooFewHex__Message[] = "A unicode escape sequence needs to be followed by 4 hex digits. ";
void TestReadStringTooFewHex(dj_read_context* Context) {
  djReadString(Context);
}

static const char TestReadStringTooBigUnicode__Json[]    = "\"Hello\\u2001, world!\"";
static const char TestReadStringTooBigUnicode__Carrot[]  = "        ^^^^         ";
static const char TestReadStringTooBigUnicode__Message[] = "Given unicode was to large. ";
void TestReadStringTooBigUnicode(dj_read_context* Context) {
  djReadString(Context);
}

static const char TestReadStringIllegalEscapeSequence__Json[]    = "\"Hello\\h, world!\"";
static const char TestReadStringIllegalEscapeSequence__Carrot[]  = "       ^         ";
static const char TestReadStringIllegalEscapeSequence__Message[] = "Unrecognised escape character. ";
void TestReadStringIllegalEscapeSequence(dj_read_context* Context) {
  djReadString(Context);
}

static const char TestReadEmptyObject__Json[] = " { }";
void TestReadEmptyObject(dj_read_context* Context) {
  dj_string Key;
  djReadKey(Context, &Key);
}

static const char TestReadObject__Json[] = " { \"key1\": 13, \"\": true, \"afdsf\": [ ] }";
void TestReadObject(dj_read_context* Context) {
  EXPECT_TRUE(djReadExpectKey(Context, "key1") == 1);
  EXPECT_TRUE(djReadS64(Context) == 13);
  EXPECT_TRUE(djReadExpectKey(Context, "") == 1);
  EXPECT_TRUE(djReadBool(Context) == 1);
  EXPECT_TRUE(djReadExpectKey(Context, "afdsf") == 1);
  EXPECT_TRUE(djReadArray(Context) == 0);
  EXPECT_TRUE(djReadObjectEnd(Context) == 1);
}

static const char TestReadEmptyObjectInObject__Json[] = " { \"key\" : {  } }";
void TestReadEmptyObjectInObject(dj_read_context* Context) {
  dj_string Key;
  EXPECT_TRUE(djReadExpectKey(Context, "key") == 1);
  {
    EXPECT_TRUE(djReadKey(Context, &Key) == 0);
  }
  EXPECT_TRUE(djReadObjectEnd(Context) == 1);
}

static const char TestReadEmptyArray__Json[] = "  [  ]";
void TestReadEmptyArray(dj_read_context* Context) {
  EXPECT_TRUE(djReadArray(Context) == 0);
}

static const char TestReadArray__Json[] = "  [ 123  , {} , true,false ,\"Hello!!\"]";
void TestReadArray(dj_read_context* Context) {
  dj_string Key;
  EXPECT_TRUE(djReadArray(Context) == 1);
  EXPECT_TRUE(djReadS64(Context) == 123);
  EXPECT_TRUE(djReadArray(Context) == 1);
  EXPECT_TRUE(djReadKey(Context, &Key) == 0);
  EXPECT_TRUE(djReadArray(Context) == 1);
  EXPECT_TRUE(djReadBool(Context) == 1);
  EXPECT_TRUE(djReadArray(Context) == 1);
  EXPECT_TRUE(djReadBool(Context) == 0);
  EXPECT_TRUE(djReadArray(Context) == 1);
  EXPECT_TRUE(strcmp(djReadString(Context).Data, "Hello!!") == 0);
  EXPECT_TRUE(djReadArray(Context) == 0);
}

static const char TestReadNestedArrays__Json[] = "  [ [ 1 ] , [] , [ 2, 3 ] ]";
void TestReadNestedArrays(dj_read_context* Context) {
  EXPECT_TRUE(djReadArray(Context) == 1);
  {
    EXPECT_TRUE(djReadArray(Context) == 1);
    EXPECT_TRUE(djReadS64(Context) == 1);
    EXPECT_TRUE(djReadArray(Context) == 0);
  }
  EXPECT_TRUE(djReadArray(Context) == 1);
  {
    EXPECT_TRUE(djReadArray(Context) == 0);
  }
  EXPECT_TRUE(djReadArray(Context) == 1);
  {
    EXPECT_TRUE(djReadArray(Context) == 1);
    EXPECT_TRUE(djReadS64(Context) == 2);
    EXPECT_TRUE(djReadArray(Context) == 1);
    EXPECT_TRUE(djReadS64(Context) == 3);
    EXPECT_TRUE(djReadArray(Context) == 0);
  }
  EXPECT_TRUE(djReadArray(Context) == 0);
}

#define ERROR_TEST(Name) { #Name, Name##__Json, Name##__Carrot, Name##__Message, Name }
static test_error ErrorTests[] = {
  ERROR_TEST(TestReadExpectedArray),
  
  ERROR_TEST(TestReadBool),
  
  ERROR_TEST(TestReadS64IllegalStart),
  ERROR_TEST(TestReadS64GotDecimal),
  ERROR_TEST(TestReadS64NegativeExponent),
  ERROR_TEST(TestReadS64EmptyExponent),
  
  ERROR_TEST(TestReadF64IllegalStart),
  ERROR_TEST(TestReadF64EmptyFraction),
  ERROR_TEST(TestReadF64EmptyExponent),
  
  ERROR_TEST(TestReadStringNotAString),
  ERROR_TEST(TestReadStringTooFewHex),
  ERROR_TEST(TestReadStringTooBigUnicode),
  ERROR_TEST(TestReadStringIllegalEscapeSequence)
};

#define SUCCESS_TEST(Name) { #Name, Name##__Json, Name }
static test_success SuccessTests[] = {
  SUCCESS_TEST(TestReadEmptyObject),
  SUCCESS_TEST(TestReadEmptyObjectInObject),
  SUCCESS_TEST(TestReadEmptyArray),
  SUCCESS_TEST(TestReadArray),
  SUCCESS_TEST(TestReadNestedArrays)
};


void PrintEscapedError(const char* Msg) {
  while (*Msg) {
    char C = *(Msg++);
    if (C == '\n')
      printf("\\n");
    else if (C == '\r')
      printf("\\r");
    else
      printf("%c", C);
  }
}

int main(int argc, char* argv[]) {
  int TotalTestCases = 0;
  int FailedTestCases = 0;
  
  // Test error messages
  for (int TestIndex = 0; TestIndex < ArrayCount(ErrorTests); TestIndex++) {
    test_error* Test = &ErrorTests[TestIndex];
    
    dj_read_context* Context = djReadFromString(Test->Json);
    
    Test->Function(Context);
    
    int Line = 1;
    int Column = 1;
    while (Test->Carrot[Column - 1] != '^') ++Column;
    
    char ExpectedError[1024];
    size_t PrintCount = snprintf(ExpectedError, ArrayCount(ExpectedError), "ERROR(Line %d, Col %d): %s\n > %s\n > %s",
                                 Line, Column, Test->Message, Test->Json, Test->Carrot);
    assert(PrintCount < ArrayCount(ExpectedError) - 1);
    
    const char* Error = djReadError(Context);
    int MessageNotEqual = Error ? strcmp(ExpectedError, Error) : 0; 
    if (!Error || MessageNotEqual) {
      printf("Error test case '%s':\n", Test->Name);
      printf("Json: '%s'\n", Test->Json);
      printf("ExpectedError: '"); PrintEscapedError(ExpectedError);                printf("'\n");
      printf("ActualError:   '"); PrintEscapedError(Error ? Error : "<no-error>"); printf("'\n");
      FailedTestCases += 1;
    }
    TotalTestCases += 1;
  }
  
  // Test correct parsing
  for (int TestIndex = 0; TestIndex < ArrayCount(SuccessTests); TestIndex++) {
    test_success* Test = &SuccessTests[TestIndex];
    
    dj_read_context* Context = djReadFromString(Test->Json);
    
    Test->Function(Context);
    
    djReadEOF(Context);
    
    if (djReadError(Context)) {
      printf("Succcess test case '%s':\n", Test->Name);
      printf("Json: '%s'\n", Test->Json);
      printf("Error: '%s'\n", djReadError(Context));
      FailedTestCases += 1;
    }
    TotalTestCases += 1;
  }
  
  if (FailedTestCases) {
    printf("Failure!\n %d failed out of %d total test case(s).\n", FailedTestCases, TotalTestCases);
    return 1;
  }
  printf("Success!\nRan %d tests.\n", TotalTestCases);    
  return 0;
}

