#pragma once

extern bool error_asserted;
extern void on_error(const char* cond, const char* file, int line);
#define AssertEnabled 

#ifdef AssertEnabled
#define MASSERT(cond)\
    do\
    {\
        if (!(cond))\
        {\
            on_error(#cond, __FILE__, __LINE__);\
        }\
    } while(0)  
#else  
#define MASSERT(cond)\
    do { (void)sizeof(cond); } while(0) 
#endif