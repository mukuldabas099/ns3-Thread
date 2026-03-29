// /* =============================================================
//  * Thread Mesh Network - Packet Delivery Ratio (PDR) Simulation
//  * NS-3.36 Compatible - Fixed Version
//  * =============================================================
//  * Topology:
//  *   - 1 Border Router (sink/destination)
//  *   - N Mesh nodes (sources)
//  *   - IEEE 802.15.4 PHY/MAC (LR-WPAN)
//  *   - 6LoWPAN + UDP traffic
//  *   - AODV Routing
//  * =============================================================
//  */

// #include "ns3/core-module.h"
// #include "ns3/network-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/mobility-module.h"
// #include "ns3/lr-wpan-module.h"
// #include "ns3/sixlowpan-module.h"
// #include "ns3/applications-module.h"
// #include "ns3/aodv-module.h"
// #include "ns3/flow-monitor-module.h"

// #include <iostream>
// #include <fstream>
// #include <iomanip>
// #include <ctime>

// using namespace ns3;

// NS_LOG_COMPONENT_DEFINE("ThreadPDRSimulation");

// // ── Global Counters ────────────────────────────────────────────
// uint32_t g_totalSent     = 0;
// uint32_t g_totalReceived = 0;

// // ── Callbacks ──────────────────────────────────────────────────
// void PacketSentCallback(Ptr<const Packet> packet)
// {
//     g_totalSent++;
// }

// void PacketReceivedCallback(Ptr<const Packet> packet, const Address& address)
// {
//     g_totalReceived++;
// }

// // ── Main ───────────────────────────────────────────────────────
// int main(int argc, char* argv[])
// {
//     // Simulation parameters
//     uint32_t    nNodes         = 10;
//     double      simTime        = 60.0;
//     uint32_t    packetSize     = 64;
//     double      packetInterval = 1.0;
//     double      areaSize       = 50.0;
//     std::string outputFile     = "thread_pdr_results.csv";

//     CommandLine cmd;
//     cmd.AddValue("nNodes",         "Number of mesh nodes",  nNodes);
//     cmd.AddValue("simTime",        "Simulation time (s)",   simTime);
//     cmd.AddValue("packetSize",     "Packet size in bytes",  packetSize);
//     cmd.AddValue("packetInterval", "Packet interval (s)",   packetInterval);
//     cmd.AddValue("areaSize",       "Network area (m)",      areaSize);
//     cmd.AddValue("outputFile",     "CSV output filename",   outputFile);
//     cmd.Parse(argc, argv);

//     LogComponentEnable("ThreadPDRSimulation", LOG_LEVEL_INFO);

//     NS_LOG_INFO("=== Thread Mesh PDR Simulation ===");
//     NS_LOG_INFO("Nodes: " << nNodes << "  Time: " << simTime << "s"
//                 << "  PktSize: " << packetSize << "B"
//                 << "  Area: " << areaSize << "m");

//     // ── Nodes ──────────────────────────────────────────────────
//     NodeContainer borderRouter;  borderRouter.Create(1);
//     NodeContainer meshNodes;     meshNodes.Create(nNodes);
//     NodeContainer allNodes;
//     allNodes.Add(borderRouter);
//     allNodes.Add(meshNodes);

//     // ── LR-WPAN (IEEE 802.15.4) ────────────────────────────────
//     LrWpanHelper lrWpanHelper;
//     NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(allNodes);
//     lrWpanHelper.AssociateToPan(lrwpanDevices, 0);

//     // ── 6LoWPAN ────────────────────────────────────────────────
//     SixLowPanHelper sixLowPan;
//     sixLowPan.SetDeviceAttribute("ForceEtherType", BooleanValue(true));
//     NetDeviceContainer sixDevices = sixLowPan.Install(lrwpanDevices);

//     // ── Internet Stack + AODV ──────────────────────────────────
//     AodvHelper aodv;
//     InternetStackHelper internet;
//     internet.SetRoutingHelper(aodv);
//     internet.Install(allNodes);

//     // ── IPv6 Addresses ─────────────────────────────────────────
//     Ipv6AddressHelper ipv6;
//     ipv6.SetBase(Ipv6Address("2001:db8::"), Ipv6Prefix(64));
//     Ipv6InterfaceContainer ifaces = ipv6.Assign(sixDevices);
//     ifaces.SetForwarding(0, true);
//     ifaces.SetDefaultRouteInAllNodes(0);

//     // ── Mobility ───────────────────────────────────────────────
//     MobilityHelper mobility;

//     // Border Router at center
//     Ptr<ListPositionAllocator> brPos = CreateObject<ListPositionAllocator>();
//     brPos->Add(Vector(areaSize / 2.0, areaSize / 2.0, 0.0));
//     mobility.SetPositionAllocator(brPos);
//     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//     mobility.Install(borderRouter);

//     // Mesh nodes randomly placed
//     std::ostringstream xStr, yStr;
//     xStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
//     yStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
//     mobility.SetPositionAllocator(
//         "ns3::RandomRectanglePositionAllocator",
//         "X", StringValue(xStr.str()),
//         "Y", StringValue(yStr.str())
//     );
//     mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//     mobility.Install(meshNodes);

//     // ── Applications ───────────────────────────────────────────
//     uint16_t port = 9;

//     // Sink on Border Router
//     PacketSinkHelper sinkHelper(
//         "ns3::UdpSocketFactory",
//         Inet6SocketAddress(Ipv6Address::GetAny(), port)
//     );
//     ApplicationContainer sinkApp = sinkHelper.Install(borderRouter.Get(0));
//     sinkApp.Start(Seconds(0.0));
//     sinkApp.Stop(Seconds(simTime));

//     Ptr<PacketSink> sinkPtr = DynamicCast<PacketSink>(sinkApp.Get(0));
//     sinkPtr->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceivedCallback));

//     // Sources on mesh nodes → send UDP to Border Router
//     Ipv6Address brAddr = ifaces.GetAddress(0, 1);
//     uint32_t dataRate  = (uint32_t)(packetSize * 8.0 / packetInterval);

//     ApplicationContainer sourceApps;
//     for (uint32_t i = 0; i < meshNodes.GetN(); i++)
//     {
//         OnOffHelper src("ns3::UdpSocketFactory",
//                         Inet6SocketAddress(brAddr, port));
//         src.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
//         src.SetAttribute("PacketSize", UintegerValue(packetSize));
//         src.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
//         src.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

//         ApplicationContainer app = src.Install(meshNodes.Get(i));
//         app.Start(Seconds(1.0 + i * 0.1));   // staggered starts
//         app.Stop(Seconds(simTime - 1.0));
//         sourceApps.Add(app);
//     }

//     for (uint32_t i = 0; i < sourceApps.GetN(); i++)
//         sourceApps.Get(i)->TraceConnectWithoutContext(
//             "Tx", MakeCallback(&PacketSentCallback));

//     // ── Flow Monitor ───────────────────────────────────────────
//     FlowMonitorHelper fmHelper;
//     Ptr<FlowMonitor> monitor = fmHelper.InstallAll();

