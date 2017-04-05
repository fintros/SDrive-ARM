#ifndef _DPRINT_H_   
#define _DPRINT_H_

#ifdef DEBUG

    void dprint(const char* msg, ...);

#else

    #define dprint(X,...)
    
#endif
    
#endif