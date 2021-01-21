//           
// Network topology
//                   10.1.1.x
//       n0 ------------+ 
//     10.1.1.1         |
//           10.1.1.2   |   10.3.1.1        10.3.1.2
//                 (n2/router) -------------- n3
//           10.2.1.2   |         10.3.1.x      
//                      |
//       n1 ------------+
//     10.2.1.1      10.2.1.x
// 
// - Flow from n0 to n2 using BulkSendApplication.
//
// - Tracing of queues and packet receptions to file "*.tr" and
//   "*.pcap" when tracing is turned on.
//

// System includes.
#include <string>
#include <fstream>

// NS3 includes.
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace std;

void
PacketsInQueueTrace (Ptr<OutputStreamWrapper> stream, uint32_t oldVal, uint32_t newVal)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newVal << std::endl;
}

void
TcPacketsInQueueTrace (Ptr<OutputStreamWrapper> stream, uint32_t oldValue, uint32_t newValue)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newValue << std::endl;
}

void 
CwndTracer1 (Ptr<OutputStreamWrapper> stream, uint32_t oldVal, uint32_t newVal)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newVal << std::endl;
}

void 
CwndTracer2 (Ptr<OutputStreamWrapper> stream, uint32_t oldVal, uint32_t newVal)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newVal << std::endl;
}

void
RttTracer1 (Ptr<OutputStreamWrapper> stream, Time oldRTTS, Time newRTTS)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newRTTS << std::endl;
}

void
RttTracer2 (Ptr<OutputStreamWrapper> stream, Time oldRTTS, Time newRTTS)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newRTTS << std::endl;
}

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon)
	{
        //double localThrou=0;
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
			std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			//std::cout<<"Dropped Packets	: "<< stats->second.packetsDropped.size()<<" Packets"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
			if(fiveTuple.sourceAddress == "10.1.1.1")
			{
				std::ofstream outFile;
				outFile.open("throughput_1.csv", ios::out | ios::app); 
				outFile << Simulator::Now().GetSeconds() << ',' << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024 << endl;				
				outFile.close();
			}
			else if(fiveTuple.sourceAddress == "10.2.1.1")
			{
				std::ofstream outFile;
				outFile.open("throughput_2.csv", ios::out | ios::app);
				outFile << Simulator::Now().GetSeconds() << ',' << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024 << endl;				
				outFile.close();
			}
		}
			Simulator::Schedule(Seconds(0.001),&ThroughputMonitor, fmhelper, flowMon);
   //if(flowToXml)
      //{
	//flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      //}

	}

// Constants.
//#define ENABLE_PCAP      true     // Set to "true" to enable pcap
//#define ENABLE_TRACE     false     // Set to "true" to enable trace
//#define flow_monitor     true
#define BIG_QUEUE        2000      // Packets
#define QUEUE_SIZE       1         // Packets
#define START_TIME       0.0       // Seconds
#define STOP_TIME        180.0       // Seconds
//#define S_TO_R_BW        "100Mbps" // Server to router
//#define S_TO_R_DELAY     "10ms"
//#define R_TO_C_BW        "1Mbps"  // Router to client (bttlneck)
//#define R_TO_C_DELAY     "1ms"
#define PACKET_SIZE      1000      // Bytes.

// Uncomment one of the below.
//#define TCP_PROTOCOL_n0     "ns3::TcpBbr"
//#define TCP_PROTOCOL_n1     "ns3::TcpNewReno"

// For logging. 

