#include "ns3/ipv4.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"

#include <iostream>
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "smartmesh-routing.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    Time::SetResolution(Time::NS);

    NodeContainer nodes;
    nodes.Create(5);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel",
                               "MaxRange", DoubleValue(45.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(40.0),
        "DeltaY", DoubleValue(20.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    Ipv4StaticRoutingHelper staticRouting;
    SmartMeshRoutingHelper smartMesh;
    Ipv4ListRoutingHelper listRouting;

    // SmartMesh gets first opportunity to route.
    // Static routing is the fallback for routes it does not handle.
    listRouting.Add(smartMesh, 10);
    listRouting.Add(staticRouting, 0);

    InternetStackHelper internet;
    internet.SetRoutingHelper(listRouting);
    internet.Install(nodes);

    Ipv4AddressHelper addresses;
    addresses.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = addresses.Assign(devices);

    // SMARTMESH_CANDIDATE_PATHS
    // Candidate routes from node 0 to node 4.
    for (uint32_t nodeId = 0; nodeId < nodes.GetN(); ++nodeId)
    {
        Ptr<Ipv4> ipv4 = nodes.Get(nodeId)->GetObject<Ipv4>();
        Ptr<Ipv4ListRouting> routingList =
            DynamicCast<Ipv4ListRouting>(ipv4->GetRoutingProtocol());

        Ptr<SmartMeshRouting> routing = nullptr;

        if (routingList)
        {
            for (uint32_t i = 0;
                 i < routingList->GetNRoutingProtocols(); ++i)
            {
                int16_t priority;
                Ptr<Ipv4RoutingProtocol> protocol =
                    routingList->GetRoutingProtocol(i, priority);

                routing = DynamicCast<SmartMeshRouting>(protocol);

                if (routing)
                {
                    break;
                }
            }
        }

        if (!routing)
        {
            std::cerr << "SmartMesh not found on node "
                      << nodeId << std::endl;
            continue;
        }

        routing->SetCandidatePaths(
            interfaces.GetAddress(4),
            {
                {0, 1, 4},
                {0, 3, 4},
                {0, 1, 2, 4},
                {0, 3, 2, 4}
            });

        // Controlled congestion experiment: make path 0-1-4 expensive.
        routing->SetCongestionScore(1, 0.9);
        routing->SetCongestionScore(3, 0.1);
        routing->SetCongestionScore(4, 0.9);
        routing->SetCongestionScore(2, 0.1);
    }


    UdpServerHelper server(9000);
    ApplicationContainer serverApp = server.Install(nodes.Get(4));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(19.0));

    UdpClientHelper client(interfaces.GetAddress(4), 9000);
    client.SetAttribute("MaxPackets", UintegerValue(1000));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
    client.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(18.0));

    // Record Wi-Fi PHY transmission and reception events.
    phy.EnablePcap("results/traffic/moderate-trace", devices, true);

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    std::cout << "Starting SmartMesh baseline simulation..."
              << std::endl;

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();

    monitor->CheckForLostPackets();
    monitor->SerializeToXmlFile("results/traffic/moderate-flow-monitor.xml", true, true);

    std::cout << "Simulation finished. FlowMonitor results saved to "
              << "results/traffic/moderate-flow-monitor.xml" << std::endl;

    Simulator::Destroy();
    return 0;
}
