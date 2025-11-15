#include "types.h"
#include <stdio.h>
#include <string.h>


//Выравнивает размер стека до 16 байт, как требуется для Windows x64 ABI.
static int align_to_16(int size) {
    return (size + 15) & ~15;
}


//рассчитывает смещения для локальных переменных в стеке (начиная с -8, уменьшая на 4 байта для каждой).
// Параметры не сохраняются в стек, а используются напрямую из регистров.
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
// Параметры используются напрямую из регистров, без сохранения в стек.
void emit_prologue(CodeGenContext* ctx) {
    sprintf(ctx->out + strlen(ctx->out), "%s:\n", ctx->current_function->name);
    sprintf(ctx->out + strlen(ctx->out), "    push rbp\n");
    sprintf(ctx->out + strlen(ctx->out), "    mov rbp, rsp\n");

    if (ctx->frame_size > 0) {
        sprintf(ctx->out + strlen(ctx->out), "    sub rsp, %d\n", ctx->frame_size);
    }
}

//Генерирует эпилог: leave, ret.
void emit_epilogue(CodeGenContext* ctx) {
    sprintf(ctx->out + strlen(ctx->out), "; Очистка стека и возврат\n");
    sprintf(ctx->out + strlen(ctx->out), "    leave       ; эквивалент: mov rsp, rbp; pop rbp\n");
    sprintf(ctx->out + strlen(ctx->out), "    ret         ; возвращаем eax как результат\n");
}

//Загружает операнд (константу или переменную) в регистр.
// Для параметров использует регистры напрямую.
void emit_load_operand(CodeGenContext* ctx, Operand* op, const char* reg32) {
    if (op->kind == OPERAND_CONST) {
        if (op->data.const_val.type->kind == TYPE_INT || op->data.const_val.type->kind == TYPE_BOOL) {
            sprintf(ctx->out + strlen(ctx->out), "    mov %s, %d\n", reg32, op->data.const_val.value.integer);
        } else {
            // Handle other types if needed
        }
    } else if (op->kind == OPERAND_VAR) {
        // Check if it's a parameter
        for (int i = 0; i < ctx->current_function->params.count && i < 4; i++) {
            if (strcmp(ctx->current_function->params.symbols[i].name, op->data.var.name) == 0) {
                const char* param_regs[] = {"ecx", "edx", "r8d", "r9d"};
                sprintf(ctx->out + strlen(ctx->out), "    mov %s, %s\n", reg32, param_regs[i]);
                return;
            }
        }
        // It's a local variable
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
    fprintf(stderr, "Error: Variable '%s' not found in symbol table\n", name);
    return -999; // Error offset
}

//проходит по блокам CFG, генерирует метки и инструкции для каждого IRInstruction (ASSIGN, ADD, SUB, LT, RET, JUMP, COND_BR).
void asm_build_from_cfg(char* out, FunctionInfo* func_info, SymbolTable* locals, CFG* cfg, FunctionTable* local_funcs) {

    // Generate extern declarations for used functions
    for (int i = 0; i < local_funcs->count; i++) {
        FunctionInfo* func = local_funcs->functions[i];
        if (func) {
            sprintf(out + strlen(out), "extern %s\n", func->name);
        }
    }
    sprintf(out + strlen(out), "\n");

    int frame_size;

    codegen_layout_stack_frame(locals, &frame_size);

    CodeGenContext ctx = {out, func_info, *locals, frame_size};

    emit_prologue(&ctx);

    printf("DEBUG: cfg->num_blocks = %zu\n", cfg->num_blocks);

    // Traverse blocks
    for (size_t i = 0; i < cfg->num_blocks; i++) {
        BasicBlock* block = &cfg->blocks[i];
        printf("DEBUG: Block %zu: id=%s, num_instructions=%zu\n", i, block->id, block->num_instructions);
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
                case IR_MUL:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    imul eax, ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_DIV:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cdq\n");
                    sprintf(ctx.out + strlen(ctx.out), "    idiv ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_EQ:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    sete al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_NE:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setne al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
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
                case IR_LE:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setle al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_GT:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setg al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_GE:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    cmp eax, ebx\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setge al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_AND:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    and eax, ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_OR:
                    emit_load_operand(&ctx, &inst->data.compute.operands[0], "eax");
                    emit_load_operand(&ctx, &inst->data.compute.operands[1], "ebx");
                    sprintf(ctx.out + strlen(ctx.out), "    or eax, ebx\n");
                    emit_store_to_var(&ctx, inst->data.compute.result, "eax");
                    break;
                case IR_NOT:
                    emit_load_operand(&ctx, &inst->data.unary.operand, "eax");
                    sprintf(ctx.out + strlen(ctx.out), "    test eax, eax\n");
                    sprintf(ctx.out + strlen(ctx.out), "    setz al\n");
                    sprintf(ctx.out + strlen(ctx.out), "    movzx eax, al\n");
                    emit_store_to_var(&ctx, inst->data.unary.result, "eax");
                    break;
                case IR_NEG:
                    emit_load_operand(&ctx, &inst->data.unary.operand, "eax");
                    sprintf(ctx.out + strlen(ctx.out), "    neg eax\n");
                    emit_store_to_var(&ctx, inst->data.unary.result, "eax");
                    break;
                case IR_POS:
                    // Unary plus does nothing, just assign
                    emit_load_operand(&ctx, &inst->data.unary.operand, "eax");
                    emit_store_to_var(&ctx, inst->data.unary.result, "eax");
                    break;
                case IR_BIT_NOT:
                    emit_load_operand(&ctx, &inst->data.unary.operand, "eax");
                    sprintf(ctx.out + strlen(ctx.out), "    not eax\n");
                    emit_store_to_var(&ctx, inst->data.unary.result, "eax");
                    break;
                case IR_CALL:
                     // Microsoft x64 calling convention: pass args in rcx, rdx, r8, r9
                     // Use 32-bit registers for int/bool types
                     const char* arg_regs[] = {"ecx", "edx", "r8d", "r9d"};
                     for (int k = 0; k < inst->data.call.num_args && k < 4; k++) {
                         emit_load_operand(&ctx, &inst->data.call.args[k], arg_regs[k]);
                     }
                     sprintf(ctx.out + strlen(ctx.out), "    sub rsp, 32\n");
                     sprintf(ctx.out + strlen(ctx.out), "    call %s\n", inst->data.call.func_name);
                     sprintf(ctx.out + strlen(ctx.out), "    add rsp, 32\n");
                     if (strcmp(inst->data.call.result, "") != 0) {
                         emit_store_to_var(&ctx, inst->data.call.result, "eax");
                     }
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
                case IR_RET:
                    if (inst->data.ret.has_value) {
                        emit_load_operand(&ctx, &inst->data.ret.value, "eax");
                    }
                    emit_epilogue(&ctx);
                    break;
                default:
                    // Unsupported opcode
                    sprintf(ctx.out + strlen(ctx.out), "    ; Unsupported IR opcode: %d\n", inst->opcode);
                    break;
            }
        }
    }
    // No extra epilogue for main, as it's handled in IR_RET
}