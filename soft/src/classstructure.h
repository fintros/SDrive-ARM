#include "project.h"

class IBootLoader
{

};





class ISIODevice
{
public:
    virtual ~ISIODevice();
    virtual bool Process(unsigned char command, unsigned char aux1, unsigned char aux2) = 0;

private:

};

class CDeviceContainer
{
public: 
    bool Attach(unsigned char deviceID, ISIODevice& device);
    bool Detach(unsigned char deviceID);
    // always return some device - if device is not attached - than default device that always return NAK
    ISIODevice& operator [](unsigned char deviceID);
};


class CSquareDrive
{
public:
    CSquareDrive(IBootLoader& bootLoader);
    virtual ~CSquareDrive();

private:
    
public:
    // initialize devices, attach primary drive
    bool Prepare();
    void MainLoop();

};


void CSquareDrive::MainLoop()
{


}