//     // ── Run ────────────────────────────────────────────────────
//     NS_LOG_INFO("Running...");
//     Simulator::Stop(Seconds(simTime));
//     Simulator::Run();

//     // ── Results ────────────────────────────────────────────────
//     monitor->CheckForLostPackets();
//     FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

//     uint64_t fmSent = 0, fmRecv = 0;
//     double   totalDelay = 0.0;
//     uint32_t flowCount  = 0;

//     std::cout << "\n";
//     std::cout << "==========================================================\n";
//     std::cout << "  Thread Mesh PDR Simulation - Per-Flow Results\n";
//     std::cout << "==========================================================\n";
//     std::cout << std::left
//               << std::setw(8)  << "FlowID"
//               << std::setw(12) << "Sent"
//               << std::setw(12) << "Received"
//               << std::setw(10) << "Lost"
//               << std::setw(10) << "PDR(%)"
//               << std::setw(14) << "AvgDelay(ms)"
//               << "\n";
//     std::cout << "----------------------------------------------------------\n";

//     for (auto& kv : stats)
//     {
//         fmSent += kv.second.txPackets;
//         fmRecv += kv.second.rxPackets;

//         double pdr = 0.0;
//         if (kv.second.txPackets > 0)
//             pdr = 100.0 * kv.second.rxPackets / kv.second.txPackets;

//         double delay = 0.0;
//         if (kv.second.rxPackets > 0)
//             delay = kv.second.delaySum.GetMilliSeconds() / kv.second.rxPackets;

//         totalDelay += delay;
//         flowCount++;

//         std::cout << std::left
//                   << std::setw(8)  << kv.first
//                   << std::setw(12) << kv.second.txPackets
//                   << std::setw(12) << kv.second.rxPackets
//                   << std::setw(10) << (kv.second.txPackets - kv.second.rxPackets)
//                   << std::setw(10) << std::fixed << std::setprecision(2) << pdr
//                   << std::setw(14) << std::fixed << std::setprecision(3) << delay
//                   << "\n";
//     }

//     // Overall PDR
//     double pdrCB = (g_totalSent > 0)
//                    ? 100.0 * g_totalReceived / g_totalSent : 0.0;
//     double pdrFM = (fmSent > 0)
//                    ? 100.0 * fmRecv / fmSent : 0.0;
//     double avgDelay = (flowCount > 0) ? totalDelay / flowCount : 0.0;

//     std::cout << "==========================================================\n";
//     std::cout << "\nOVERALL RESULTS:\n";
//     std::cout << "----------------------------------------------------------\n";
//     std::cout << "  Mesh Nodes (+ 1 Border Router) : " << nNodes + 1 << "\n";
//     std::cout << "  Simulation Time                : " << simTime << " s\n";
//     std::cout << "  Packet Size                    : " << packetSize << " bytes\n";
//     std::cout << "  Area                           : " << areaSize << "x" << areaSize << " m\n";
//     std::cout << "----------------------------------------------------------\n";
//     std::cout << "  [Callback]  Sent     : " << g_totalSent << "\n";
//     std::cout << "  [Callback]  Received : " << g_totalReceived << "\n";
//     std::cout << "  [Callback]  Lost     : " << g_totalSent - g_totalReceived << "\n";
//     std::cout << "  PDR (Callback)       : " << std::fixed << std::setprecision(2)
//               << pdrCB << " %\n";
//     std::cout << "----------------------------------------------------------\n";
//     std::cout << "  [FlowMon]   Sent     : " << fmSent << "\n";
//     std::cout << "  [FlowMon]   Received : " << fmRecv << "\n";
//     std::cout << "  [FlowMon]   Lost     : " << fmSent - fmRecv << "\n";
//     std::cout << "  PDR (FlowMonitor)    : " << std::fixed << std::setprecision(2)
//               << pdrFM << " %\n";
//     std::cout << "  Packet Loss Rate     : " << std::fixed << std::setprecision(2)
//               << (100.0 - pdrFM) << " %\n";
//     std::cout << "  Avg Network Delay    : " << std::fixed << std::setprecision(3)
//               << avgDelay << " ms\n";
//     std::cout << "==========================================================\n";


//     // ── Save CSV (Vertical Append Format) ───────────────────────────
// {
//     std::ofstream csv(outputFile, std::ios::app); // Open in append mode

//     // Get current timestamp for the run header
//     std::time_t now = std::time(nullptr);
//     char timeBuf[20];
//     std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

//     // Write a separator and timestamp to mark a new simulation run
//     csv << "\n# --- Simulation Run: " << timeBuf << " ---\n";
//     csv << "Parameter,Value\n";
    
//     // Write Data vertically
//     csv << "TotalNodes,"         << nNodes + 1 << "\n";
//     csv << "SimulationTime_s,"   << simTime << "\n";
//     csv << "PacketSize_bytes,"   << packetSize << "\n";
//     csv << "PacketInterval_s,"   << packetInterval << "\n";
//     csv << "AreaSize_m,"         << areaSize << "\n";
    
//     csv << "PacketsSent_CB,"     << g_totalSent << "\n";
//     csv << "PacketsReceived_CB," << g_totalReceived << "\n";
//     csv << "PacketsLost_CB,"     << (g_totalSent - g_totalReceived) << "\n";
//     csv << "PDR_CB_pct,"         << std::fixed << std::setprecision(4) << pdrCB << "\n";
    
//     csv << "PacketsSent_FM,"     << fmSent << "\n";
//     csv << "PacketsReceived_FM," << fmRecv << "\n";
//     csv << "PacketsLost_FM,"     << (fmSent - fmRecv) << "\n";
//     csv << "PDR_FM_pct,"         << std::fixed << std::setprecision(4) << pdrFM << "\n";
    
//     csv << "PacketLossRate_pct," << std::fixed << std::setprecision(4) << (100.0 - pdrFM) << "\n";
//     csv << "AvgDelay_ms,"        << std::fixed << std::setprecision(4) << avgDelay << "\n";

//     csv.close();
//     NS_LOG_INFO("Results appended vertically to: " << outputFile);
// }
// }








// /* =============================================================
//  * Thread Mesh Network - PDR + Throughput Simulation
//  * NS-3.40 Compatible
//  * =============================================================
//  * Topology:
//  *   - 1 Border Router (sink/destination)
//  *   - (totalNodes - 1) Mesh nodes (sources)
//  *   - IEEE 802.15.4 PHY/MAC (LR-WPAN)
//  *   - 6LoWPAN + UDP traffic
//  *   - AODV Routing
//  *
//  * CLI Usage Examples:
//  *   ./ns3 run "thread-pdr-simulation"
//  *   ./ns3 run "thread-pdr-simulation --totalNodes=11 --nPackets=100 --nodeSpacing=15 --pdrThreshold=80"
//  *   ./ns3 run "thread-pdr-simulation --totalNodes=6  --nPackets=50  --areaSize=40  --simTime=30"
//  *
//  * Parameters (all optional, defaults shown):
//  *   --totalNodes     Total nodes incl. border router  (default: 11  → 10 mesh + 1 BR)
//  *   --nPackets       Packets each mesh node sends     (default: 0   → unlimited, uses simTime)
//  *   --nodeSpacing    Fixed grid spacing in metres     (default: 0   → random placement)
//  *   --pdrThreshold   PDR % below which WARN is shown  (default: 80)
//  *   --simTime        Simulation duration (s)           (default: 60)
//  *   --packetSize     UDP payload bytes                 (default: 64)
//  *   --packetInterval Seconds between packets           (default: 1.0)
//  *   --areaSize       Random area side length (m)       (default: 50, ignored if nodeSpacing>0)
//  *   --outputFile     CSV output path                   (default: thread_pdr_results.csv)
//  * =============================================================
//  */

