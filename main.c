#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
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

        fprintf(stderr,
                "Usage: %s <source_file>\n",
                argv[0]);

        return EXIT_FAILURE;
    }

    FILE *source = fopen(argv[1], "r");

    if (!source) {

        perror("Failed to open source file");

        return EXIT_FAILURE;
    }

    /*
     * ============================================================
     * STAGE 1 : LEXICAL ANALYSIS
     * ============================================================
     */

    print_banner("STAGE 1: LEXICAL ANALYSIS");

    printf("Token stream for: %s\n\n",
           argv[1]);

    lexer_dump_all(source);

    /*
     * ============================================================
     * STAGE 2 : SYNTAX ANALYSIS
     * ============================================================
     */

    print_banner("STAGE 2: SYNTAX ANALYSIS (AST)");

    parser_init(source);

    ASTList *program = parse_program();

    if (!program) {

        fprintf(stderr,
                "Parsing failed\n");

        fclose(source);

        return EXIT_FAILURE;
    }

    ast_print_program(program);

    /*
     * ============================================================
     * STAGE 3 : SEMANTIC ANALYSIS
     * ============================================================
     */

    print_banner("STAGE 3: SEMANTIC ANALYSIS");

    semantic_check_program(program);

    /*
     * ============================================================
     * STAGE 4 : IR GENERATION
     * ============================================================
     */

    print_banner("STAGE 4: INTERMEDIATE REPRESENTATION (IR)");

    IRList ir;

    ir_list_init(&ir);

    ir_generate_program(&ir, program);

    ir_print(&ir);

    /*
     * ============================================================
     * STAGE 5 : OPTIMIZATION
     * ============================================================
     */

    print_banner("STAGE 5: OPTIMIZATION");

    ir_optimize(&ir);

    printf("\nOptimized IR:\n");

    ir_print(&ir);

  
    

    print_banner("STAGE 6: LLVM IR CODE GENERATION");

    FILE *ll = fopen("out.ll", "w");

    if (!ll) {

        perror("Failed to open out.ll for writing");

        ir_free(&ir);

        ast_list_free(program);

        fclose(source);

        return EXIT_FAILURE;
    }

    codegen_llvm_emit(&ir, ll);

    fclose(ll);

    printf("\nLLVM IR written to out.ll\n");


    print_banner("STAGE 7: PROGRAM EXECUTION (runtime output)");

    printf("Output from your program:\n\n");

    ir_interp_run(&ir);

    printf("\n");


    ir_free(&ir);

    ast_list_free(program);

    fclose(source);

    return EXIT_SUCCESS;
}