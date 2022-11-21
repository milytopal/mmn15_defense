#include <iostream>
#include <cryptlib.h>
#include "ClientManager.h"

int main() {

    auto manager = new ClientManager();
    manager->Initialize();
    manager->Run();
    delete manager;
    return 0;
}
