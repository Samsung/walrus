#ifndef PARENT_C_FLAG
#error Parent C flags were lost
#endif
#if defined(WALRUS) || defined(ENABLE_GC) || defined(ENABLE_WASI) || defined(__STDC_LIMIT_MACROS)
#error Private library definitions reached an unrelated target
#endif
#if !defined(PARENT_DIRECTORY_OPTION) || !defined(PARENT_DIRECTORY_DEFINITION)
#error Parent directory settings were lost
#endif
int main(void) { return 0; }