// #include "ns3/core-module.h"
// #include "ns3/network-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/mobility-module.h"
// #include "ns3/lr-wpan-module.h"
// #include "ns3/sixlowpan-module.h"
// #include "ns3/applications-module.h"
// #include "ns3/aodv-module.h"
// #include "ns3/flow-monitor-module.h"

// #include <iostream>
// #include <fstream>
// #include <iomanip>
// #include <ctime>
// #include <cmath>

// using namespace ns3;

// NS_LOG_COMPONENT_DEFINE("ThreadPDRSimulation");

// // ── Global Counters ────────────────────────────────────────────
// uint32_t g_totalSent     = 0;
// uint32_t g_totalReceived = 0;

// void PacketSentCallback(Ptr<const Packet> packet)     { g_totalSent++;     }
// void PacketReceivedCallback(Ptr<const Packet> packet,
//                             const Address&  address)  { g_totalReceived++; }

// // ── Helper: print a separator line ────────────────────────────
// static void Sep(char c = '-', int w = 62)
// { std::cout << std::string(w, c) << "\n"; }

// // ── Main ───────────────────────────────────────────────────────
// int main(int argc, char* argv[])
// {
//     // ── Default parameters ─────────────────────────────────────
//     uint32_t    totalNodes     = 11;     // 1 BR + 10 mesh
//     uint32_t    nPackets       = 0;      // 0 = unlimited (use simTime)
//     double      nodeSpacing    = 0.0;    // 0 = random, >0 = grid (metres)
//     double      pdrThreshold   = 80.0;  // WARN if PDR < this %

//     double      simTime        = 60.0;
//     uint32_t    packetSize     = 64;
//     double      packetInterval = 1.0;
//     double      areaSize       = 50.0;
//     std::string outputFile     = "thread_pdr_results.csv";

//     // ── Command-line parsing ───────────────────────────────────
//     CommandLine cmd;
//     cmd.AddValue("totalNodes",     "Total nodes (BR + mesh)",          totalNodes);
//     cmd.AddValue("nPackets",       "Pkts per node (0=unlimited)",       nPackets);
//     cmd.AddValue("nodeSpacing",    "Grid spacing m (0=random area)",    nodeSpacing);
//     cmd.AddValue("pdrThreshold",   "PDR % warning threshold",          pdrThreshold);
//     cmd.AddValue("simTime",        "Simulation time (s)",              simTime);
//     cmd.AddValue("packetSize",     "Packet size (bytes)",              packetSize);
//     cmd.AddValue("packetInterval", "Interval between packets (s)",     packetInterval);
//     cmd.AddValue("areaSize",       "Random area side (m)",             areaSize);
//     cmd.AddValue("outputFile",     "CSV output path",                  outputFile);
//     cmd.Parse(argc, argv);

//     // Guard: need at least 2 nodes (1 BR + 1 mesh)
//     if (totalNodes < 2)
//     {
//         std::cerr << "ERROR: totalNodes must be >= 2\n";
//         return 1;
//     }
//     uint32_t nMesh = totalNodes - 1;   // mesh nodes (sources)

//     LogComponentEnable("ThreadPDRSimulation", LOG_LEVEL_INFO);

//     // ── Print config banner ────────────────────────────────────
//     Sep('=');
//     std::cout << "  Thread Mesh PDR + Throughput Simulation\n";
//     Sep('=');
//     std::cout << "  Total nodes      : " << totalNodes
//               << "  (1 BR + " << nMesh << " mesh)\n";
//     if (nPackets > 0)
//         std::cout << "  Packets/node     : " << nPackets << "\n";
//     else
//         std::cout << "  Packets/node     : unlimited (simTime=" << simTime << "s)\n";
//     if (nodeSpacing > 0)
//         std::cout << "  Node spacing     : " << nodeSpacing << " m (grid)\n";
//     else
//         std::cout << "  Area             : " << areaSize << "x" << areaSize << " m (random)\n";
//     std::cout << "  Packet size      : " << packetSize << " bytes\n";
//     std::cout << "  Packet interval  : " << packetInterval << " s\n";
//     std::cout << "  PDR threshold    : " << pdrThreshold << " %\n";
//     std::cout << "  Output CSV       : " << outputFile << "\n";
//     Sep('=');

//     // ── Nodes ──────────────────────────────────────────────────
//     NodeContainer borderRouter; borderRouter.Create(1);
//     NodeContainer meshNodes;    meshNodes.Create(nMesh);
//     NodeContainer allNodes;
//     allNodes.Add(borderRouter);
//     allNodes.Add(meshNodes);

//     // ── LR-WPAN (IEEE 802.15.4) ────────────────────────────────
//     LrWpanHelper lrWpanHelper;
//     NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(allNodes);
//     lrWpanHelper.AssociateToPan(lrwpanDevices, 0);

//     // ── 6LoWPAN ────────────────────────────────────────────────
//     SixLowPanHelper sixLowPan;
//     sixLowPan.SetDeviceAttribute("ForceEtherType", BooleanValue(true));
//     NetDeviceContainer sixDevices = sixLowPan.Install(lrwpanDevices);

//     // ── Internet Stack + AODV ──────────────────────────────────
//     AodvHelper aodv;
//     InternetStackHelper internet;
//     internet.SetRoutingHelper(aodv);
//     internet.Install(allNodes);

//     // ── IPv6 Addresses ─────────────────────────────────────────
//     Ipv6AddressHelper ipv6;
//     ipv6.SetBase(Ipv6Address("2001:db8::"), Ipv6Prefix(64));
//     Ipv6InterfaceContainer ifaces = ipv6.Assign(sixDevices);
//     ifaces.SetForwarding(0, true);
//     ifaces.SetDefaultRouteInAllNodes(0);

//     // ── Mobility ───────────────────────────────────────────────
//     MobilityHelper mobility;

//     if (nodeSpacing > 0.0)
//     {
//         // ── Grid layout: BR at origin, mesh nodes in a row ────
//         // Row layout: BR at (0,0), mesh at (spacing, 0), (2*spacing, 0), …
//         Ptr<ListPositionAllocator> posAlloc =
//             CreateObject<ListPositionAllocator>();

