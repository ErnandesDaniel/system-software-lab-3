#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "symbol.h"

#define MAX_FUNCTIONS 64

typedef enum FunctionKind {
    FUNCTION_DECLARATION,
    FUNCTION_DEFINITION
} FunctionKind;

typedef struct FunctionInfo {
    char name[64];
    Type* return_type;
    SymbolTable params;
    FunctionKind kind;
} FunctionInfo;

typedef struct FunctionTable {
    FunctionInfo* functions[64];
    int count;
} FunctionTable;

// Глобальный реестр (массив) функций
extern FunctionInfo global_functions[MAX_FUNCTIONS];
extern int global_function_count;


// Утилиты для работы с функциями
void init_function_registry(void);

bool register_function(const char* name, Type* ret_type, FunctionKind kind);

FunctionInfo* find_function(const char* name);

void free_function_info(FunctionInfo* func);

void free_all_functions(void);

int get_function_index(const FunctionInfo* func);

void function_table_init(FunctionTable* table);
bool function_table_add(FunctionTable* table, FunctionInfo* func);
FunctionInfo* function_table_lookup(FunctionTable* table, const char* name);

#endif