NS_LOG_COMPONENT_DEFINE ("main");
/////////////////////////////////////////////////
int main (int argc, char *argv[]) {
  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  std::string queueDiscType = "RedQueueDisc";
  bool PCAP = true;
  bool TRACE = false;
  bool flow_monitor = true;
  std::string S_TO_R_BW = "500kbps";
  std::string S_TO_R_DELAY = "10ms";
  std::string R_TO_C_BW = "1Mbps";
  std::string R_TO_C_DELAY = "1ms";
  std::string TCP_PROTOCOL_n0 = "ns3::TcpBbr";
  std::string TCP_PROTOCOL_n1 = "ns3::TcpBbr";
  uint32_t q_disc_size = 10; 
  /////////////////////////////////////////
  // Turn on logging for this script.
  // Note: for BBR', other components that may be
  // of interest include "TcpBbr" and "BbrState".
  LogComponentEnable("main", LOG_LEVEL_INFO);

  /////////////////////////////////////////
  // Setup environment
  //Config::SetDefault("ns3::TcpL4Protocol::SocketType",
  //                   StringValue(TCP_PROTOCOL));

  CommandLine cmd;
  cmd.AddValue("q_disc_type","Queue disc type", queueDiscType);
  cmd.AddValue("pcap", "Enable or disable PCAP tracing", PCAP);
  cmd.AddValue("trace", "Enable or disable TRACE files", TRACE);
  cmd.AddValue("flow_monitor", "Enable or disable flow monitor", flow_monitor);
  cmd.AddValue("TCP_PROTOCOL_n0", "TCP Protocol of n0", TCP_PROTOCOL_n0);
  cmd.AddValue("TCP_PROTOCOL_n1", "TCP Protocol of n1", TCP_PROTOCOL_n1);
  cmd.AddValue("S2R_BW", "Server to Router Bandwidth", S_TO_R_BW);
  cmd.AddValue("S2R_DELAY", "Server to Router Delay", S_TO_R_DELAY);
  cmd.AddValue("R2C_BW", "Router to Client Bandwidth", R_TO_C_BW);
  cmd.AddValue("R2C_DELAY", "Router to Client Delay", R_TO_C_DELAY);
  cmd.AddValue("q_disc_size", "Queue disc size (packets)", q_disc_size),
  cmd.Parse (argc, argv);

  // Report parameters.
  NS_LOG_INFO("TCP protocol of n0: " << TCP_PROTOCOL_n0);
  NS_LOG_INFO("TCP protocol of n1: " << TCP_PROTOCOL_n1);
  NS_LOG_INFO("Server to Router Bwdth: " << S_TO_R_BW);
  NS_LOG_INFO("Server to Router Delay: " << S_TO_R_DELAY);
  NS_LOG_INFO("Router to Client Bwdth: " << R_TO_C_BW);
  NS_LOG_INFO("Router to Client Delay: " << R_TO_C_DELAY);
  NS_LOG_INFO("Queue disc size: " << q_disc_size);
  NS_LOG_INFO("Packet size (bytes): " << PACKET_SIZE);
  
  // Set segment size (otherwise, ns-3 default is 536).
  Config::SetDefault("ns3::TcpSocket::SegmentSize",
                     UintegerValue(PACKET_SIZE)); 

  // Turn off delayed ack (so, acks every packet).
  // Note, BBR' still works without this.
  Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
   
  /////////////////////////////////////////
  // Create nodes.
  NS_LOG_INFO("Creating nodes.");
  NodeContainer nodes;  // 0=source1, 1=source2, 2=router, 3=sink
  nodes.Create(4);

  /////////////////////////////////////////
  // Create channels.
  NS_LOG_INFO("Creating channels.");
  NodeContainer n0_to_r = NodeContainer(nodes.Get(0), nodes.Get(2));
  NodeContainer n1_to_r = NodeContainer(nodes.Get(1), nodes.Get(2));
  NodeContainer r_to_n3 = NodeContainer(nodes.Get(2), nodes.Get(3));
  NodeContainer senders = NodeContainer(nodes.Get(0), nodes.Get(1));

  /////////////////////////////////////////
  // Create links.
  NS_LOG_INFO("Creating links.");

  // Server to Router.
  int mtu = 1500;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue (S_TO_R_BW));
  p2p.SetChannelAttribute("Delay", StringValue (S_TO_R_DELAY));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  NetDeviceContainer devices1 = p2p.Install(n0_to_r);
  NetDeviceContainer devices2 = p2p.Install(n1_to_r);
  

  // Router to Client.
  p2p.SetDeviceAttribute("DataRate", StringValue (R_TO_C_BW));
  p2p.SetChannelAttribute("Delay", StringValue (R_TO_C_DELAY));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  NS_LOG_INFO("Router queue size: "<< QUEUE_SIZE);
  p2p.SetQueue("ns3::DropTailQueue",
               "Mode", StringValue ("QUEUE_MODE_PACKETS"),
               "MaxPackets", UintegerValue(QUEUE_SIZE));
  NetDeviceContainer devices3 = p2p.Install(r_to_n3);

  /////////////////////////////////////////
  // Install Internet stack.
  NS_LOG_INFO("Installing Internet stack.");
  InternetStackHelper internet;
  internet.Install(nodes);
  
  /////////////////////////////////////////
  // Buffer
  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::RedQueueDisc");
  Config::SetDefault("ns3::RedQueueDisc::QueueLimit", UintegerValue(q_disc_size));
  //Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(90));
  //Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(110));
  QueueDiscContainer qdiscs = tch.Install (devices3.Get(0));

  /////////////////////////////////////////
  // Add IP addresses.
  NS_LOG_INFO("Assigning IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i2 = ipv4.Assign(devices1);
  
  ipv4.SetBase("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(devices2);

  ipv4.SetBase("10.3.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i3 = ipv4.Assign(devices3);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  /////////////////////////////////////////
  // Create apps.
  NS_LOG_INFO("Creating applications.");
  NS_LOG_INFO("  Bulk send1.");

  // Well-known port for server.
  uint16_t port = 911;
  
  // Set the congestion control algorithm at node 0
  Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType",
                     StringValue(TCP_PROTOCOL_n0));
  // Source (at node 0).
  BulkSendHelper source1("ns3::TcpSocketFactory",
                        InetSocketAddress(i2i3.GetAddress(1), port));
  // Set the amount of data to send in bytes (0 for unlimited).
  source1.SetAttribute("MaxBytes", UintegerValue(0));
  source1.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
  ApplicationContainer apps = source1.Install(nodes.Get(0));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));
  
  // Set the congestion control algorithm at node 1
  Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType",
                     StringValue(TCP_PROTOCOL_n1));
  // Source (at node 1).
  NS_LOG_INFO("  Bulk send2.");
  BulkSendHelper source2("ns3::TcpSocketFactory",
                        InetSocketAddress(i2i3.GetAddress(1), port));
  // Set the amount of data to send in bytes (0 for unlimited).
  source2.SetAttribute("MaxBytes", UintegerValue(0));
  source2.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
  apps = source2.Install(nodes.Get(1));
  apps.Start(Seconds(23));
  apps.Stop(Seconds(STOP_TIME));


  // Sink (at node 3).
  NS_LOG_INFO("  Sink.");
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), port));
  apps = sink.Install(nodes.Get(3));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));
  Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (apps.Get(0)); // 4 stats
  
  /////////////////////////////////////////
  // Setup tracing (as appropriate).
  if (TRACE) {
    NS_LOG_INFO("Enabling trace files.");
    AsciiTraceHelper ath;
    p2p.EnableAsciiAll(ath.CreateFileStream("trace.tr"));
  }  
  if (PCAP) {
    NS_LOG_INFO("Enabling pcap files.");
    p2p.EnablePcapAll("shark", true);
  }

  AsciiTraceHelper ath;
  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (devices3.Get (0))->GetQueue ();
  Ptr<OutputStreamWrapper> streamPacketsInQueue = ath.CreateFileStream (queueDiscType + "-PacketsInDeviceQueue.csv");
  queue->TraceConnectWithoutContext ("PacketsInQueue",MakeBoundCallback (&PacketsInQueueTrace, streamPacketsInQueue));

  Ptr<QueueDisc> q = qdiscs.Get(0);
  Ptr<OutputStreamWrapper> streamPacketsInQueueDisc = ath.CreateFileStream (queueDiscType + "-PacketsInQueueDisc.csv");
  q->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&TcPacketsInQueueTrace, streamPacketsInQueueDisc));

  // Trace changes to the congestion window
  Ptr<OutputStreamWrapper> CwndChanges_n0 = ath.CreateFileStream ("CwndChanges_n0.csv");
  Ptr<OutputStreamWrapper> CwndChanges_n1 = ath.CreateFileStream ("CwndChanges_n1.csv");
  Simulator::Schedule(Seconds(0.001), Config::ConnectWithoutContext, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback (&CwndTracer1, CwndChanges_n0));
  Simulator::Schedule(Seconds(23.001), Config::ConnectWithoutContext, "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback (&CwndTracer2, CwndChanges_n1));
  
  // Trace changes to the RTT
  Ptr<OutputStreamWrapper> Rtt_n0 = ath.CreateFileStream ("Rtt_n0.csv");
  Ptr<OutputStreamWrapper> Rtt_n1 = ath.CreateFileStream ("Rtt_n1.csv");
  Simulator::Schedule(Seconds(0.001), Config::ConnectWithoutContext, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeBoundCallback (&RttTracer1, Rtt_n0));
  Simulator::Schedule(Seconds(23.001), Config::ConnectWithoutContext, "/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeBoundCallback (&RttTracer2, Rtt_n1));
  
  // Setup flow monitor
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> allMon = fmHelper.InstallAll();
  ThroughputMonitor(&fmHelper, allMon);
  
  /////////////////////////////////////////
  // Run simulation.
  NS_LOG_INFO("Running simulation.");
  Simulator::Stop(Seconds(STOP_TIME));
  NS_LOG_INFO("Simulation time: [" << 
              START_TIME << "," <<
              STOP_TIME << "]");
  NS_LOG_INFO("---------------- Start -----------------------");
  Simulator::Run();
  if (flow_monitor)
    {
      fmHelper.SerializeToXmlFile ("flowmon.xml", true, true);
    }
  
	/* Calculation of experiment statistics, no need to analyze */
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmHelper.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> flowStats = allMon->GetFlowStats ();
	std::cout << std::endl << "*** Flow monitor statistics ***" << std::endl;
	uint32_t totalPacketsDroppedByQueueDisc = 0;
	uint64_t totalBytesDroppedByQueueDisc = 0;
	uint32_t totalPacketsDroppedByNetDevice = 0;
	uint64_t totalBytesDroppedByNetDevice = 0;
	uint32_t totalPacketsDropped = 0;
	uint64_t totalBytesDropped = 0;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
	{
		Ipv4FlowClassifier::FiveTuple fiveTuple = classifier->FindFlow (stats->first);
		std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
		std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
		std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
		std::cout<<"Duration		: "<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())<<std::endl;
		
		uint32_t packetsDroppedByQueueDisc = 0;
		uint64_t bytesDroppedByQueueDisc = 0;
		uint32_t packetsDropped = 0;
		uint64_t bytesDropped = 0;
		packetsDropped = stats->second.packetsDropped.size ();
		bytesDropped = stats->second.bytesDropped.size ();
		std::cout << "  Packets/Bytes Dropped:   " << packetsDropped << " / " << bytesDropped << std::endl;
		
		if (stats->second.packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE_DISC)
		{
			packetsDroppedByQueueDisc = stats->second.packetsDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
			bytesDroppedByQueueDisc = stats->second.bytesDropped[Ipv4FlowProbe::DROP_QUEUE_DISC];
		}
		std::cout << "  Packets/Bytes Dropped by Queue Disc:   " << packetsDroppedByQueueDisc << " / " << bytesDroppedByQueueDisc << std::endl;
		uint32_t packetsDroppedByNetDevice = 0;
		uint64_t bytesDroppedByNetDevice = 0;
		if (stats->second.packetsDropped.size () > Ipv4FlowProbe::DROP_QUEUE)
		{
			packetsDroppedByNetDevice = stats->second.packetsDropped[Ipv4FlowProbe::DROP_QUEUE];
			bytesDroppedByNetDevice = stats->second.bytesDropped[Ipv4FlowProbe::DROP_QUEUE];
		}
		
		totalPacketsDroppedByQueueDisc += packetsDroppedByQueueDisc;
		totalBytesDroppedByQueueDisc += bytesDroppedByQueueDisc;
		totalPacketsDroppedByNetDevice += packetsDroppedByNetDevice;
		totalBytesDroppedByNetDevice += bytesDroppedByNetDevice;
		totalPacketsDropped += packetsDropped;
		totalBytesDropped += bytesDropped;
		std::cout << "  Packets/Bytes Dropped by NetDevice:   " << packetsDroppedByNetDevice << " / " << bytesDroppedByNetDevice << std::endl;
		std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
		std::cout<<"---------------------------------------------------------------------------"<<std::endl;
	}
	
	/* End of statistics calculation */
  
  NS_LOG_INFO("---------------- Stop ------------------------");
  // Report parameters.
  NS_LOG_INFO("TCP protocol of n0: " << TCP_PROTOCOL_n0);
  NS_LOG_INFO("TCP protocol of n1: " << TCP_PROTOCOL_n1);
  NS_LOG_INFO("Server to Router Bwdth: " << S_TO_R_BW);
  NS_LOG_INFO("Server to Router Delay: " << S_TO_R_DELAY);
  NS_LOG_INFO("Router to Client Bwdth: " << R_TO_C_BW);
  NS_LOG_INFO("Router to Client Delay: " << R_TO_C_DELAY);
  NS_LOG_INFO("Queue disc size: " << q_disc_size);
  NS_LOG_INFO("Packet size (bytes): " << PACKET_SIZE);
  /////////////////////////////////////////
  // Ouput stats.
  NS_LOG_INFO("Total bytes received: " << p_sink->GetTotalRx());
  NS_LOG_INFO("Total Packets/Bytes Dropped:   " << totalPacketsDropped << " / " << totalBytesDropped);
  NS_LOG_INFO("Total Packets/Bytes Dropped by Queue Disc:   " << totalPacketsDroppedByQueueDisc << " / " << totalBytesDroppedByQueueDisc);
  NS_LOG_INFO("Total Packets/Bytes Dropped by NetDevice:   " << totalPacketsDroppedByNetDevice << " / " << totalBytesDroppedByNetDevice);
  double tput = p_sink->GetTotalRx() / (STOP_TIME - START_TIME);
  tput *= 8;          // Convert to bits.
  tput /= 1000000.0;  // Convert to Mb/s
  NS_LOG_INFO("Throughput: " << tput << " Mb/s");
  NS_LOG_INFO("Done.");

  // Done.
  Simulator::Destroy();
  return 0;
}

