// Copyright (c) 2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

/**
 * @file cttc-3gpp-channel-example.cc
 * @ingroup examples
 * @brief Channel Example
 *
 * This example describes how to setup a simulation using the 3GPP channel model
 * from TR 38.901. Topology consists by default of 2 UEs and 2 gNbs, and can be
 * configured to be either mobile or static scenario.
 *
 * The output of this example are default NR trace files that can be found in
 * the root ns-3 project folder.
 */

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/buildings-helper.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-pf.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

#include <fstream>




int
main(int argc, char* argv[])
{
    std::string scenario = "UMa"; // scenario
    double frequency = 28e9;      // central frequency
    double bandwidth = 20e6 ;     // bandwidth
    double mobility = true;      // whether to enable mobility
    double simTime = 1;           // in second
    double speed = 1;             // in m/s for walking UT.
    bool logging = true; // whether to enable logging from the simulation, another option is by
                         // exporting the NS_LOG environment variable
    double hBS;          // base station antenna height in meters
    double hUT;          // user antenna height in meters
    double txPower = 40; // txPower

    CommandLine cmd(__FILE__);
    cmd.AddValue("scenario",
                 "The scenario for the simulation. Choose among 'RMa', 'UMa', 'UMi', "
                 "'InH-OfficeMixed', 'InH-OfficeOpen'.",
                 scenario);
    cmd.AddValue("frequency", "The central carrier frequency in Hz.", frequency);
    cmd.AddValue("mobility",
                 "If set to 1 UEs will be mobile, when set to 0 UE will be static. By default, "
                 "they are mobile.",
                 mobility);
    cmd.AddValue("logging", "If set to 0, log components will be disabled.", logging);
    cmd.Parse(argc, argv);

    // enable logging
    if (logging)
    {
        // LogComponentEnable ("ThreeGppSpectrumPropagationLossModel", LOG_LEVEL_ALL);
        //LogComponentEnable("ThreeGppPropagationLossModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ThreeGppChannelModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("ChannelConditionModel", LOG_LEVEL_ALL);
        // LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        // LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
        // LogComponentEnable ("NrRlcUm", LOG_LEVEL_LOGIC);
        // LogComponentEnable ("NrPdcp", LOG_LEVEL_INFO);
        LogComponentEnable ("NrUeMac", LOG_LEVEL_ALL);
        LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
    }

    /*
     * Default values for the simulation. We are progressively removing all
     * the instances of SetDefault, but we need it for legacy code (LTE)
     */
    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    // set mobile device and base station antenna heights in meters, according to the chosen
    // scenario
    if (scenario == "RMa")
    {
        hBS = 35;
        hUT = 1.5;
    }
    else if (scenario == "UMa")
    {
        hBS = 25;
        hUT = 1.5;
    }
    else if (scenario == "UMi-StreetCanyon")
    {
        hBS = 10;
        hUT = 1.5;
    }
    else if (scenario == "InH-OfficeMixed" || scenario == "InH-OfficeOpen")
    {
        hBS = 3;
        hUT = 1;
    }
    else
    {
        NS_ABORT_MSG("Scenario not supported. Choose among 'RMa', 'UMa', 'UMi', "
                     "'InH-OfficeMixed', and 'InH-OfficeOpen'.");
    }

    // create base stations and mobile terminals
    NodeContainer gnbNodes;
    NodeContainer ueNodes;
    gnbNodes.Create(1);
    ueNodes.Create(3);

    // position the base stations
    Ptr<ListPositionAllocator> gnbPositionAlloc = CreateObject<ListPositionAllocator>();
   
    gnbPositionAlloc->Add(Vector(0.0, 85.0, hBS));
    MobilityHelper gnbMobility;
    gnbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobility.SetPositionAllocator(gnbPositionAlloc);
    gnbMobility.Install(gnbNodes);

    // position the mobile terminals and enable the mobility
    MobilityHelper uemobility;
    uemobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    uemobility.Install(ueNodes);

    if (mobility)
    {
        ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(
            Vector(1, 80, hUT)); // (x, y, z) in m
        ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(0, 0, 0)); // move UE1 along the y axis

        ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(
            Vector(2, 80, hUT)); // (x, y, z) in m
        ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(0, 0, 0)); // move UE2 along the x axis
        ueNodes.Get(2)->GetObject<MobilityModel>()->SetPosition(
            Vector(3, 80, hUT)); // (x, y, z) in m
        ueNodes.Get(2)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(0, 0, 0)); // move UE1 along the y axis



    }
    else
    {
        ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(90, 15, hUT));
        ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));

        ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(30, 50.0, hUT));
        ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0, 0, 0));
    }

    /*
     * Create NR simulation helpers
     */
    Ptr<NrPointToPointEpcHelper> nrEpcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(nrEpcHelper);

    /*
     * Spectrum configuration. We create a single operational band and configure the scenario.
     */

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example we have a single band, and that band is
                                    // composed of a single component carrier

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf creates
     * a single BWP per CC and a single BWP in CC.
     *
     * Hence, the configured spectrum is:
     *
     * |---------------Band---------------|
     * |---------------CC-----------------|
     * |---------------BWP----------------|
     */
    CcBwpCreator::SimpleOperationBandConf bandConf(frequency, bandwidth, numCcPerBand);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    // Create the channel helper
    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    // Set and configure the channel to the current band
    channelHelper->ConfigureFactories(
        scenario,
        "Default",
        "ThreeGpp"); // Configure the spectrum channel with the scenario
    channelHelper->AssignChannelsToBands({band});
    allBwps = CcBwpCreator::GetAllBwps({band});

    // Configure ideal beamforming method
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));

    // Configure scheduler
    // 1. 스케줄러 클래스 자체를 PF로 지정 (이게 핵심!)
    nrHelper->SetSchedulerTypeId(ns3::NrMacSchedulerTdmaPF::GetTypeId());

    // 2. 만약 PF 내부의 상세 속성(TimeWindow 등)을 바꾸고 싶다면 그 다음에 사용
    // nrHelper->SetSchedulerAttribute("FairnessIndex", DoubleValue(1.0));

    // Antennas for the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));

    // install nr net devices
    NetDeviceContainer gnbNetDev = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gnbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    NrHelper::GetGnbPhy(gnbNetDev.Get(0), 0)->SetTxPower(txPower);
    

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    auto [remoteHost, remoteHostIpv4Address] =
        nrEpcHelper->SetupRemoteHost("100Gb/s", 2500, Seconds(0.010));

    InternetStackHelper internet;
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = nrEpcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    uint16_t ulPort = 5000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    UdpServerHelper ulPacketSinkHelper(ulPort);
    serverApps.Add(ulPacketSinkHelper.Install(remoteHost)); // RemoteHost가 수신

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        UdpClientHelper ulClient(remoteHostIpv4Address, ulPort);
        ulClient.SetAttribute("Interval", TimeValue(MicroSeconds(500))); // [수정] 세미콜론 오타 해결
        if(u==0)  ulClient.SetAttribute("Interval", TimeValue(MicroSeconds(50)));
        ulClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
        ulClient.SetAttribute("PacketSize", UintegerValue(1400));
        clientApps.Add(ulClient.Install(ueNodes.Get(u))); // UE가 전송
    }

    nrHelper->AttachToClosestGnb(ueNetDev, gnbNetDev);

    serverApps.Start(Seconds(0.4));
    clientApps.Start(Seconds(0.5));
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime - 0.1));

    // --- Flow Monitor Setup ---
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll(); // [수정] 선언 누락 해결

    Simulator::Stop(Seconds(simTime));
   
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    

    for (auto const& [id, stat] : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(id);
        
        // [수정 1] 자잘한 제어 트래픽은 무시하고 UE(7.0.0.x) 트래픽만 필터링
        Ipv4Mask mask("255.255.255.0");
        if (t.sourceAddress.CombineMask(mask) == Ipv4Address("7.0.0.0")) 
        {
            std::cout << "Flow " << id << " [" << t.sourceAddress << " -> " << t.destinationAddress << "]\n";

            // [수정 2] 전송 시간 계산 (시뮬레이션 시작 0.5초 후부터 트래픽 발생 가정)
            double duration = simTime - 0.5; 
            double throughput = stat.rxBytes * 8.0 / duration / 1e6; // Mbps
            
            // [수정 3] 패킷 손실률(PDR) 계산 - Grayhole 탐지의 핵심 지표
            double packetLossRate = (stat.txPackets > 0) ? 
                (double)(stat.txPackets - stat.rxPackets) / stat.txPackets * 100 : 0;

            // [수정 4] 평균 지연 시간(End-to-End Delay) 계산
            double avgDelay = (stat.rxPackets > 0) ? 
                stat.delaySum.GetSeconds() / stat.rxPackets * 1000 : 0; // ms

            std::cout << "  - Throughput:   " << throughput << " Mbps\n";
            std::cout << "  - Packet Loss: " << packetLossRate << " %\n";
            std::cout << "  - Avg Delay:   " << avgDelay << " ms\n";
            std::cout << "  - TX/RX Packets: " << stat.txPackets << " / " << stat.rxPackets << "\n";
            std::cout << "----------------------------------------------------------\n";
        }
    }

    std::ofstream out("v2x_data.csv", std::ios::app);
    if (out.is_open()) {
        for (auto const& [id, stat] : stats) {
            Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(id);
            if (t.destinationAddress == remoteHostIpv4Address) {
            // Flow ID, Source, Throughput, PacketLossRate 등을 저장
                double lossRate = (stat.txPackets > 0) ? 
                (double)(stat.txPackets - stat.rxPackets) / stat.txPackets : 0;
                out << id << "," << t.sourceAddress << "," 
                << stat.rxBytes * 8.0 / (simTime - 0.5) / 1e6 << "," 
                << lossRate << "\n";
            }
        }
        out.close();
    }

    Simulator::Destroy();
    return 0;
}