//         // Border Router at origin
//         posAlloc->Add(Vector(0.0, 0.0, 0.0));

//         // Mesh nodes spaced along X axis
//         for (uint32_t i = 0; i < nMesh; i++)
//             posAlloc->Add(Vector((i + 1) * nodeSpacing, 0.0, 0.0));

//         mobility.SetPositionAllocator(posAlloc);
//         mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//         mobility.Install(allNodes);

//         NS_LOG_INFO("Grid layout: spacing=" << nodeSpacing
//                     << "m, total span=" << nMesh * nodeSpacing << "m");
//     }
//     else
//     {
//         // ── Random layout inside areaSize x areaSize ──────────
//         // Border Router at center
//         Ptr<ListPositionAllocator> brPos =
//             CreateObject<ListPositionAllocator>();
//         brPos->Add(Vector(areaSize / 2.0, areaSize / 2.0, 0.0));
//         mobility.SetPositionAllocator(brPos);
//         mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//         mobility.Install(borderRouter);

//         // Mesh nodes randomly placed
//         std::ostringstream xStr, yStr;
//         xStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
//         yStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
//         mobility.SetPositionAllocator(
//             "ns3::RandomRectanglePositionAllocator",
//             "X", StringValue(xStr.str()),
//             "Y", StringValue(yStr.str()));
//         mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
//         mobility.Install(meshNodes);
//     }

//     // ── Applications ───────────────────────────────────────────
//     uint16_t port = 9;

//     // Sink on Border Router
//     PacketSinkHelper sinkHelper(
//         "ns3::UdpSocketFactory",
//         Inet6SocketAddress(Ipv6Address::GetAny(), port));
//     ApplicationContainer sinkApp = sinkHelper.Install(borderRouter.Get(0));
//     sinkApp.Start(Seconds(0.0));
//     sinkApp.Stop(Seconds(simTime));

//     Ptr<PacketSink> sinkPtr = DynamicCast<PacketSink>(sinkApp.Get(0));
//     sinkPtr->TraceConnectWithoutContext(
//         "Rx", MakeCallback(&PacketReceivedCallback));

//     // Sources on mesh nodes → send UDP to Border Router
//     Ipv6Address brAddr = ifaces.GetAddress(0, 1);
//     uint32_t dataRate  = (uint32_t)(packetSize * 8.0 / packetInterval);

//     ApplicationContainer sourceApps;
//     for (uint32_t i = 0; i < nMesh; i++)
//     {
//         OnOffHelper src("ns3::UdpSocketFactory",
//                         Inet6SocketAddress(brAddr, port));
//         src.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
//         src.SetAttribute("PacketSize", UintegerValue(packetSize));
//         src.SetAttribute("OnTime",
//             StringValue("ns3::ConstantRandomVariable[Constant=1]"));
//         src.SetAttribute("OffTime",
//             StringValue("ns3::ConstantRandomVariable[Constant=0]"));

//         // If nPackets > 0, limit total bytes sent
//         if (nPackets > 0)
//             src.SetAttribute("MaxBytes",
//                 UintegerValue(nPackets * packetSize));

//         ApplicationContainer app = src.Install(meshNodes.Get(i));
//         app.Start(Seconds(1.0 + i * 0.1));   // staggered starts ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//         app.Stop(Seconds(simTime - 1.0));
//         sourceApps.Add(app);
//     }

//     for (uint32_t i = 0; i < sourceApps.GetN(); i++)
//         sourceApps.Get(i)->TraceConnectWithoutContext(
//             "Tx", MakeCallback(&PacketSentCallback));

//     // ── Flow Monitor ───────────────────────────────────────────
//     FlowMonitorHelper fmHelper;
//     Ptr<FlowMonitor> monitor = fmHelper.InstallAll();

//     // ── Run ────────────────────────────────────────────────────
//     NS_LOG_INFO("Simulation running...");
//     Simulator::Stop(Seconds(simTime));
//     Simulator::Run();

//     // ── Collect Results ────────────────────────────────────────
//     monitor->CheckForLostPackets();
//     FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

//     uint64_t fmSent       = 0;
//     uint64_t fmRecv       = 0;
//     double   totalDelay   = 0.0;
//     double   totalTxBytes = 0.0;   // bytes sent   (all flows)
//     double   totalRxBytes = 0.0;   // bytes received
//     uint32_t flowCount    = 0;

//     // ── Per-flow table ─────────────────────────────────────────
//     std::cout << "\n";
//     Sep('=');
//     std::cout << "  Thread Mesh - Per-Flow Results\n";
//     Sep('=');
//     std::cout << std::left
//               << std::setw(8)  << "FlowID"
//               << std::setw(10) << "Sent"
//               << std::setw(10) << "Recv"
//               << std::setw(8)  << "Lost"
//               << std::setw(10) << "PDR(%)"
//               << std::setw(14) << "Delay(ms)"
//               << std::setw(16) << "Throughput(kbps)"
//               << "\n";
//     Sep();

//     for (auto& kv : stats)
//     {
//         fmSent += kv.second.txPackets;
//         fmRecv += kv.second.rxPackets;
//         totalTxBytes += kv.second.txBytes;
//         totalRxBytes += kv.second.rxBytes;

//         double pdr = 0.0;
//         if (kv.second.txPackets > 0)
//             pdr = 100.0 * kv.second.rxPackets / kv.second.txPackets;

//         double delay = 0.0;
//         if (kv.second.rxPackets > 0)
//             delay = kv.second.delaySum.GetMilliSeconds()
//                     / kv.second.rxPackets;

//         // Flow throughput = received bits / simulation time (kbps)
//         double tput = (kv.second.rxBytes * 8.0) / (simTime * 1000.0);

//         totalDelay += delay;
//         flowCount++;

//         std::cout << std::left
//                   << std::setw(8)  << kv.first
//                   << std::setw(10) << kv.second.txPackets
//                   << std::setw(10) << kv.second.rxPackets
//                   << std::setw(8)  << (kv.second.txPackets - kv.second.rxPackets)
//                   << std::setw(10) << std::fixed << std::setprecision(2) << pdr
//                   << std::setw(14) << std::fixed << std::setprecision(3) << delay
//                   << std::setw(16) << std::fixed << std::setprecision(3) << tput
//                   << "\n";
//     }

//     // ── Overall metrics ────────────────────────────────────────
//     double pdrCB  = (g_totalSent > 0)
//                     ? 100.0 * g_totalReceived / g_totalSent : 0.0;
//     double pdrFM  = (fmSent > 0)
//                     ? 100.0 * fmRecv / fmSent : 0.0;
//     double avgDelay    = (flowCount > 0) ? totalDelay / flowCount : 0.0;

//     // Network throughput = total received bits / simTime
//     double netThroughputKbps = (totalRxBytes * 8.0) / (simTime * 1000.0);
//     // Per-node throughput
//     double perNodeTputKbps   = (nMesh > 0)
//                                ? netThroughputKbps / nMesh : 0.0;
//     // Goodput = useful data received (payload only, approx same as rxBytes for UDP)
//     double goodputKbps = netThroughputKbps;   // same metric, labelled separately

