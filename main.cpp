#include <iostream>
#include <cryptlib.h>
#include "ClientManager.h"

int main() {

ClientManager* manager = new ClientManager();
manager->Initialize();
manager->Run();

    return 0;
}
