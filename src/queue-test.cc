#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <iostream>

using namespace ns3;

int main()
{
    NodeContainer nodes;
    nodes.Create(2);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        Ptr<WifiNetDevice> wifiDevice =
            DynamicCast<WifiNetDevice>(devices.Get(i));

        Ptr<WifiMac> wifiMac = wifiDevice->GetMac();
        Ptr<WifiMacQueue> queue = wifiMac->GetTxopQueue(AC_BE);

        std::cout << "Node " << i << ": ";

        if (queue)
        {
            std::cout << "Best-effort queue packets = "
                      << queue->GetNPackets()
                      << ", bytes = " << queue->GetNBytes() << '\n';
        }
        else
        {
            std::cout << "Best-effort queue unavailable\n";
        }
    }

    Simulator::Destroy();
    return 0;
}
