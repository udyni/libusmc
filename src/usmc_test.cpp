#include <iostream>
#include <iomanip>
#include <libusmc.h>

using namespace std;

int main(void)
{
    cout << "USMC driver test program" << endl;

    USMC* usmc_driver = USMC::getInstance();
    int ndev = usmc_driver->probeDevices();
    cout << "Found " << ndev << " devices" << endl;

    for(size_t i = 0; i < ndev; i++) {

        cout << "==> Device " << i << endl;

        std::string serial("");
        usmc_driver->getSerialNumber(i, serial);
        cout << " * Serial: " << serial << endl;

        uint32_t version = 0;
        usmc_driver->getVersion(i, version);
        cout << " * Version: 0x" << std::hex << version << endl;

        USMC_State state;
        usmc_driver->getState(i, &state);
        cout << " * Current position: " << state.CurPos << endl;
        cout << " * Temperature: " << state.Temp << " degC" << endl;
        cout << " * Voltage: " << state.Voltage << " V" << endl;
    }
    return 0;
}
