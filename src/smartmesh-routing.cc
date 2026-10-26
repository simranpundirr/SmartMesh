
#include "ns3/node-list.h"
#include "smartmesh-routing.h"
#include "ns3/ipv4-static-routing-helper.h"

#include "ns3/log.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SmartMeshRouting");
NS_OBJECT_ENSURE_REGISTERED(SmartMeshRouting);

TypeId
SmartMeshRouting::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SmartMeshRouting")
            .SetParent<Ipv4RoutingProtocol>()
            .SetGroupName("Internet")
            .AddConstructor<SmartMeshRouting>();
    return tid;
}

SmartMeshRouting::SmartMeshRouting()
    : m_criticalTraffic(false)
{
}

SmartMeshRouting::~SmartMeshRouting() = default;

void
SmartMeshRouting::SetIpv4(Ptr<Ipv4> ipv4)
{
    m_ipv4 = ipv4;
}

void
SmartMeshRouting::SetCandidatePaths(
    Ipv4Address destination,
    const std::vector<std::vector<uint32_t>>& paths)
{
    std::cerr << "SMARTMESH candidate paths configured: "
              << paths.size() << " for destination " << destination << std::endl;
    m_paths[destination] = paths;
}

void
SmartMeshRouting::SetCongestionScore(uint32_t nodeId, double score)
{
    m_congestion[nodeId] = std::clamp(score, 0.0, 1.0);
}

void
SmartMeshRouting::SetCriticalTraffic(bool critical)
{
    m_criticalTraffic = critical;
}

Ptr<Ipv4Route>
SmartMeshRouting::BuildRoute(
    Ipv4Address destination,
    Ptr<NetDevice> outputDevice) const
{
    if (!m_ipv4 || !outputDevice)
    {
        return nullptr;
    }

    int32_t interface =
        m_ipv4->GetInterfaceForDevice(outputDevice);

    if (interface < 0)
    {
        return nullptr;
    }

    Ptr<Ipv4Route> route = Create<Ipv4Route>();
    route->SetDestination(destination);
    route->SetGateway(Ipv4Address::GetZero());
    route->SetOutputDevice(outputDevice);
    route->SetSource(m_ipv4->GetAddress(interface, 0).GetLocal());

    return route;
}

Ptr<Ipv4Route>
SmartMeshRouting::RouteOutput(
    Ptr<Packet> packet,
    const Ipv4Header& header,
    Ptr<NetDevice> outputInterface,
    Socket::SocketErrno& error)
{
    static uint64_t routeOutputCalls = 0;
    ++routeOutputCalls;
    std::cerr << "SMARTMESH RouteOutput called: " << routeOutputCalls
              << ", destination=" << header.GetDestination() << std::endl;

    error = Socket::ERROR_NOROUTETOHOST;

    if (!m_ipv4)
    {
        return nullptr;
    }

    Ipv4Address destination = header.GetDestination();

    auto found = m_paths.find(destination);

    // For destinations with configured SmartMesh paths, use path selection.
    // Respect a caller-specified interface when resolving the selected hop.
    if (found == m_paths.end() || found->second.empty())
    {
        if (outputInterface)
        {
            Ptr<Ipv4Route> route = BuildRoute(destination, outputInterface);
            if (route)
            {
                error = Socket::ERROR_NOTERROR;
            }
            return route;
        }
        return nullptr;
    }
    if (found == m_paths.end() || found->second.empty())
    {
        return nullptr;
    }

    Ptr<Node> currentNode = m_ipv4->GetObject<Node>();
    if (!currentNode)
    {
        return nullptr;
    }

    uint32_t currentId = currentNode->GetId();
    const std::vector<uint32_t>* selectedPath = nullptr;
    size_t nextIndex = 0;
    double bestCost = std::numeric_limits<double>::max();

    for (const auto& path : found->second)
    {
        auto pos = std::find(path.begin(), path.end(), currentId);
        if (pos == path.end() || pos + 1 == path.end())
        {
            continue;
        }

        size_t index = static_cast<size_t>(pos - path.begin());
        double cost = 0.0;

        for (size_t i = index + 1; i < path.size(); ++i)
        {
            auto score = m_congestion.find(path[i]);
            if (score != m_congestion.end())
            {
                cost += score->second;
            }
        }

        if (cost < bestCost)
        {
            bestCost = cost;
            selectedPath = &path;
            nextIndex = index + 1;
        }
    }

    if (!selectedPath)
    {
        return nullptr;
    }

    uint32_t nextHopId = (*selectedPath)[nextIndex];
    Ptr<Node> nextNode = NodeList::GetNode(nextHopId);
    Ptr<Ipv4> nextIpv4 = nextNode ? nextNode->GetObject<Ipv4>() : nullptr;

    if (!nextIpv4)
    {
        return nullptr;
    }

    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); ++i)
    {
        Ptr<NetDevice> device = m_ipv4->GetNetDevice(i);
        if (!device || !device->IsLinkUp())
        {
            continue;
        }
        if (outputInterface && device != outputInterface)
        {
            continue;
        }

        for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); ++j)
        {
            Ipv4InterfaceAddress local = m_ipv4->GetAddress(i, j);
            if (local.GetLocal() == Ipv4Address::GetLoopback())
            {
                continue;
            }

            for (uint32_t k = 0; k < nextIpv4->GetNInterfaces(); ++k)
            {
                for (uint32_t l = 0; l < nextIpv4->GetNAddresses(k); ++l)
                {
                    Ipv4InterfaceAddress remote = nextIpv4->GetAddress(k, l);
                    if (remote.GetLocal() == Ipv4Address::GetLoopback())
                    {
                        continue;
                    }

                    if (local.GetLocal().CombineMask(local.GetMask()) ==
                        remote.GetLocal().CombineMask(local.GetMask()))
                    {
                        Ptr<Ipv4Route> route = Create<Ipv4Route>();
                        route->SetDestination(destination);
                        route->SetGateway(remote.GetLocal());
                        route->SetOutputDevice(device);
                        route->SetSource(local.GetLocal());

                        error = Socket::ERROR_NOTERROR;
                        std::cerr << "SMARTMESH DECISION: node="
                                  << currentId
                                  << " nextHop=" << nextHopId
                                  << " destination=" << destination
                                  << " cost=" << bestCost
                                  << " path=";
                        for (uint32_t pathNode : *selectedPath)
                        {
                            std::cerr << pathNode << " ";
                        }
                        std::cerr << std::endl;
                        return route;
                    }
                }
            }
        }
    }

    NS_LOG_WARN("SmartMesh node " << currentId
                << " could not resolve next hop " << nextHopId);
    return nullptr;
}

