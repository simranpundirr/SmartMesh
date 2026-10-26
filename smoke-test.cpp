#include "ns3/core-module.h"
#include "ns3/network-module.h"

#include <iostream>

int main() {
    ns3::NodeContainer nodes;
    nodes.Create(3);

    std::cout << "SmartMesh ns-3 test passed: "
              << nodes.GetN() << " nodes created.\n";

    return 0;
}
