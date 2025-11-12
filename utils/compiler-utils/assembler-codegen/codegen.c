#include "types.h"
#include <stdio.h>
#include <string.h>


//Выравнивает размер стека до 16 байт, как требуется для Windows x64 ABI.
static int align_to_16(int size) {
    return (size + 15) & ~15;
}


//рассчитывает смещения для локальных переменных в стеке (начиная с -8, уменьшая на 4 байта для каждой), вычисляет общий размер фрейма и выравнивает его.
void codegen_layout_stack_frame(SymbolTable* locals, int* out_frame_size) {
    int offset = -8; // Start after return address
    for (int i = 0; i < locals->count; i++) {
        // Assume all types are 4 bytes for simplicity
        locals->symbols[i].stack_offset = offset;
        offset -= 4;
    }
    int frame_size = -offset; // Total size needed
    *out_frame_size = align_to_16(frame_size);
}

//Генерирует пролог функции: метку, push rbp, mov rbp, rsp, sub rsp для резервирования места.
void emit_prologue(CodeGenContext* ctx) {
    sprintf(ctx->out + strlen(ctx->out), "%s:\n", ctx->current_function);
    sprintf(ctx->out + strlen(ctx->out), "    push rbp\n");
    sprintf(ctx->out + strlen(ctx->out), "    mov rbp, rsp\n");
    if (ctx->frame_size > 0) {
        sprintf(ctx->out + strlen(ctx->out), "    sub rsp, %d\n", ctx->frame_size);
    }
}

//Генерирует эпилог: mov rsp, rbp, pop rbp, ret.
void emit_epilogue(CodeGenContext* ctx) {
    sprintf(ctx->out + strlen(ctx->out), "    mov rsp, rbp\n");
    sprintf(ctx->out + strlen(ctx->out), "    pop rbp\n");
    sprintf(ctx->out + strlen(ctx->out), "    ret\n");
}

//Загружает операнд (константу или переменную) в регистр.
void emit_load_operand(CodeGenContext* ctx, Operand* op, const char* reg32) {
    if (op->kind == OPERAND_CONST) {
        if (op->data.const_val.type->kind == TYPE_INT || op->data.const_val.type->kind == TYPE_BOOL) {
            sprintf(ctx->out + strlen(ctx->out), "    mov %s, %d\n", reg32, op->data.const_val.value.integer);
        } else {
            // Handle other types if needed
        }
    } else if (op->kind == OPERAND_VAR) {
        int offset = get_var_offset(&ctx->local_vars, op->data.var.name);
        sprintf(ctx->out + strlen(ctx->out), "    mov %s, [rbp + %d]\n", reg32, offset);
    }
}

//Сохраняет регистр в переменную по смещению.
void emit_store_to_var(CodeGenContext* ctx, const char* var_name, const char* reg32) {
    int offset = get_var_offset(&ctx->local_vars, var_name);
    sprintf(ctx->out + strlen(ctx->out), "    mov [rbp + %d], %s\n", offset, reg32);
}

//Находит смещение переменной по имени.
int get_var_offset(SymbolTable* locals, const char* name) {
    for (int i = 0; i < locals->count; i++) {
        if (strcmp(locals->symbols[i].name, name) == 0) {
            return locals->symbols[i].stack_offset;
        }
    }
    return 0; // Error, but for now
}

//проходит по блокам CFG, генерирует метки и инструкции для каждого IRInstruction (ASSIGN, ADD, SUB, LT, RET, JUMP, COND_BR).
void asm_build_from_cfg(char* out, FunctionInfo* func_info, SymbolTable* locals, CFG* cfg) {

    int frame_size;

    codegen_layout_stack_frame(locals, &frame_size);

    CodeGenContext ctx = {out, func_info->name, *locals, frame_size};

    emit_prologue(&ctx);

    // Traverse blocks
    for (size_t i = 0; i < cfg->num_blocks; i++) {
        BasicBlock* block = &cfg->blocks[i];
        sprintf(ctx.out + strlen(ctx.out), "%s:\n", block->id);

        for (size_t j = 0; j < block->num_instructions; j++) {
            IRInstruction* inst = &block->instructions[j];
            switch (inst->opcode) {
                case IR_ASSIGN:
                    emit_load_operand(&ctx, &inst->data.assign.value, "eax");
                    emit_store_to_var(&ctx, inst->data.assign.target, "eax");
                    break;
                case IR_ADD:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    add eax, ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_SUB:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    sub eax, ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_LT:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setl al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_RET:
                    if (inst->data.ret.has_value) {
                        emit_load_operand(&ctx, &inst->data.ret.value, "eax");
                    }
                    emit_epilogue(&ctx);
                    break;
                case IR_JUMP:
                    sprintf(ctx.out + strlen(ctx.out), "    jmp %s\n", inst->data.jump.target);
                    break;
                case IR_COND_BR:
                    emit_load_operand(&ctx, &inst->data.cond_br.condition, "eax");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, 0\n");
                    sprintf(ctx.out + strlen(ctx.out), "    jne %s\n", inst->data.cond_br.true_target);
                    sprintf(ctx.out + strlen(ctx.out), "    jmp %s\n", inst->data.cond_br.false_target);
                    break;
                // Add more opcodes as needed
                default:
                    break;
            }
        }
    }
}