bool
SmartMeshRouting::RouteInput(
    Ptr<const Packet> packet,
    const Ipv4Header& header,
    Ptr<const NetDevice> inputDevice,
    const UnicastForwardCallback& unicastCallback,
    const MulticastForwardCallback& multicastCallback,
    const LocalDeliverCallback& localCallback,
    const ErrorCallback& errorCallback)
{
    if (!m_ipv4)
    {
        return false;
    }

    Ipv4Address destination = header.GetDestination();

    int32_t inputInterface = m_ipv4->GetInterfaceForDevice(inputDevice);

    if (inputInterface >= 0 &&
        m_ipv4->IsDestinationAddress(
            destination, static_cast<uint32_t>(inputInterface)))
    {
        if (!localCallback.IsNull())
        {
            localCallback(packet, header, m_ipv4->GetInterfaceForDevice(inputDevice));
            return true;
        }
    }

    auto found = m_paths.find(destination);

    if (found == m_paths.end() || found->second.empty())
    {
        return false;
    }

    Socket::SocketErrno error;
    Ptr<Ipv4Route> route =
        RouteOutput(packet->Copy(), header, nullptr, error);

    if (route && !unicastCallback.IsNull())
    {
        unicastCallback(route, packet, header);
        return true;
    }

    return false;
}

void
SmartMeshRouting::NotifyInterfaceUp(uint32_t interface)
{
}

void
SmartMeshRouting::NotifyInterfaceDown(uint32_t interface)
{
}

void
SmartMeshRouting::NotifyAddAddress(
    uint32_t interface,
    Ipv4InterfaceAddress address)
{
}

void
SmartMeshRouting::NotifyRemoveAddress(
    uint32_t interface,
    Ipv4InterfaceAddress address)
{
}

void
SmartMeshRouting::PrintRoutingTable(
    Ptr<OutputStreamWrapper> stream,
    Time::Unit unit) const
{
    *stream->GetStream() << "SmartMesh routing table\n";

    for (const auto& entry : m_paths)
    {
        *stream->GetStream()
            << "Destination: " << entry.first
            << ", candidate paths: " << entry.second.size()
            << '\n';
    }
}

SmartMeshRoutingHelper::SmartMeshRoutingHelper() = default;

SmartMeshRoutingHelper*
SmartMeshRoutingHelper::Copy() const
{
    return new SmartMeshRoutingHelper(*this);
}

Ptr<Ipv4RoutingProtocol>
SmartMeshRoutingHelper::Create(Ptr<Node> node) const
{
    (void)node;
    return CreateObject<SmartMeshRouting>();
}

} // namespace ns3

