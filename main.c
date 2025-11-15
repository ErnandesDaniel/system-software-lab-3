#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

#include "lib/tree-sitter/lib/include/tree_sitter/api.h"
#include "src/tree_sitter/parser.h"
#include "utils/common-utils/common-utils.h"
#include "utils/mermaid-utils/mermaid-utils.h"

#include "utils/compiler-utils/ast/ast.h"

#include "utils/compiler-utils/cfg/cfg.h"

#include "utils/compiler-utils/semantics-analysis/functions.h"

#include "utils/compiler-utils/semantics-analysis/semantics-analysis.h"

#include "utils/compiler-utils/assembler-codegen/codegen.c"


// Подключаем твою грамматику
TSLanguage *tree_sitter_mylang(); // Объявляем функцию из parser.c


int main(const int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <input_file> <output_mmd> <output_dir> <asm_output_dir>\n", argv[0]);
        return 1;
    }

    long file_size;
    char* source_code = read_file(argv[1], &file_size);
    if (!source_code) {
        perror("The input file could not be read");
        return 1;
    }

    // Создаем директорию для файлов функций и очищаем ее
    if (_mkdir(argv[3]) != 0 && errno != EEXIST) {
        perror("Failed to create output directory");
        free(source_code);
        return 1;
    } else if (errno == EEXIST) {
        // Директория существует, очищаем ее
        char search_path[256];
        sprintf(search_path, "%s\\*", argv[3]);
        WIN32_FIND_DATA find_data;
        HANDLE hFind = FindFirstFile(search_path, &find_data);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                    char file_path[256];
                    sprintf(file_path, "%s\\%s", argv[3], find_data.cFileName);
                    DeleteFile(file_path);
                }
            } while (FindNextFile(hFind, &find_data));
            FindClose(hFind);
        }
    }

    // Создаем директорию для .asm файлов и очищаем ее
    if (_mkdir(argv[4]) != 0 && errno != EEXIST) {
        perror("Failed to create assembler output directory");
        free(source_code);
        return 1;
    } else if (errno == EEXIST) {
        // Директория существует, очищаем ее
        char search_path[256];
        sprintf(search_path, "%s\\*", argv[4]);
        WIN32_FIND_DATA find_data;
        HANDLE hFind = FindFirstFile(search_path, &find_data);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
                    char file_path[256];
                    sprintf(file_path, "%s\\%s", argv[4], find_data.cFileName);
                    DeleteFile(file_path);
                }
            } while (FindNextFile(hFind, &find_data));
            FindClose(hFind);
        }
    }

    // Инициализация парсера
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_mylang());

    //Парсинг
    TSTree *tree = ts_parser_parse_string(parser, NULL, source_code, file_size);

    const TSNode root_node = ts_tree_root_node(tree);

    // Проверяем на ошибки разбора
    if (ts_node_has_error(root_node)) {
        // Выводим конкретные ошибки
        fprintf(stderr, "Parsing errors:\n");
        print_parse_errors(root_node, source_code, 0);
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        free(source_code);
        return 1;
    }

    // Генерируем Mermaid диаграмму для всего файла
    char *mermaid_str = generate_mermaid(root_node, source_code);

    if (!mermaid_str) {
        fprintf(stderr, "Mermaid generation error\n");
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        free(source_code);
        return 1;
    }

    // Создаем MD файл с диаграммой всего файла
    FILE *out_md = fopen(argv[2], "w");
    if (!out_md) {
        perror("Failed to create MD output file");
        free(mermaid_str);
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        free(source_code);
        return 1;
    }

    fputs(mermaid_str, out_md);

    fclose(out_md);
    free(mermaid_str);

    // 2. Семантический анализ (генерация массива данных о функциях)
    build_global_symbol_table(root_node, source_code);

    // Массивы для хранения всех CFG
    CFG* function_cfgs[MAX_FUNCTIONS] = {0}; // инициализируем нулями

    // Теперь генерируем файлы для каждой функции
    const uint32_t child_count = ts_node_child_count(root_node);

    for (uint32_t i = 0; i < child_count; i++) {


        TSNode child = ts_node_child(root_node, i);

        if (strcmp(ts_node_type(child), "source_item") != 0) continue;

        TSNode function_node=ts_node_child(child, 0);

        // Находим сигнатуру функции
        const TSNode signature = ts_node_child_by_field_name(function_node, "signature", strlen("signature"));
        if (ts_node_is_null(signature)) continue;

        // Находим имя функции
        const TSNode func_name_node = ts_node_child_by_field_name(signature, "name", strlen("name"));
        if (ts_node_is_null(func_name_node)) continue;

        char func_name[64];
        get_node_text(func_name_node, source_code, func_name, sizeof(func_name));

        // Находим FunctionInfo по имени
        FunctionInfo* func_info = find_function(func_name);
        if (!func_info) continue;

        //Пропускаем декларации функций (только определения)
        if (func_info->kind == FUNCTION_DECLARATION) continue;

        // Строим CFG для этой функции

        //Объявляем таблицу символов для переменных функции
        SymbolTable local_vars;

        //Объявляем таблицу функций для используемых функций из вне
        FunctionTable local_funcs;

        CFG* func_cfg = cfg_build_from_ast(func_info, source_code, child, &local_vars, &local_funcs);
        if (!func_cfg) continue;

        // Получаем индекс функции
        int idx = get_function_index(func_info);
        if (idx == -1) continue;

        // Сохраняем граф в массив
        function_cfgs[idx] = func_cfg;

        if (func_cfg) {
            // Генерируем Mermaid диаграмму из CFG
            char* func_mermaid = cfg_generate_mermaid(func_cfg);

            if (func_mermaid) {
                // Создаем файл для функции
                char filepath[256];
                sprintf(filepath, "%s\\%s.mmd", argv[3], func_name);
                FILE* func_file = fopen(filepath, "w");
                if (func_file) {
                    fputs(func_mermaid, func_file);
                    fclose(func_file);
                } else {
                    fprintf(stderr, "Failed to create file for function %s\n", func_name);
                }
                free(func_mermaid);
            }

            // Generate assembly code
            char asm_filepath[256];
            sprintf(asm_filepath, "%s\\%s.asm", argv[4], func_name);
            FILE* asm_file = fopen(asm_filepath, "w");
            if (asm_file) {
                // Write NASM header
                fprintf(asm_file, "bits 64\n");
                fprintf(asm_file, "default rel\n");
                fprintf(asm_file, "section .text\n\n");
                fprintf(asm_file, "global %s\n", func_name);

                char* asm_code = (char*)malloc(1024 * 1024); // 1MB buffer
                if (asm_code) {
                    asm_code[0] = '\0';
                    asm_build_from_cfg(asm_code, func_info, &local_vars, func_cfg, &local_funcs);
                    fputs(asm_code, asm_file);
                    free(asm_code);
                }
                fclose(asm_file);
            } else {
                fprintf(stderr, "Failed to create .asm file for function %s\n", func_name);
            }

            cfg_destroy_graph(func_cfg);
        }
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    free(source_code);


    // === СБОРКА ВСЕХ .asm → program.exe В УКАЗАННОЙ ПАПКЕ ===
    printf("Assembling and linking all functions into program.exe...\n");

    const char* asm_dir = argv[4];
    char cmd[2048];

    // Команда для Windows cmd:
    // 1. Перейти в папку asm
    // 2. Удалить старые .obj (если есть)
    // 3. Собрать все .asm → .obj через nasm
    // 4. Слинковать все .obj в program.exe через gcc
    snprintf(cmd, sizeof(cmd),
        "cd /d \"%s\" && "
        "del *.obj 2>nul && "
        "(for %%f in (*.asm) do nasm -f win64 \"%%f\" -o \"%%~nf.obj\") && "
        "gcc *.obj -o program.exe",
        asm_dir
    );

    printf("Executing: %s\n", cmd);
    int result = system(cmd);

    if (result == 0) {
        printf("✅ Successfully built: %s\\program.exe\n", asm_dir);
    } else {
        fprintf(stderr, "❌ Assembly or linking failed (exit code: %d).\n", result);
        return 1;
    }

    return 0;
}