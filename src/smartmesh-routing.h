#ifndef SMARTMESH_ROUTING_H
#define SMARTMESH_ROUTING_H

#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4.h"
#include "ns3/node.h"

#include <map>
#include <vector>

namespace ns3
{

class SmartMeshRouting : public Ipv4RoutingProtocol
{
public:
    static TypeId GetTypeId();

    SmartMeshRouting();
    ~SmartMeshRouting() override;

    void SetIpv4(Ptr<Ipv4> ipv4) override;

    Ptr<Ipv4Route> RouteOutput(
        Ptr<Packet> packet,
        const Ipv4Header& header,
        Ptr<NetDevice> outputInterface,
        Socket::SocketErrno& error) override;

    bool RouteInput(
        Ptr<const Packet> packet,
        const Ipv4Header& header,
        Ptr<const NetDevice> inputDevice,
        const UnicastForwardCallback& unicastCallback,
        const MulticastForwardCallback& multicastCallback,
        const LocalDeliverCallback& localCallback,
        const ErrorCallback& errorCallback) override;

    void NotifyInterfaceUp(uint32_t interface) override;
    void NotifyInterfaceDown(uint32_t interface) override;
    void NotifyAddAddress(
        uint32_t interface,
        Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(
        uint32_t interface,
        Ipv4InterfaceAddress address) override;

    void PrintRoutingTable(
        Ptr<OutputStreamWrapper> stream,
        Time::Unit unit = Time::S) const override;

    void SetCandidatePaths(
        Ipv4Address destination,
        const std::vector<std::vector<uint32_t>>& paths);

    void SetCongestionScore(uint32_t nodeId, double score);
    void SetCriticalTraffic(bool critical);

private:
    Ptr<Ipv4Route> BuildRoute(
        Ipv4Address destination,
        Ptr<NetDevice> outputDevice) const;

    Ptr<Ipv4> m_ipv4;
    Ptr<Ipv4StaticRouting> m_staticRouting;

    std::map<Ipv4Address, std::vector<std::vector<uint32_t>>> m_paths;
    std::map<uint32_t, double> m_congestion;
    bool m_criticalTraffic;
};

class SmartMeshRoutingHelper : public Ipv4RoutingHelper
{
public:
    SmartMeshRoutingHelper();

    SmartMeshRoutingHelper* Copy() const override;
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;
};

} // namespace ns3

#endif
