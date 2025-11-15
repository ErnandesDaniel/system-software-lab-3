
#include "compiler-utils/cfg/types.h"
#include "compiler-utils/semantics-analysis/types.h"

typedef struct CodeGenContext {
    char* out;// выходной .asm файл
    // Информация о текущей функции (из symbol table)
    FunctionInfo* current_function;
    SymbolTable local_vars; // ссылка на локальные переменные с заполненным stack_offset
    int frame_size;               // общий размер стекового фрейма (выровненный до 16 байт)
    int string_counter;
    char data_section[4096];
} CodeGenContext;



void codegen_layout_stack_frame(SymbolTable* locals, int* out_frame_size);

// Что делает:
// Проходит по всем символам в locals.
// Присваивает каждому stack_offset (начиная с -8, -12, -16, …).
// Учитывает размер типа (sizeof(int) = 4, bool = 1, но выравнивание!).
// Вычисляет общий frame_size и округляет для правильного выравнивания стека (требование Windows x64).
// Сохраняет смещения в symbol->stack_offset.
// Пока можно считать, что все типы — 4 байта → проще.

void asm_build_from_cfg(char* out, FunctionInfo* func_info, SymbolTable* locals, CFG* cfg, FunctionTable* local_funcs);

// 🔹 2.2. Точка входа: генерация всей функции
// Что делает:
//
// Вызывает codegen_layout_stack_frame(locals, &frame_size).
// Создаёт CodeGenContext ctx = {out, func_name, locals, frame_size}.
// Вызывает emit_prologue(&ctx).
// Обходит все блоки в cfg и генерирует инструкции.
// Вызывает emit_epilogue(&ctx).


void emit_prologue(CodeGenContext* ctx);
void emit_epilogue(CodeGenContext* ctx);


// 2.4. Генерация инструкций
//
// void emit_instruction(CodeGenContext* ctx, IRInstruction* inst);
// Внутри — switch по inst->opcode:
//
// IR_ASSIGN → загрузить значение в eax, сохранить по смещению.
// IR_ADD, IR_SUB, ... → загрузить два операнда, выполнить операцию, сохранить результат.
// IR_RET → загрузить значение в eax, вызвать эпилог.
// IR_JUMP, IR_COND_BR → генерировать метки и jmp / je.


// Загрузить операнд (константу или переменную) в 32-битный регистр (eax, edx, ...)
void emit_load_operand(CodeGenContext* ctx, Operand* op, const char* reg32);

// Сохранить 32-битный регистр в переменную по имени
void emit_store_to_var(CodeGenContext* ctx, const char* var_name, const char* reg32);

// Получить смещение переменной по имени (из ctx->locals)
int get_var_offset(SymbolTable* locals, const char* name);


// // После построения CFG для функции "main":
// FILE* asm_file = fopen("output.asm", "w");
//
// // Добавь заголовок NASM
// fprintf(asm_file, "global main\n");
// fprintf(asm_file, "section .text\n\n");
//
// // Сгенерируй код
// codegen_emit_function(asm_file, "main", &builder_ctx.local_vars, &cfg);
//
// fclose(asm_file);








// Запусти программу:
// cmd
//
//
// 1
// program.exe
// Сразу после этого введи:
// cmd
//
//
// 1
// echo %ERRORLEVEL%
// 💡 %ERRORLEVEL% — это переменная Windows, хранящая код возврата последней программы.
//
//
//





