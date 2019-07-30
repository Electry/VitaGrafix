#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "../src/interpreter/interpreter.h"
#include "../src/interpreter/parser.h"

#define INTP_PRIMITIVE_SIZE 4

typedef struct {
    const char *expr;
    byte_t expected_raw[MAX_VALUE_SIZE];
    uint32_t expected_size;
    intp_value_data_type_t expected_type;
} intp_testcase_t;

typedef struct {
    const char *expr;
    byte_t expected_raw[MAX_VALUE_SIZE];
    uint32_t expected_size;
    intp_value_data_type_t expected_type;
    bool expected_unk[MAX_VALUE_SIZE];
} intp_unk_testcase_t;

typedef struct {
    const char *expr;
    intp_status_code_t expected_status;
    uint32_t expected_pos;
} intp_error_testcase_t;

const intp_testcase_t _TESTS[] = {
    // Numeric primitives
    {"1",         {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"+1",        {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"-1",        {0xFF, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"1f",        {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"1.0",       {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"1.0f",      {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"0x123",     {0x23, 0x01, 0x00, 0x00}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"-0x123",    {0xDD, 0xFE, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},

    // Raw primitives
    {"12 34 r",   {0x12, 0x34},             2,                   DATA_TYPE_RAW},
    {"12 34r",    {0x12, 0x34},             2,                   DATA_TYPE_RAW},
    {"1234r",     {0x12, 0x34},             2,                   DATA_TYPE_RAW},
    {"1234 r",    {0x12, 0x34},             2,                   DATA_TYPE_RAW},
    {"DEADBEEFr", {0xDE, 0xAD, 0xBE, 0xEF}, 4,                   DATA_TYPE_RAW},

    // Constants
    {"pi",        {0xDB, 0x0F, 0x49, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"e",         {0x54, 0xF8, 0x2D, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // min/max
    {"4294967295",    {0xFF, 0xFF, 0xFF, 0xFF},  INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"-2147483648",   {0x00, 0x00, 0x00, 0x80},  INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"1.9999999",     {0xFF, 0xFF, 0xFF, 0x3F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"3.4028235E38",  {0xFF, 0xFF, 0x7F, 0x7F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"-3.4028235E38", {0xFF, 0xFF, 0x7F, 0xFF},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"0xFFFFFFFF",    {0xFF, 0xFF, 0xFF, 0xFF},  INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"-0x80000000",   {0x00, 0x00, 0x00, 0x80},  INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF r",
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        32, DATA_TYPE_RAW},

    // Parentheses
    {"(42)",        {0x2A},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"((+42))",     {0x2A},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"(((-42)))",   {0xD6, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"(12.1f)",     {0x9A, 0x99, 0x41, 0x41}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"((-0x123))",  {0xDD, 0xFE, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},

    // Simple math
    {"2+1",         {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"2 + 1",       {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"2   +   1",   {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(((2+(1))))", {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3+2",         {0x05},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3+2+4",       {0x09},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(3+2)+4",     {0x09},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3+(2+4)",     {0x09},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(3+2+4)",     {0x09},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3 - 2",       {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3 - -2",      {0x05},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"-3 - 2",      {0xFB, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"2 - 3",       {0xFF, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"3 - (2 - 4)", {0x05},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"(3-2) - 4",   {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"(3-2-4)",     {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"3*2*4",       {0x18},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(3*2)*4",     {0x18},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3*(2*4)",     {0x18},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(3*2*4)",     {0x18},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"-3 * 2",      {0xFA, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"3 / 2",       {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 / 2",       {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"8 / 2 / 2",   {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"8 / (2 / 2)", {0x08},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"-6 / 2",      {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"6 / -2",      {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"3 * 4 / 2",   {0x06},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 / 2 * 3",   {0x06},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 \% 2",      {0x00},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 \% 3",      {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"123 \% 23",   {0x08},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"123 \% -23",  {0x08},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"-123 \% -23", {0xF8, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},

    // Floats
    {"4f + 3f",     {0x00, 0x00, 0xE0, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"4f - 3f",     {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"4f / 3f",     {0xAB, 0xAA, 0xAA, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"4f * 3f",     {0x00, 0x00, 0x40, 0x41}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Bitwise
    {"2 << 1",      {0x04},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"16>>2",       {0x04},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"6 & 2",       {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"3 | 4",       {0x07},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"255^0",       {0xFF},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"255^255",     {0x00},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},

    // Operator precedence
    {"8 / 2 - 2",   {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"8 - 2 / 2",   {0x07},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(8 - 2)/2",   {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"(8*2-1)/5",   {0x03},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"10+2/2*5",    {0x0F},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"2*(3+2)*2",   {0x14},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"10 & 2 | 8 << 1", {0x12},               INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"16 & (2 | 8)<<1", {0x10},               INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},

    // Math fns
    {"sin(pi / 2)", {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"tan(pi / 4)", {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"abs(1)",      {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"abs((1))",    {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"abs(    ( 1  ) )", {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"abs(-1)",     {0x01},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"abs(-1.0)",   {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"abs(1.0)",    {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"min(1,-5)",   {0xFB, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"max(-1,5)",   {0x05},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"align(720,32)", {0xE0, 0x02},           INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"align(736,32)", {0xE0, 0x02},           INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"align(737,32)", {0x00, 0x03},           INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"align(-16,32)", {0x00},                 INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"align(0.5,1.0)", {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"align(-0.5,1.0)", {0x00, 0x00, 0x00, 0x80}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"align(720, -32)",    {0xE0, 0x02},             INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"align(720, 32.0)",   {0x00, 0x00, 0x38, 0x44}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"align(720, -32.0)",  {0x00, 0x00, 0x38, 0x44}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"align(-720, 32)",    {0x40, 0xFD, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"align(-720, -32)",   {0x40, 0xFD, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"align(-720, 32.0)",  {0x00, 0x00, 0x30, 0xC4}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"align(-720, -32.0)", {0x00, 0x00, 0x30, 0xC4}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Fns limits
    {"sin(4294967296.0)", {0x81, 0x89, 0xEC, 0xBE},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"abs(-2147483647)",  {0xFF, 0xFF, 0xFF, 0x7F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"sin(1.9999999)",    {0xB8, 0xC7, 0x68, 0x3F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"pow(2, 31)",        {0x00, 0x00, 0x00, 0x80},  INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"pow(-2, 15)",       {0x00, 0x80, 0xFF, 0xFF},  INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},

    // Auto type casting
    {"4f + 3",     {0x00, 0x00, 0xE0, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"4 - 3f",     {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"4f + -3",    {0x00, 0x00, 0x80, 0x3F}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"-4 + 3f",    {0x00, 0x00, 0x80, 0xBF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Manual type casting
    {"int(3)",     {0x03},   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"int(3.0)",   {0x03},   INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"int(-3.0)",  {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_SIGNED},
    {"uint(3)",    {0x03},   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"uint(3.0)",  {0x03},   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"uint(-3)",   {0xFD, 0xFF, 0xFF, 0xFF}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"float(3)",   {0x00, 0x00, 0x40, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"float(-3)",  {0x00, 0x00, 0x40, 0xC0}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"float(3.0)", {0x00, 0x00, 0x40, 0x40}, INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Raw casting
    {"int8(127)",          {0x7F},                    1, DATA_TYPE_RAW},
    {"int8(-128)",         {0x80},                    1, DATA_TYPE_RAW},
    {"int16(32767)",       {0xFF, 0x7F},              2, DATA_TYPE_RAW},
    {"int16(-32768)",      {0x00, 0x80},              2, DATA_TYPE_RAW},
    {"int32(2147483647)",  {0xFF, 0xFF, 0xFF, 0x7F},  4, DATA_TYPE_RAW},
    {"int32(-2147483648)", {0x00, 0x00, 0x00, 0x80},  4, DATA_TYPE_RAW},
    {"uint8(0)",           {0x00},                    1, DATA_TYPE_RAW},
    {"uint8(255)",         {0xFF},                    1, DATA_TYPE_RAW},
    {"uint16(0)",          {0x00},                    2, DATA_TYPE_RAW},
    {"uint16(65535)",      {0xFF, 0xFF},              2, DATA_TYPE_RAW},
    {"uint32(0)",          {0x00},                    4, DATA_TYPE_RAW},
    {"uint32(4294967295)", {0xFF, 0xFF, 0xFF, 0xFF},  4, DATA_TYPE_RAW},
    {"fl32(1)",            {0x00, 0x00, 0x80, 0x3F},  4, DATA_TYPE_RAW},
    {"fl32(1.0)",          {0x00, 0x00, 0x80, 0x3F},  4, DATA_TYPE_RAW},
    {"fl32(pi)",           {0xDB, 0x0F, 0x49, 0x40},  4, DATA_TYPE_RAW},
    {"bytes(pi)",          {0xDB, 0x0F, 0x49, 0x40},  4, DATA_TYPE_RAW},
    {"bytes(DEADBEEFr)",   {0xDE, 0xAD, 0xBE, 0xEF},  4, DATA_TYPE_RAW},
    {"bytes(DEADBEEF)",    {0xDE, 0xAD, 0xBE, 0xEF},  4, DATA_TYPE_RAW},
    {"bytes(DE AD BE EF)", {0xDE, 0xAD, 0xBE, 0xEF},  4, DATA_TYPE_RAW},

    // Overflow?
    {"int8(4294967295)",   {0xFF},                    1, DATA_TYPE_RAW},
    {"int16(4294967295)",  {0xFF, 0xFF},              2, DATA_TYPE_RAW},
    {"uint8(4294967295)",  {0xFF},                    1, DATA_TYPE_RAW},
    {"uint16(4294967295)", {0xFF, 0xFF},              2, DATA_TYPE_RAW},

    // Encoders
    {"nop",                 {0x00, 0xBF},              2, DATA_TYPE_RAW},
    {"bkpt",                {0x00, 0xBE},              2, DATA_TYPE_RAW},
    {"t1_mov(1, 255)",      {0xFF, 0x21},              2, DATA_TYPE_RAW},
    {"t2_mov(1,2,720/2*2)", {0x5F, 0xF4, 0x34, 0x72},  4, DATA_TYPE_RAW},
    {"t3_mov(2,739)",       {0x40, 0xF2, 0xE3, 0x22},  4, DATA_TYPE_RAW},
    {"t1_movt( 3 , 42 )",   {0xC0, 0xF2, 0x2A, 0x03},  4, DATA_TYPE_RAW},
    {"a1_mov(1,5,255)",     {0xFF, 0x50, 0xB0, 0xE3},  4, DATA_TYPE_RAW},
    {"a2_mov(1,736)",       {0xE0, 0x12, 0x00, 0xE3},  4, DATA_TYPE_RAW},
    {"t2_vmov(1, 1.0)",     {0xF7, 0xEE, 0x00, 0x0A},  4, DATA_TYPE_RAW},
    {"t2_vmov(8, -1.5)",    {0xBF, 0xEE, 0x08, 0x4A},  4, DATA_TYPE_RAW},
    {"t2_vmov(16, 31.0)",   {0xB3, 0xEE, 0x0F, 0x8A},  4, DATA_TYPE_RAW},

    // Mixed real world cases
    {"t1_movt(4, 725.0f >> 16)",                  {0xC4, 0xF2, 0x35, 0x44}, 4, DATA_TYPE_RAW},
    {"t1_movt(2, 544 / 2.0 >> 16)",               {0xC4, 0xF2, 0x88, 0x32}, 4, DATA_TYPE_RAW},
    {"t2_mov(0, 0, (544 + 31 & 0xFFFFFFE0) * 4)", {0x4F, 0xF4, 0x08, 0x60}, 4, DATA_TYPE_RAW},
    {"t2_mov(0, 0, align(544, 32) * 4)",          {0x4F, 0xF4, 0x08, 0x60}, 4, DATA_TYPE_RAW},
    {"t2_mov(1, 1, 960 * 10222 / 10000)",         {0x5F, 0xF4, 0x75, 0x71}, 4, DATA_TYPE_RAW},
    {"t2_mov(1, 1, uint(960 * 1.0222f))",         {0x5F, 0xF4, 0x75, 0x71}, 4, DATA_TYPE_RAW},
    {"t2_mov(1, 1, uint(960 * (736 / 720f)))",    {0x5F, 0xF4, 0x75, 0x71}, 4, DATA_TYPE_RAW},

    // Case insens.
    {"nOp",                 {0x00, 0xBF},              2, DATA_TYPE_RAW},
    {"NoP",                 {0x00, 0xBF},              2, DATA_TYPE_RAW},
    {"NOP",                 {0x00, 0xBF},              2, DATA_TYPE_RAW},
    {"t1_MOV(1, 255)",      {0xFF, 0x21},              2, DATA_TYPE_RAW},
    {"T1_MOV(1, 255)",      {0xFF, 0x21},              2, DATA_TYPE_RAW},
    {"BYTES(deadbeefR)",    {0xDE, 0xAD, 0xBE, 0xEF},  4, DATA_TYPE_RAW},
    {"INT16(-32768)",       {0x00, 0x80},              2, DATA_TYPE_RAW},
    {"4F + 3F",             {0x00, 0x00, 0xE0, 0x40},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Sum spaces
    {"sin    (  pi / 2 ) ", {0x00, 0x00, 0x80, 0x3F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"1.0 f",   {0x00, 0x00, 0x80, 0x3F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},
    {"1 f",     {0x00, 0x00, 0x80, 0x3F},  INTP_PRIMITIVE_SIZE, DATA_TYPE_FLOAT},

    // Concatenations
    {"2 . 3",               {0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}, 8, DATA_TYPE_RAW},
    {"uint8(2) . uint8(3)", {0x02, 0x03}, 2, DATA_TYPE_RAW},
    {"float(1) . float(1)", {0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x80, 0x3F}, 8, DATA_TYPE_RAW},
    {"DEr . ADr",             {0xDE, 0xAD}, 2, DATA_TYPE_RAW},
    {"DEr.ADr",               {0xDE, 0xAD}, 2, DATA_TYPE_RAW},
    {"bytes(DE) . bytes(AD)", {0xDE, 0xAD}, 2, DATA_TYPE_RAW},
    {"t1_mov(1, 255) . nop",  {0xFF, 0x21, 0x00, 0xBF}, 4, DATA_TYPE_RAW},

    // Repeat
    {"bytes(DEAD) *2",  {0xDE, 0xAD, 0xDE, 0xAD}, 4, DATA_TYPE_RAW},
    {"bytes(DEAD) * 2", {0xDE, 0xAD, 0xDE, 0xAD}, 4, DATA_TYPE_RAW},
    {"uint8(255) *3",   {0xFF, 0xFF, 0xFF}, 3, DATA_TYPE_RAW},
    {"nop *2",          {0x00, 0xBF, 0x00, 0xBF}, 4, DATA_TYPE_RAW},
    {"2 * nop",         {0x00, 0xBF, 0x00, 0xBF}, 4, DATA_TYPE_RAW},

    // VG
    {"fb_w",    {0xC0, 0x03}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"fb_h",    {0x20, 0x02}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"ib_w(0)", {0xC0, 0x03}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"ib_h(0)", {0x20, 0x02}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"vblank",  {0x01},       INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"msaa",    {0x02},       INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},

#ifdef BUILD_LEGACY_SUPPORT
    // Legacy
    {"nop()",              {0x00, 0xBF},              2, DATA_TYPE_RAW},
    {"bkpt()",             {0x00, 0xBE},              2, DATA_TYPE_RAW},
    {"t1_movt(4,<r,<to_fl,</,<*,960,544>,720>>,16>)", {0xC4, 0xF2, 0x35, 0x44}, 4, DATA_TYPE_RAW},
    {"t1_movt(2,<r,<to_fl,</,544,2>>,16>)",           {0xC4, 0xF2, 0x88, 0x32}, 4, DATA_TYPE_RAW},
    {"t2_mov(1,0,<*,<&,<+,544,31>,0xFFFFFFE0>,4>)",   {0x5F, 0xF4, 0x08, 0x60}, 4, DATA_TYPE_RAW},
    {"t2_mov(1,2,</,<*,960,10222>,10000>)",           {0x5F, 0xF4, 0x75, 0x72}, 4, DATA_TYPE_RAW},
#endif

    // Terminator
    {"4 / 2$",       {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 / 2 $",      {0x02},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"4 $/ 2",       {0x04},                   INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"2 . 3 $",      {0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}, 8, DATA_TYPE_RAW},
    {"2 $. 3",       {0x02, 0x00, 0x00, 0x00}, INTP_PRIMITIVE_SIZE, DATA_TYPE_UNSIGNED},
    {"t1_mov(1, 255) . nop $",  {0xFF, 0x21, 0x00, 0xBF}, 4, DATA_TYPE_RAW},
    {"bytes(DE AD BE EF)$",     {0xDE, 0xAD, 0xBE, 0xEF}, 4, DATA_TYPE_RAW},
};

const intp_error_testcase_t _TESTS_ERROR[] = {
    // Invalid token
    {"abcd",      INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {"   abcd",   INTP_STATUS_ERROR_INVALID_TOKEN, 3},
    {"DEADBEEF",  INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {"0xZZ",      INTP_STATUS_ERROR_INVALID_TOKEN, 1},
    {".0f",       INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {"+",         INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {".",         INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {"f",         INTP_STATUS_ERROR_INVALID_TOKEN, 0},
    {"0 x 123",   INTP_STATUS_ERROR_INVALID_TOKEN, 2},
    {"bytes(0x123)", INTP_STATUS_ERROR_INVALID_TOKEN, 7},
    {"1)",        INTP_STATUS_ERROR_INVALID_TOKEN, 1},
    {"1 + ()",    INTP_STATUS_ERROR_INVALID_TOKEN, 5},
    {"()",        INTP_STATUS_ERROR_INVALID_TOKEN, 1},
    {"(",         INTP_STATUS_ERROR_INVALID_TOKEN, 1},
    {"1 +",       INTP_STATUS_ERROR_INVALID_TOKEN, 3},

    // Invalid argument count
#ifdef BUILD_LEGACY_SUPPORT
    {"nop(1)",               INTP_STATUS_ERROR_TOO_MANY_ARGS, 4},
#else
    {"nop(1)",               INTP_STATUS_ERROR_INVALID_TOKEN, 3},
#endif
    {"sin(1, 2)",            INTP_STATUS_ERROR_TOO_MANY_ARGS, 5},
    {"abs(1, 2)",            INTP_STATUS_ERROR_TOO_MANY_ARGS, 5},
    {"t2_mov()",             INTP_STATUS_ERROR_TOO_FEW_ARGS, 7},
    {"t2_mov(1)",            INTP_STATUS_ERROR_TOO_FEW_ARGS, 8},
    {"t2_mov(1, 2)",         INTP_STATUS_ERROR_TOO_FEW_ARGS, 11},
    {"t2_mov(1, 2, 3, 4)",   INTP_STATUS_ERROR_TOO_MANY_ARGS, 14},
    {"t2_mov(1, 2, 3121313, 422)",   INTP_STATUS_ERROR_TOO_MANY_ARGS, 20},

    // Missing brackets
    {"sin(pi",               INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, 6},
    {"sin(pi + 2.0",         INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, 12},
    {"sinpi)",               INTP_STATUS_ERROR_MISSING_OPEN_BRACKET, 3},

    // Invalid data type
    {"DEr + ADr",    INTP_STATUS_ERROR_INVALID_DATATYPE, 4},
    {"abs(DEADr)",   INTP_STATUS_ERROR_INVALID_DATATYPE, 0},
    {"bytes(DE AD) * bytes(DE AD)", INTP_STATUS_ERROR_INVALID_DATATYPE, 13},
    {"1.0 % 5",      INTP_STATUS_ERROR_INVALID_DATATYPE, 4},

    // VG
    {"ib_w(-1)",     INTP_STATUS_ERROR_INVALID_DATATYPE, 0},

    // Terminator
    {"sin(pi$",      INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, 6},
    {"sin$pi)",      INTP_STATUS_ERROR_MISSING_OPEN_BRACKET, 3},
    {"1 + ($",       INTP_STATUS_ERROR_INVALID_TOKEN, 5},
    {"1 + $",        INTP_STATUS_ERROR_INVALID_TOKEN, 4},
    {"t2_mov(1,$",   INTP_STATUS_ERROR_INVALID_TOKEN, 9},
    {"t2_mov(1, $, 3)", INTP_STATUS_ERROR_INVALID_TOKEN, 10},
    {"bytes(DE AD$BE EF)", INTP_STATUS_ERROR_MISSING_CLOSE_BRACKET, 11},
};

const intp_unk_testcase_t _TESTS_UNK[] = {
    {"??(4)",                        {0x00, 0x00, 0x00, 0x00},                   4, DATA_TYPE_RAW, {1, 1, 1, 1}},
    {"DEr . ??(1) . ADr",            {0xDE, 0x00, 0xAD},                         3, DATA_TYPE_RAW, {0, 1, 0}},
    {"DEr.ADr.??(5)",                {0xDE, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00}, 7, DATA_TYPE_RAW, {0, 0, 1, 1, 1, 1, 1}},
    {"t1_mov(1, 255) . ??(2) . nop", {0xFF, 0x21, 0x00, 0x00, 0x00, 0xBF},       6, DATA_TYPE_RAW, {0, 0, 1, 1, 0, 0}},
    {"mov32(2, 725.0, 2)",           {0x44, 0xF2, 0x00, 0x02, 0x00, 0x00, 0xC4, 0xF2, 0x35, 0x42},  10, DATA_TYPE_RAW, {0, 0, 0, 0, 1, 1, 0, 0, 0, 0}},
};

uint32_t g_success_cnt = 0;

void pr_bytes(byte_t *bytes, int size) {
    for (int i = 0; i < size; i++) {
        printf("%02X ", bytes[i]);
    }
}

void test_assert(intp_testcase_t test, uint32_t pos, intp_value_t *value, intp_status_t status) {
    byte_t expected_full[test.expected_size];
    memset(expected_full, 0, test.expected_size);
    memcpy(expected_full, test.expected_raw, test.expected_size);

    if (status.code != INTP_STATUS_OK) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected status %d, got %d\n", 0, status.code);
        goto ERROR;
    }

    // Value size
    if (value->size != test.expected_size) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected size %d, got %d\n", test.expected_size, value->size);
        goto ERROR;
    }

    // Value type
    if (value->type != test.expected_type) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected type '%s', got '%s'\n",
                intp_data_type_to_string(test.expected_type),
                intp_data_type_to_string(value->type));
        goto ERROR;
    }

    // Value data
    for (int i = 0; i < test.expected_size; i++) {
        if (expected_full[i] != value->data.raw[i]) {
            printf("FAIL: '%s'\n", test.expr);
            printf("-  Expected: ");
            pr_bytes(expected_full, test.expected_size);
            printf("\n");
            printf("-  Got: ");
            pr_bytes(value->data.raw, value->size);
            printf("\n");
            goto ERROR;
        }
    }

    //printf("SUCCES: '%s'\n", test.expr);
    g_success_cnt++;

ERROR:
    return;
}

void test_error_assert(intp_error_testcase_t test, intp_status_t status) {
    if (status.code != test.expected_status) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected status %d, got %d\n", test.expected_status, status.code);
        goto ERROR_ETEST;
    }

    if (status.pos != test.expected_pos) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected pos %d, got %d\n", test.expected_pos, status.pos);
        goto ERROR_ETEST;
    }

    //printf("SUCCES: '%s'\n", test.expr);
    g_success_cnt++;

ERROR_ETEST:
    return;
}

void test_unk_assert(intp_unk_testcase_t test, uint32_t pos, intp_value_t *value, intp_status_t status) {
    byte_t expected_full[test.expected_size];
    memset(expected_full, 0, test.expected_size);
    memcpy(expected_full, test.expected_raw, test.expected_size);

    if (status.code != INTP_STATUS_OK) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected status %d, got %d\n", 0, status.code);
        goto ERROR_UNKTEST;
    }

    // Value size
    if (value->size != test.expected_size) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected size %d, got %d\n", test.expected_size, value->size);
        goto ERROR_UNKTEST;
    }

    // Value type
    if (value->type != test.expected_type) {
        printf("FAIL: '%s'\n", test.expr);
        printf("-  Expected type '%s', got '%s'\n",
                intp_data_type_to_string(test.expected_type),
                intp_data_type_to_string(value->type));
        goto ERROR_UNKTEST;
    }

    // Value data
    for (int i = 0; i < test.expected_size; i++) {
        if (expected_full[i] != value->data.raw[i]) {
            printf("FAIL: '%s'\n", test.expr);
            printf("-  Expected: ");
            pr_bytes(expected_full, test.expected_size);
            printf("\n");
            printf("-  Got: ");
            pr_bytes(value->data.raw, value->size);
            printf("\n");
            goto ERROR_UNKTEST;
        }
    }

    // Value data
    for (int i = 0; i < test.expected_size; i++) {
        if (test.expected_unk[i] != value->unk[i]) {
            printf("FAIL: '%s'\n", test.expr);
            printf("-  Expected ?? bytes: ");
            pr_bytes((byte_t *)test.expected_unk, test.expected_size);
            printf("\n");
            printf("-  Got: ");
            pr_bytes((byte_t *)value->unk, value->size);
            printf("\n");
            goto ERROR_UNKTEST;
        }
    }

    g_success_cnt++;

ERROR_UNKTEST:
    return;
}

extern const token_t _TOKENS[];

int main() {
    printf("Hello!\n");
    printf("\n");

    g_success_cnt = 0;

    intp_value_t value;
    uint32_t pos;
    uint32_t tests_cnt = sizeof(_TESTS) / sizeof(intp_testcase_t);
    uint32_t tests_error_cnt = sizeof(_TESTS_ERROR) / sizeof(intp_error_testcase_t);
    uint32_t tests_unk_cnt = sizeof(_TESTS_UNK) / sizeof(intp_unk_testcase_t);

    intp_status_t ret;

    // Test token table
    bool tt_broken = false;
    for (int i = 0; i < TOKEN_INVALID + 1; i++) {
        if (_TOKENS[i].type != i) {
            printf("Token table type mismatch, index: %d\n", i);
            tt_broken = true;
            break;
        }
    }
    if (!tt_broken) {
        printf("Token table OK!\n");
    }


    for (int i = 0; i < tests_cnt; i++) {
        memset(&value, 0, sizeof(intp_value_t));
        pos = 0;

        ret = intp_evaluate(_TESTS[i].expr, &pos, &value);
        test_assert(_TESTS[i], pos, &value, ret);
    }

    for (int i = 0; i < tests_error_cnt; i++) {
        pos = 0;

        ret = intp_evaluate(_TESTS_ERROR[i].expr, &pos, &value);
        test_error_assert(_TESTS_ERROR[i], ret);
    }

    for (int i = 0; i < tests_unk_cnt; i++) {
        memset(&value, 0, sizeof(intp_value_t));
        pos = 0;

        ret = intp_evaluate(_TESTS_UNK[i].expr, &pos, &value);
        test_unk_assert(_TESTS_UNK[i], pos, &value, ret);
    }

    printf("\n");
    printf("%d out of %d tests succeeded!\n", g_success_cnt, tests_cnt + tests_error_cnt + tests_unk_cnt);
    printf("\n");

    return 0;
}
