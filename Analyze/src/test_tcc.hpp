#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "libtcc.h"

void handle_error(void *opaque, const char *msg)
{
    // Explicitly cast opaque to FILE*
    fprintf(static_cast<FILE*>(opaque), "%s\n", msg);
    // Alternatively:
    // fprintf(reinterpret_cast<FILE*>(opaque), "%s\n", msg);
}

/* this function is called by the generated code */
int add(int a, int b)
{
    return a + b;
}

/* this string is referenced by the generated code */
const char hello[] = "Hello World!";

char my_program[] =
"#include <tcclib.h>\n" /* include the "Simple libc header for TCC" */
"extern int add(int a, int b);\n"
"#ifdef _WIN32\n" /* dynamically linked data needs 'dllimport' */
" __attribute__((dllimport))\n"
"#endif\n"
"extern const char hello[];\n"
"int fib(int n)\n"
"{\n"
"    if (n <= 2)\n"
"        return 1;\n"
"    else\n"
"        return fib(n-1) + fib(n-2);\n"
"}\n"
"\n"
"int foo(int n)\n"
"{\n"
"    printf(\"%s\\n\", hello);\n"
"    printf(\"fib(%d) = %d\\n\", n, fib(n));\n"
"    printf(\"add(%d, %d) = %d\\n\", n, 2 * n, add(n, 2 * n));\n"
"    return 0;\n"
"}\n";

int libtcc_test()
{
    TCCState *s;
    int (*foo)(int);
    int (*fib)(int);
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }
    assert(tcc_get_error_func(s) == NULL);
    assert(tcc_get_error_opaque(s) == NULL);
    tcc_set_error_func(s, stderr, handle_error);
    assert(tcc_get_error_func(s) == handle_error);
    assert(tcc_get_error_opaque(s) == stderr);
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if (tcc_compile_string(s, my_program) == -1)
        return 1;
    tcc_add_symbol(s, "add", reinterpret_cast<const void*>(add));
    tcc_add_symbol(s, "hello", reinterpret_cast<const void*>(hello));
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0)
        return 1;
    foo = reinterpret_cast<int (*)(int)>(tcc_get_symbol(s, "foo"));
    fib = reinterpret_cast<int (*)(int)>(tcc_get_symbol(s, "fib"));
    if (!foo)
        return 1;
    if (!fib)
        return 1;
    foo(32);
    printf("%d\n", fib(9));
    tcc_delete(s);    
    return 0;
}
