#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "ir.h"
#include "optimizer.h"
#include "codegen_llvm.h"
#include "ir_interp.h"

static void print_banner(const char *title) {
    printf("\n");
    printf("============================================================\n");
    printf("  %s\n", title);
    printf("============================================================\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) {
        perror("Failed to open source file");
        return EXIT_FAILURE;
    }

    print_banner("STAGE 1: LEXICAL ANALYSIS");
    printf("Token stream for: %s\n\n", argv[1]);
    lexer_dump_all(source);

    print_banner("STAGE 2: SYNTAX ANALYSIS (AST)");
    parser_init(source);
    ASTList *program = parse_program();
    if (!program) {
        fprintf(stderr, "Parsing failed\n");
        fclose(source);
        return EXIT_FAILURE;
    }
    ast_print_program(program);

    print_banner("STAGE 3: INTERMEDIATE REPRESENTATION (IR)");
    IRList ir;
    ir_list_init(&ir);
    ir_generate_program(&ir, program);
    ir_print(&ir);

    print_banner("STAGE 4: OPTIMIZATION");
    ir_optimize(&ir);
    printf("\nOptimized IR:\n");
    ir_print(&ir);

    print_banner("STAGE 5: LLVM IR CODE GENERATION");
    FILE *ll = fopen("output.ll", "w");
    if (!ll) {
        perror("Failed to open output.ll for writing");
        ir_free(&ir);
        ast_list_free(program);
        fclose(source);
        return EXIT_FAILURE;
    }
    codegen_llvm_emit(&ir, ll);
    fclose(ll);

    FILE *ll_copy = fopen("out.ll", "w");
    if (ll_copy) {
        FILE *in = fopen("output.ll", "r");
        if (in) {
            int c;
            while ((c = fgetc(in)) != EOF)
                fputc(c, ll_copy);
            fclose(in);
        }
        fclose(ll_copy);
    }

    FILE *show = fopen("output.ll", "r");
    if (show) {
        char buf[512];
        while (fgets(buf, sizeof(buf), show))
            fputs(buf, stdout);
        fclose(show);
    }
    printf("\nLLVM IR written to output.ll and out.ll\n");

    print_banner("STAGE 6: PROGRAM EXECUTION (runtime output)");
    printf("Output from your program:\n\n");
    ir_interp_run(&ir);
    printf("\n");

    ir_free(&ir);
    ast_list_free(program);
    fclose(source);
    return EXIT_SUCCESS;
}