//     bool   passThreshold = (pdrFM >= pdrThreshold);

//     Sep('=');
//     std::cout << "\n  OVERALL RESULTS\n";
//     Sep();
//     std::cout << "  Total Nodes (BR + mesh)  : " << totalNodes
//               << "  (" << nMesh << " sources)\n";
//     std::cout << "  Simulation Time          : " << simTime << " s\n";
//     std::cout << "  Packet Size              : " << packetSize << " bytes\n";
//     std::cout << "  Packet Interval          : " << packetInterval << " s\n";
//     if (nodeSpacing > 0)
//         std::cout << "  Node Spacing             : " << nodeSpacing << " m\n";
//     else
//         std::cout << "  Area                     : "
//                   << areaSize << "x" << areaSize << " m\n";
//     Sep();
//     std::cout << "  [Callback] Sent          : " << g_totalSent    << "\n";
//     std::cout << "  [Callback] Received      : " << g_totalReceived << "\n";
//     std::cout << "  [Callback] Lost          : "
//               << g_totalSent - g_totalReceived << "\n";
//     std::cout << "  PDR (Callback)           : " << std::fixed
//               << std::setprecision(2) << pdrCB << " %\n";
//     Sep();
//     std::cout << "  [FlowMon]  Sent          : " << fmSent << "\n";
//     std::cout << "  [FlowMon]  Received      : " << fmRecv << "\n";
//     std::cout << "  [FlowMon]  Lost          : " << fmSent - fmRecv << "\n";
//     std::cout << "  PDR (FlowMonitor)        : " << std::fixed
//               << std::setprecision(2) << pdrFM << " %\n";
//     std::cout << "  Packet Loss Rate         : " << std::fixed
//               << std::setprecision(2) << (100.0 - pdrFM) << " %\n";
//     std::cout << "  Avg Network Delay        : " << std::fixed
//               << std::setprecision(3) << avgDelay << " ms\n";
//     Sep();
//     std::cout << "  Network Throughput       : " << std::fixed
//               << std::setprecision(3) << netThroughputKbps << " kbps\n";
//     std::cout << "  Per-Node Throughput      : " << std::fixed
//               << std::setprecision(3) << perNodeTputKbps << " kbps\n";
//     std::cout << "  Goodput                  : " << std::fixed
//               << std::setprecision(3) << goodputKbps << " kbps\n";
//     std::cout << "  Total TX bytes           : " << (uint64_t)totalTxBytes << "\n";
//     std::cout << "  Total RX bytes           : " << (uint64_t)totalRxBytes << "\n";
//     Sep();
//     // Threshold verdict
//     std::cout << "  PDR Threshold            : " << pdrThreshold << " %\n";
//     std::cout << "  Network Quality          : "
//               << (passThreshold ? "PASS ✓  (PDR >= threshold)"
//                                 : "WARN ✗  (PDR < threshold — mesh degraded)")
//               << "\n";
//     Sep('=');

//     // ── Save CSV (horizontal append) ───────────────────────────
//     {
//         // Write header only when file is new / empty
//         bool writeHeader = false;
//         {
//             std::ifstream chk(outputFile);
//             writeHeader = !chk.good() ||
//                 chk.peek() == std::ifstream::traits_type::eof();
//         }

//         std::ofstream csv(outputFile, std::ios::app);

//         if (writeHeader)
//         {
//             csv << "Timestamp,"
//                 << "TotalNodes,MeshNodes,SimTime_s,"
//                 << "PacketSize_bytes,PacketInterval_s,"
//                 << "NodeSpacing_m,AreaSize_m,"
//                 << "NPackets_per_node,"
//                 << "PDRthreshold_pct,"
//                 // Callback
//                 << "Sent_CB,Recv_CB,Lost_CB,PDR_CB_pct,"
//                 // FlowMonitor
//                 << "Sent_FM,Recv_FM,Lost_FM,PDR_FM_pct,"
//                 << "PacketLoss_pct,AvgDelay_ms,"
//                 // Throughput
//                 << "NetThroughput_kbps,PerNodeTput_kbps,Goodput_kbps,"
//                 << "TxBytes,RxBytes,"
//                 // Verdict
//                 << "QualityPass\n";
//         }

//         // Timestamp
//         std::time_t now = std::time(nullptr);
//         char timeBuf[20];
//         std::strftime(timeBuf, sizeof(timeBuf),
//                       "%Y-%m-%d %H:%M:%S", std::localtime(&now));

//         csv << timeBuf                                                << ","
//             << totalNodes                                             << ","
//             << nMesh                                                  << ","
//             << simTime                                                << ","
//             << packetSize                                             << ","
//             << packetInterval                                         << ","
//             << nodeSpacing                                            << ","
//             << areaSize                                               << ","
//             << nPackets                                               << ","
//             << pdrThreshold                                           << ","
//             // Callback
//             << g_totalSent                                            << ","
//             << g_totalReceived                                        << ","
//             << (g_totalSent - g_totalReceived)                        << ","
//             << std::fixed << std::setprecision(4) << pdrCB           << ","
//             // FlowMonitor
//             << fmSent                                                 << ","
//             << fmRecv                                                 << ","
//             << (fmSent - fmRecv)                                      << ","
//             << std::fixed << std::setprecision(4) << pdrFM           << ","
//             << std::fixed << std::setprecision(4) << (100.0 - pdrFM) << ","
//             << std::fixed << std::setprecision(4) << avgDelay        << ","
//             // Throughput
//             << std::fixed << std::setprecision(4) << netThroughputKbps << ","
//             << std::fixed << std::setprecision(4) << perNodeTputKbps   << ","
//             << std::fixed << std::setprecision(4) << goodputKbps       << ","
//             << (uint64_t)totalTxBytes                                   << ","
//             << (uint64_t)totalRxBytes                                   << ","
//             // Verdict
//             << (passThreshold ? "PASS" : "WARN")                        << "\n";

//         csv.close();
//         NS_LOG_INFO("CSV row appended: " << outputFile);
//     }

//     monitor->SerializeToXmlFile("thread_pdr_flowmonitor.xml", true, true);
//     NS_LOG_INFO("FlowMonitor XML: thread_pdr_flowmonitor.xml");

//     Simulator::Destroy();
//     NS_LOG_INFO("Done.");
//     return 0;
// }











































