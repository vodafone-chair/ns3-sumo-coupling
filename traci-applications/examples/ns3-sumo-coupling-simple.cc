#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traci-applications-module.h"
#include "ns3/network-module.h"
#include "ns3/traci-module.h"
#include "ns3/wave-module.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/netanim-module.h"
#include <functional>
#include <stdlib.h>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("ns3-sumo-coupling-simple");

int
main (int argc, char *argv[])
{
	/*** 0. Logging Options ***/
	bool verbose = true;

	CommandLine cmd;
	cmd.Parse (argc,argv);
	if (verbose)
	{
		LogComponentEnable("TraciClient", LOG_LEVEL_INFO);
	    LogComponentEnable("TrafficInfoApplication", LOG_LEVEL_INFO);
	}

	/*** 1. Create node pool and counter; large enough to cover all sumo vehicles ***/
	NodeContainer nodePool;
	nodePool.Create (20);
	uint32_t nodeCounter (0);

	/*** 2. Create and setup channel ***/
	std::string phyMode ("OfdmRate6MbpsBW10MHz");
	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.Set("TxPowerStart", DoubleValue(20));
	wifiPhy.Set("TxPowerEnd", DoubleValue(20));
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	Ptr<YansWifiChannel> channel = wifiChannel.Create ();
	wifiPhy.SetChannel (channel);

	/*** 3. Create and setup MAC ***/
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
	Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
	wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));
	NetDeviceContainer netDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, nodePool);

	/*** 4. Add Internet layers stack and routing ***/
	InternetStackHelper stack;
	stack.Install (nodePool);

	/*** 5. Assign IP address to each device ***/
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer serverInterfaces;
	serverInterfaces = address.Assign(netDevices);

	/*** 6. Setup Mobility and position node pool ***/
	MobilityHelper mobility;
	Ptr<UniformDiscPositionAllocator> positionAlloc = CreateObject<UniformDiscPositionAllocator> ();
	positionAlloc->SetX(320.0);
	positionAlloc->SetY(320.0);
	positionAlloc->SetRho(25.0);
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodePool);

    /*** 7. Setup Traci and start SUMO ***/
	Ptr<TraciClient> client = CreateObject<TraciClient> ();
	client->SetAttribute("SumoConfigPath", StringValue("src/traci/examples/circle-simple/circle.sumo.cfg"));
	client->SetAttribute("SumoBinaryPath", StringValue(""));	// use system installation of sumo
	client->SetAttribute("SynchInterval", TimeValue(Seconds(0.1)));
	client->SetAttribute("StartTime", TimeValue(Seconds(0.0)));
	client->SetAttribute("SumoGUI", BooleanValue(false));
	client->SetAttribute("SumoPort", UintegerValue(3400));
	client->SetAttribute("PenetrationRate", DoubleValue(1.0));
	client->SetAttribute("SumoLogFile", BooleanValue(true));
	client->SetAttribute("SumoStepLog", BooleanValue(false));
	client->SetAttribute("SumoSeed", IntegerValue(10));
	client->SetAttribute("SumoAdditionalCmdOptions", StringValue("--verbose true"));
	client->SetAttribute("SumoWaitForSocket", UintegerValue(1000000));  // in micro seconds

	/*** 8. Create and Setup Applications for server nodes and set position ***/
	TrafficInfoServerHelper trafficServer (9); // Port #9
	trafficServer.SetAttribute ("Velocity", UintegerValue (30));
	trafficServer.SetAttribute ("Interval", TimeValue (Seconds (7.0)));
	trafficServer.SetAttribute("Client", (PointerValue)(client));	// pass TraciClient object for accessing sumo in application

    ApplicationContainer serverApps = trafficServer.Install (nodePool.Get(0));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (500.0));

    Ptr<MobilityModel> mobilityServerNode = nodePool.Get(0)->GetObject<MobilityModel>();
    mobilityServerNode->SetPosition(Vector(70.7, 70.7, 3.0));
    nodeCounter++;	// one node consumed from "pool"

	/*** 9. Setup interface and application for dynamic nodes ***/
	TrafficInfoClientHelper trafficClient (9);
	trafficClient.SetAttribute("Client", (PointerValue)client); // pass TraciClient object for accessing sumo in application

    // callback function for node creation
    std::function<Ptr<Node> ()> setupNewWifiNode = [&] () -> Ptr<Node>
    {
    	if (nodeCounter >= nodePool.GetN())
        NS_FATAL_ERROR("Node Pool empty!: " << nodeCounter << " nodes created.");

    	Ptr<Node> includedNode = nodePool.Get(nodeCounter); // don't create and install node at simulation time -> take from "pool"
    	++nodeCounter; // increment counter for next node

    	// Install Application
    	ApplicationContainer clientApps = trafficClient.Install (includedNode);
    	clientApps.Start (Seconds (0.0));
    	clientApps.Stop (Seconds (500));

    	return includedNode;
    };

	// callback function for node shutdown
	std::function<void(Ptr<Node>)> shutdownWifiNode = [] (Ptr<Node> exNode)
	{
		// stop all applications
		Ptr<TrafficInfoClient> infoClient = exNode->GetApplication(0)->GetObject<TrafficInfoClient>();
		if(infoClient)
		    infoClient->StopApplicationNow();

		// set position outside communication range
		Ptr<ConstantPositionMobilityModel> mob = exNode->GetObject<ConstantPositionMobilityModel>();
		mob->SetPosition(Vector(-100.0+(rand()%25),320.0+(rand()%25),250.0));	// rand() for visualization purposes

        // NOTE: further actions could be required for a save shut down!
	};

	// start traci client with given function pointers
	client->SumoSetup(setupNewWifiNode, shutdownWifiNode);

    /*** 10. Setup and Start Simulation + Animation ***/
	AnimationInterface anim("src/traci-applications/examples/ns3-sumo-coupling.xml"); // Mandatory
    Simulator::Stop (Seconds (500.0));

    Simulator::Run ();
    Simulator::Destroy ();

    return 0;
}
