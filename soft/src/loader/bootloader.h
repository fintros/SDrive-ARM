#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

#include <sd/sdcard.h>

class CBootLoader
{   
private:
    CBootLoader();
    void setBuffers(void* memptr);

public:
    virtual ~CBootLoader();
    virtual ISDCard* getSDCard();

    static CBootLoader* getBootloader(void* memptr, int size);
    static int getRequiredSize();
};

#endif