/* =============================================================
 * Thread Mesh Network - PDR + Throughput Simulation
 * NS-3.40 Compatible
 * =============================================================
 * Topology:
 *   - 1 Border Router (sink/destination)
 *   - (totalNodes - 1) Mesh nodes (sources)
 *   - IEEE 802.15.4 PHY/MAC (LR-WPAN)
 *   - 6LoWPAN + UDP traffic
 *   - AODV Routing
 *
 * CLI Usage Examples:
 *   ./ns3 run "thread-pdr-simulation"
 *   ./ns3 run "thread-pdr-simulation --totalNodes=11 --nPackets=100 --nodeSpacing=15"
 *   ./ns3 run "thread-pdr-simulation --totalNodes=6  --nPackets=50  --areaSize=40 --simTime=30"
 *
 * Parameters (all optional, defaults shown):
 *   --totalNodes     Total nodes incl. border router  (default: 11  -> 10 mesh + 1 BR)
 *   --nPackets       Packets each mesh node sends     (default: 0   -> unlimited, uses simTime)
 *   --nodeSpacing    Fixed grid spacing in metres     (default: 0   -> random placement)
 *   --simTime        Simulation duration (s)           (default: 60)
 *   --packetSize     UDP payload bytes                 (default: 64)
 *   --packetInterval Seconds between packets           (default: 1.0)
 *   --areaSize       Random area side length (m)       (default: 50, ignored if nodeSpacing>0)
 *   --outputFile     CSV output path                   (default: thread_pdr_results.csv)
 *
 * NOTE: No threshold parameter needed.
 *       Network quality and threshold breakpoint are auto-calculated
 *       from PDR and delay using IEEE 802.15.4 / Thread mesh standards.
 * =============================================================
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreadPDRSimulation");

// ── Global Counters ────────────────────────────────────────────
uint32_t g_totalSent     = 0;
uint32_t g_totalReceived = 0;

void PacketSentCallback    (Ptr<const Packet> p)                 { g_totalSent++;     }
void PacketReceivedCallback(Ptr<const Packet> p, const Address&) { g_totalReceived++; }

// ── Separator ──────────────────────────────────────────────────
static void Sep(char c = '-', int w = 64)
{ std::cout << std::string(w, c) << "\n"; }

// ── Auto Network Quality Classifier ───────────────────────────
//
//  Fixed engineering thresholds (IEEE 802.15.4 / Thread spec):
//
//    EXCELLENT : PDR >= 95 %  AND  avg delay <  100 ms
//    GOOD      : PDR >= 85 %  AND  avg delay <  500 ms
//    DEGRADED  : PDR >= 60 %
//    POOR      : PDR <  60 %
//
//  Returns the quality label and fills qualityReason + thresholdReport.
// ──────────────────────────────────────────────────────────────
static std::string ClassifyNetwork(double       pdrFM,
                                   double       avgDelay,
                                   double       netTputKbps,
                                   uint32_t     nMesh,
                                   std::string& qualityReason,
                                   std::string& thresholdReport)
{
    // Reference thresholds (not user-configurable)
    const double PDR_EXCELLENT   = 95.0;   // %
    const double PDR_GOOD        = 85.0;   // %
    const double PDR_DEGRADED    = 60.0;   // %
    const double DELAY_EXCELLENT = 100.0;  // ms
    const double DELAY_GOOD      = 500.0;  // ms

    std::string label;

    if (pdrFM >= PDR_EXCELLENT && avgDelay < DELAY_EXCELLENT)
    {
        label         = "EXCELLENT";
        qualityReason = "PDR >= 95% and delay < 100 ms — mesh at full capacity.";
        thresholdReport =
            "  The mesh is perfectly working at this configuration.\n"
            "  Degradation begins when PDR falls below 95% OR delay rises above 100 ms.\n"
            "  Safe operating limit: keep adding nodes until PDR drops below 95%.";
    }
    else if (pdrFM >= PDR_GOOD && avgDelay < DELAY_GOOD)
    {
        label         = "GOOD";
        qualityReason = "PDR >= 85% and delay < 500 ms — mesh is stable and usable.";
        thresholdReport =
            "  The mesh is working but is no longer in its optimal zone.\n"
            "  It crossed the EXCELLENT threshold (PDR < 95% or delay > 100 ms).\n"
            "  It will become DEGRADED if PDR drops below 85% or delay exceeds 500 ms.\n"
            "  Recommendation: this node count / spacing is near the usable upper limit.";
    }
    else if (pdrFM >= PDR_DEGRADED)
    {
        label         = "DEGRADED";
        qualityReason = "PDR is between 60-85% — some routing paths are failing.";
        thresholdReport =
            "  The mesh has crossed the GOOD threshold (PDR dropped below 85%).\n"
            "  The network is degraded — packet loss is significant.\n"
            "  The mesh was working well BEFORE this point (fewer nodes / smaller area).\n"
            "  Action: reduce node count, reduce spacing, or increase TX power.";
    }
    else
    {
        label         = "POOR";
        qualityReason = "PDR < 60% — mesh is severely congested or nodes are out of range.";
        thresholdReport =
            "  The mesh has crossed the DEGRADED threshold (PDR dropped below 60%).\n"
            "  The network is barely functional at this configuration.\n"
            "  Action: significantly reduce node density or increase link quality.";
    }

    return label;
}

// ── Main ───────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    // ── Defaults ───────────────────────────────────────────────
    uint32_t    totalNodes     = 11;
    uint32_t    nPackets       = 0;
    double      nodeSpacing    = 0.0;
    double      simTime        = 60.0;
    uint32_t    packetSize     = 64;
    double      packetInterval = 1.0;
    double      areaSize       = 50.0;
    std::string outputFile     = "thread_pdr_results.csv";

    CommandLine cmd;
    cmd.AddValue("totalNodes",     "Total nodes (BR + mesh)",        totalNodes);
    cmd.AddValue("nPackets",       "Pkts per node (0=unlimited)",     nPackets);
    cmd.AddValue("nodeSpacing",    "Grid spacing m (0=random area)",  nodeSpacing);
    cmd.AddValue("simTime",        "Simulation time (s)",            simTime);
    cmd.AddValue("packetSize",     "Packet size (bytes)",            packetSize);
    cmd.AddValue("packetInterval", "Interval between packets (s)",   packetInterval);
    cmd.AddValue("areaSize",       "Random area side (m)",           areaSize);
    cmd.AddValue("outputFile",     "CSV output path",                outputFile);
    cmd.Parse(argc, argv);

    if (totalNodes < 2)
    {
        std::cerr << "ERROR: totalNodes must be >= 2\n";
        return 1;
    }
    uint32_t nMesh = totalNodes - 1;

    LogComponentEnable("ThreadPDRSimulation", LOG_LEVEL_INFO);

    // ── Config banner ──────────────────────────────────────────
    Sep('=');
    std::cout << "  Thread Mesh PDR + Throughput Simulation\n";
    Sep('=');
    std::cout << "  Total nodes      : " << totalNodes
              << "  (1 BR + " << nMesh << " mesh)\n";
    if (nPackets > 0)
        std::cout << "  Packets/node     : " << nPackets << "\n";
    else
        std::cout << "  Packets/node     : unlimited  (simTime = " << simTime << " s)\n";
    if (nodeSpacing > 0.0)
        std::cout << "  Node spacing     : " << nodeSpacing << " m  (grid layout)\n";
    else
        std::cout << "  Area             : " << areaSize << "x" << areaSize << " m  (random)\n";
    std::cout << "  Packet size      : " << packetSize     << " bytes\n";
    std::cout << "  Packet interval  : " << packetInterval << " s\n";
    std::cout << "  Output CSV       : " << outputFile     << "\n";
    Sep('=');

    // ── Nodes ──────────────────────────────────────────────────
    NodeContainer borderRouter; borderRouter.Create(1);
    NodeContainer meshNodes;    meshNodes.Create(nMesh);
    NodeContainer allNodes;
    allNodes.Add(borderRouter);
    allNodes.Add(meshNodes);

    // ── LR-WPAN ────────────────────────────────────────────────
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(allNodes);
    lrWpanHelper.AssociateToPan(lrwpanDevices, 0);

    // ── 6LoWPAN ────────────────────────────────────────────────
    SixLowPanHelper sixLowPan;
    sixLowPan.SetDeviceAttribute("ForceEtherType", BooleanValue(true));
    NetDeviceContainer sixDevices = sixLowPan.Install(lrwpanDevices);

    // ── Internet Stack + AODV ──────────────────────────────────
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(allNodes);

    // ── IPv6 Addresses ─────────────────────────────────────────
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:db8::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer ifaces = ipv6.Assign(sixDevices);
    ifaces.SetForwarding(0, true);
    ifaces.SetDefaultRouteInAllNodes(0);

    // ── Mobility ───────────────────────────────────────────────
    MobilityHelper mobility;

    if (nodeSpacing > 0.0)
    {
        // Grid: BR at origin, mesh nodes in a line along X axis
        Ptr<ListPositionAllocator> posAlloc =
            CreateObject<ListPositionAllocator>();
        posAlloc->Add(Vector(0.0, 0.0, 0.0));
        for (uint32_t i = 0; i < nMesh; i++)
            posAlloc->Add(Vector((i + 1) * nodeSpacing, 0.0, 0.0));
        mobility.SetPositionAllocator(posAlloc);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(allNodes);
    }
    else
    {
        // Random: BR at centre, mesh randomly inside areaSize x areaSize
        Ptr<ListPositionAllocator> brPos =
            CreateObject<ListPositionAllocator>();
        brPos->Add(Vector(areaSize / 2.0, areaSize / 2.0, 0.0));
        mobility.SetPositionAllocator(brPos);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(borderRouter);

        std::ostringstream xStr, yStr;
        xStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
        yStr << "ns3::UniformRandomVariable[Min=0|Max=" << (int)areaSize << "]";
        mobility.SetPositionAllocator(
            "ns3::RandomRectanglePositionAllocator",
            "X", StringValue(xStr.str()),
            "Y", StringValue(yStr.str()));
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(meshNodes);
    }

    // ── Applications ───────────────────────────────────────────
    uint16_t port     = 9;
    uint32_t dataRate = (uint32_t)(packetSize * 8.0 / packetInterval);

    // Sink on Border Router
    PacketSinkHelper sinkHelper(
        "ns3::UdpSocketFactory",
        Inet6SocketAddress(Ipv6Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(borderRouter.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    Ptr<PacketSink> sinkPtr = DynamicCast<PacketSink>(sinkApp.Get(0));
    sinkPtr->TraceConnectWithoutContext("Rx", MakeCallback(&PacketReceivedCallback));

    // Sources on mesh nodes
    Ipv6Address brAddr = ifaces.GetAddress(0, 1);
    ApplicationContainer sourceApps;

    for (uint32_t i = 0; i < nMesh; i++)
    {
        OnOffHelper src("ns3::UdpSocketFactory",
                        Inet6SocketAddress(brAddr, port));
        src.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
        src.SetAttribute("PacketSize", UintegerValue(packetSize));
        src.SetAttribute("OnTime",
            StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        src.SetAttribute("OffTime",
            StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        if (nPackets > 0)
            src.SetAttribute("MaxBytes", UintegerValue(nPackets * packetSize));

        ApplicationContainer app = src.Install(meshNodes.Get(i));
        app.Start(Seconds(1.0 + i * 0.1));   // staggered starts
        app.Stop(Seconds(simTime - 1.0));
        sourceApps.Add(app);
    }

    for (uint32_t i = 0; i < sourceApps.GetN(); i++)
        sourceApps.Get(i)->TraceConnectWithoutContext(
            "Tx", MakeCallback(&PacketSentCallback));

    // ── Flow Monitor ───────────────────────────────────────────
    FlowMonitorHelper fmHelper;
    Ptr<FlowMonitor>  monitor = fmHelper.InstallAll();

    NS_LOG_INFO("Simulation running...");
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ── Collect stats ──────────────────────────────────────────
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t fmSent       = 0;
    uint64_t fmRecv       = 0;
    double   totalDelay   = 0.0;
    double   totalTxBytes = 0.0;
    double   totalRxBytes = 0.0;
    uint32_t flowCount    = 0;

    // ── Per-flow table ─────────────────────────────────────────
    std::cout << "\n";
    Sep('=');
    std::cout << "  Per-Flow Results\n";
    Sep('=');
    std::cout << std::left
              << std::setw(8)  << "FlowID"
              << std::setw(10) << "Sent"
              << std::setw(10) << "Recv"
              << std::setw(8)  << "Lost"
              << std::setw(10) << "PDR(%)"
              << std::setw(14) << "Delay(ms)"
              << std::setw(16) << "Tput(kbps)"
              << "\n";
    Sep();

    for (auto& kv : stats)
    {
        fmSent       += kv.second.txPackets;
        fmRecv       += kv.second.rxPackets;
        totalTxBytes += kv.second.txBytes;
        totalRxBytes += kv.second.rxBytes;

        double pdr   = (kv.second.txPackets > 0)
                       ? 100.0 * kv.second.rxPackets / kv.second.txPackets : 0.0;
        double delay = (kv.second.rxPackets > 0)
                       ? kv.second.delaySum.GetMilliSeconds() / kv.second.rxPackets : 0.0;
        double tput  = (kv.second.rxBytes * 8.0) / (simTime * 1000.0);

        totalDelay += delay;
        flowCount++;

        std::cout << std::left
                  << std::setw(8)  << kv.first
                  << std::setw(10) << kv.second.txPackets
                  << std::setw(10) << kv.second.rxPackets
                  << std::setw(8)  << (kv.second.txPackets - kv.second.rxPackets)
                  << std::setw(10) << std::fixed << std::setprecision(2) << pdr
                  << std::setw(14) << std::fixed << std::setprecision(3) << delay
                  << std::setw(16) << std::fixed << std::setprecision(3) << tput
                  << "\n";
    }

    // ── Aggregate metrics ──────────────────────────────────────
    double pdrCB  = (g_totalSent > 0)
                    ? 100.0 * g_totalReceived / g_totalSent : 0.0;
    double pdrFM  = (fmSent > 0)
                    ? 100.0 * fmRecv / fmSent : 0.0;
    double avgDelay          = (flowCount > 0) ? totalDelay / flowCount : 0.0;
    double netThroughputKbps = (totalRxBytes * 8.0) / (simTime * 1000.0);
    double perNodeTputKbps   = (nMesh > 0) ? netThroughputKbps / nMesh : 0.0;

    // ── Auto-classify ──────────────────────────────────────────
    std::string qualityReason, thresholdReport;
    std::string qualityLabel = ClassifyNetwork(
        pdrFM, avgDelay, netThroughputKbps, nMesh,
        qualityReason, thresholdReport);

    // ── Overall stats block ────────────────────────────────────
    Sep('=');
    std::cout << "\n  OVERALL RESULTS\n";
    Sep();
    std::cout << "  Total Nodes (BR + mesh)  : " << totalNodes
              << "  (" << nMesh << " mesh sources)\n";
    std::cout << "  Simulation Time          : " << simTime      << " s\n";
    std::cout << "  Packet Size              : " << packetSize   << " bytes\n";
    std::cout << "  Packet Interval          : " << packetInterval << " s\n";
    if (nodeSpacing > 0.0)
        std::cout << "  Node Spacing             : " << nodeSpacing << " m\n";
    else
        std::cout << "  Area                     : "
                  << areaSize << "x" << areaSize << " m\n";
    Sep();
    std::cout << "  [Callback]  Sent         : " << g_totalSent     << "\n";
    std::cout << "  [Callback]  Received     : " << g_totalReceived  << "\n";
    std::cout << "  [Callback]  Lost         : "
              << g_totalSent - g_totalReceived << "\n";
    std::cout << "  PDR (Callback)           : " << std::fixed
              << std::setprecision(2) << pdrCB << " %\n";
    Sep();
    std::cout << "  [FlowMon]   Sent         : " << fmSent           << "\n";
    std::cout << "  [FlowMon]   Received     : " << fmRecv           << "\n";
    std::cout << "  [FlowMon]   Lost         : " << fmSent - fmRecv  << "\n";
    std::cout << "  PDR (FlowMonitor)        : " << std::fixed
              << std::setprecision(2) << pdrFM    << " %\n";
    std::cout << "  Packet Loss Rate         : " << std::fixed
              << std::setprecision(2) << (100.0 - pdrFM) << " %\n";
    std::cout << "  Avg Network Delay        : " << std::fixed
              << std::setprecision(3) << avgDelay << " ms\n";
    Sep();
    std::cout << "  Network Throughput       : " << std::fixed
              << std::setprecision(3) << netThroughputKbps << " kbps\n";
    std::cout << "  Per-Node Throughput      : " << std::fixed
              << std::setprecision(3) << perNodeTputKbps   << " kbps\n";
    std::cout << "  Total TX bytes           : " << (uint64_t)totalTxBytes << "\n";
    std::cout << "  Total RX bytes           : " << (uint64_t)totalRxBytes << "\n";
    Sep('=');

    // ── Auto-Calculated Quality & Threshold Report ─────────────
    std::cout << "\n";
    Sep('*');
    std::cout << "  NETWORK QUALITY ASSESSMENT  (Auto-Calculated)\n";
    Sep('*');
    std::cout << "\n";
    std::cout << "  Reference Tiers (IEEE 802.15.4 / Thread Mesh):\n";
    std::cout << "  +-----------+------------------+-------------------+\n";
    std::cout << "  | Label     | PDR              | Avg Delay         |\n";
    std::cout << "  +-----------+------------------+-------------------+\n";
    std::cout << "  | EXCELLENT | >= 95 %          | < 100 ms          |\n";
    std::cout << "  | GOOD      | >= 85 %          | < 500 ms          |\n";
    std::cout << "  | DEGRADED  | >= 60 %          | (any)             |\n";
    std::cout << "  | POOR      | <  60 %          | (any)             |\n";
    std::cout << "  +-----------+------------------+-------------------+\n";
    std::cout << "\n";
    std::cout << "  Measured Values:\n";
    std::cout << "    PDR (FlowMonitor)   = " << std::fixed
              << std::setprecision(2) << pdrFM    << " %\n";
    std::cout << "    Avg Network Delay   = " << std::fixed
              << std::setprecision(2) << avgDelay << " ms\n";
    std::cout << "    Net Throughput      = " << std::fixed
              << std::setprecision(3) << netThroughputKbps << " kbps\n";
    std::cout << "\n";
    std::cout << "  >> Quality Label    : [ " << qualityLabel << " ]\n";
    std::cout << "  >> Assessment       : " << qualityReason << "\n";
    std::cout << "\n";
    std::cout << "  >> Threshold Report :\n";
    std::cout << thresholdReport << "\n";
    std::cout << "\n";
    Sep('*');

    // ── CSV append (one row per run) ───────────────────────────
    {
        bool writeHeader = false;
        {
            std::ifstream chk(outputFile);
            writeHeader = !chk.good() ||
                chk.peek() == std::ifstream::traits_type::eof();
        }

        std::ofstream csv(outputFile, std::ios::app);

        if (writeHeader)
        {
            csv << "Timestamp,"
                << "TotalNodes,MeshNodes,SimTime_s,"
                << "PacketSize_bytes,PacketInterval_s,"
                << "NodeSpacing_m,AreaSize_m,NPackets_per_node,"
                << "Sent_CB,Recv_CB,Lost_CB,PDR_CB_pct,"
                << "Sent_FM,Recv_FM,Lost_FM,PDR_FM_pct,"
                << "PacketLoss_pct,AvgDelay_ms,"
                << "NetThroughput_kbps,PerNodeTput_kbps,"
                << "TxBytes,RxBytes,"
                << "QualityLabel\n";
        }

        std::time_t now = std::time(nullptr);
        char timeBuf[20];
        std::strftime(timeBuf, sizeof(timeBuf),
                      "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        csv << timeBuf                                                   << ","
            << totalNodes                                                << ","
            << nMesh                                                     << ","
            << simTime                                                   << ","
            << packetSize                                                << ","
            << packetInterval                                            << ","
            << nodeSpacing                                               << ","
            << areaSize                                                  << ","
            << nPackets                                                  << ","
            << g_totalSent                                               << ","
            << g_totalReceived                                           << ","
            << (g_totalSent - g_totalReceived)                           << ","
            << std::fixed << std::setprecision(4) << pdrCB              << ","
            << fmSent                                                    << ","
            << fmRecv                                                    << ","
            << (fmSent - fmRecv)                                         << ","
            << std::fixed << std::setprecision(4) << pdrFM              << ","
            << std::fixed << std::setprecision(4) << (100.0 - pdrFM)    << ","
            << std::fixed << std::setprecision(4) << avgDelay           << ","
            << std::fixed << std::setprecision(4) << netThroughputKbps  << ","
            << std::fixed << std::setprecision(4) << perNodeTputKbps    << ","
            << (uint64_t)totalTxBytes                                    << ","
            << (uint64_t)totalRxBytes                                    << ","
            << qualityLabel                                              << "\n";

        csv.close();
        NS_LOG_INFO("CSV row appended: " << outputFile);
    }

    monitor->SerializeToXmlFile("thread_pdr_flowmonitor.xml", true, true);
    Simulator::Destroy();
    NS_LOG_INFO("Done.");
    return 0;
}