/*
 * SPDX-License-Identifier: GPL-2.0-only
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
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScratchSimulator");

int
main(int argc, char* argv[])
{
    double simTime = 1;
    double frequency = 28e9;
    double bandwidth = 100e6;
    double hBS = 25.0;
    double hUT = 1.5;
    double txPower = 40;


    


    NodeContainer gnbNodes, ueNodes;
    gnbNodes.Create(2);
    ueNodes.Create(2);
    
    Ptr<ListPositionAllocator> gnbPos = CreateObject<ListPositionAllocator>();
    gnbPos->Add(Vector(0.0, 0.0, hBS));
    gnbPos->Add(Vector(0.0, 80.0, hBS));
    MobilityHelper gnbMobil;
    gnbMobil.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gnbMobil.SetPositionAllocator(gnbPos);
    gnbMobil.Install(gnbNodes);

    MobilityHelper ueMobil;
    ueMobil.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobil.Install(ueNodes);
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(
        Vector(30, 20, hUT));
    ueNodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
        Vector(0, 10, 0));
    ueNodes.Get(1)->GetObject<MobilityModel>()->SetPosition(
        Vector(30, 60, hUT));
    ueNodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
        Vector(0, -10, 0));

    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> beam = CreateObject<IdealBeamformingHelper>();
    nrHelper->SetBeamformingHelper(beam);
    nrHelper->SetEpcHelper(epcHelper);

    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    CcBwpCreator::SimpleOperationBandConf bandConf(frequency, bandwidth, numCcPerBand);
    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);

    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    channelHelper->ConfigureFactories(
        "UMa",
        "Default",
        "ThreeGpp"
    );
    channelHelper->AssignChannelsToBands({band});
    allBwps = CcBwpCreator::GetAllBwps({band});

    beam->SetAttribute("BeamformingMethod",
                                        TypeIdValue(DirectPathBeamforming::GetTypeId()));
    
    nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());
    
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement", PointerValue(CreateObject<IsotropicAntennaModel>()));

    NetDeviceContainer gnbNet = nrHelper->InstallGnbDevice(gnbNodes, allBwps);
    NetDeviceContainer ueNet = nrHelper->InstallUeDevice(ueNodes, allBwps);
    
    int64_t randomStream = 1; //스트림분리 ->독립시행 보장
    randomStream += nrHelper->AssignStreams(gnbNet, randomStream);
    randomStream += nrHelper->AssignStreams(ueNet, randomStream);

    NrHelper::GetGnbPhy(gnbNet.Get(0), 0)->SetTxPower(txPower);
    NrHelper::GetGnbPhy(gnbNet.Get(1), 0)->SetTxPower(txPower);

    auto [remoteHost, remoteHostIpv4Address] =
        epcHelper->SetupRemoteHost("100Gb/s", 2500, Seconds(0.010));

    InternetStackHelper internet;
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIp;
    ueIp = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNet));

    uint16_t dlPort = 1234;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for(uint32_t u = 0; u < ueNodes.GetN(); ++u){
        Ptr<Node> ueNode = ueNodes.Get(u);
        UdpServerHelper dlPacketSinkHelper(dlPort);
        serverApps.Add(dlPacketSinkHelper.Install(ueNodes.Get(u)));
        
        UdpClientHelper dlClient(ueIp.GetAddress(u), dlPort);
        dlClient.SetAttribute("Interval", TimeValue(MicroSeconds(1)));
        dlClient.SetAttribute("MaxPackets", UintegerValue(10));
        dlClient.SetAttribute("PacketSize", UintegerValue(1000));
        clientApps.Add(dlClient.Install(remoteHost));
    }

    nrHelper->AttachToClosestGnb(ueNet, gnbNet);
    //nrHelper->SetGnbHandoverAlgorithmType("ns3::A3RrspHandoverAlgorithm"); 핸드오버...
    serverApps.Start(Seconds(0.4));
    clientApps.Start(Seconds(0.4));
    serverApps.Stop(Seconds(simTime));
    clientApps.Stop(Seconds(simTime-0.2));
    
    nrHelper->EnableTraces();


    Simulator::Stop(Seconds(simTime));

    Simulator::Run();

    Ptr<UdpServer> serverApp = serverApps.Get(0)->GetObject<UdpServer>();
    uint64_t receivedPackets = serverApp->GetReceived();
    Simulator::Destroy();
    std::cout << "수신된 패킷 수: " << receivedPackets << std::endl;
    if (receivedPackets == 10)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        std::cout << "아쉽게됐습니다..\n";
    }
    return 0;
}
