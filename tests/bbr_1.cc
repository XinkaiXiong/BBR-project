//           
// Network topology
//
//       n0 ------------ (n1/router) -------------- n2
//            10.1.1.x                192.168.1.x
//       10.1.1.1    10.1.1.2   192.16.1.1     192.168.1.2
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
//#include "ns3/csma-module.h"
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
RttTracer (Ptr<OutputStreamWrapper> stream, Time oldRTTS, Time newRTTS)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newRTTS << std::endl;
}

/*
void
CongStateTracer (Ptr<OutputStreamWrapper> stream,  TcpSocketState::TcpCongState_t oldState, TcpSocketState::TcpCongState_t newState)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << ", " << newState << std::endl;
}
*/

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
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps"<<std::endl;
            //localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
			// updata gnuplot data
            //DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
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
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon);
	}

// Constants.
#define ENABLE_PCAP      true     // Set to "true" to enable pcap
#define ENABLE_TRACE     false     // Set to "true" to enable trace
#define flow_monitor     true
#define BIG_QUEUE        2000      // Packets
#define QUEUE_SIZE       1         // Packets
#define START_TIME       0.0       // Seconds
#define STOP_TIME        30.0       // Seconds
#define S_TO_R_BW        "100Mbps" // Server to router
#define S_TO_R_DELAY     "10ms"
#define R_TO_C_BW        "1Mbps"  // Router to client (bttlneck)
#define R_TO_C_DELAY     "1ms"
#define PACKET_SIZE      1000      // Bytes.

// Uncomment one of the below.
#define TCP_PROTOCOL     "ns3::TcpBbr"
//#define TCP_PROTOCOL     "ns3::TcpNewReno"

// For logging. 

NS_LOG_COMPONENT_DEFINE ("main");
/////////////////////////////////////////////////
int main (int argc, char *argv[]) {
  //GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  std::string queueDiscType = "RedQueueDisc";
  //uint32_t queueDiscSize = 1000;

  /////////////////////////////////////////
  // Turn on logging for this script.
  // Note: for BBR', other components that may be
  // of interest include "TcpBbr" and "BbrState".
  LogComponentEnable("main", LOG_LEVEL_INFO);

  /////////////////////////////////////////
  // Setup environment
  //Config::SetDefault("ns3::TcpL4Protocol::SocketType",
  //                   StringValue(TCP_PROTOCOL));

  // Report parameters.
  NS_LOG_INFO("TCP protocol: " << TCP_PROTOCOL);
  NS_LOG_INFO("Server to Router Bwdth: " << S_TO_R_BW);
  NS_LOG_INFO("Server to Router Delay: " << S_TO_R_DELAY);
  NS_LOG_INFO("Router to Client Bwdth: " << R_TO_C_BW);
  NS_LOG_INFO("Router to Client Delay: " << R_TO_C_DELAY);
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
  NodeContainer nodes;  // 0=source, 1=router, 2=sink
  nodes.Create(3);

  /////////////////////////////////////////
  // Create channels.
  NS_LOG_INFO("Creating channels.");
  NodeContainer n0_to_r = NodeContainer(nodes.Get(0), nodes.Get(1));
  NodeContainer r_to_n1 = NodeContainer(nodes.Get(1), nodes.Get(2));

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
  

  // Router to Client.
  p2p.SetDeviceAttribute("DataRate", StringValue (R_TO_C_BW));
  p2p.SetChannelAttribute("Delay", StringValue (R_TO_C_DELAY));
  p2p.SetDeviceAttribute ("Mtu", UintegerValue(mtu));
  NS_LOG_INFO("Router queue size: "<< QUEUE_SIZE);
  p2p.SetQueue("ns3::DropTailQueue",
               "Mode", StringValue ("QUEUE_MODE_PACKETS"),
               "MaxPackets", UintegerValue(QUEUE_SIZE));
  NetDeviceContainer devices2 = p2p.Install(r_to_n1);

  /////////////////////////////////////////
  // Install Internet stack.
  NS_LOG_INFO("Installing Internet stack.");
  InternetStackHelper internet;
  internet.Install(nodes);

  /////////////////////////////////////////
  // Buffer
  TrafficControlHelper tch;
  tch.SetRootQueueDisc ("ns3::RedQueueDisc");
  Config::SetDefault("ns3::RedQueueDisc::QueueLimit", UintegerValue(100));
  Config::SetDefault("ns3::RedQueueDisc::MinTh", DoubleValue(90));
  Config::SetDefault("ns3::RedQueueDisc::MaxTh", DoubleValue(110));
  QueueDiscContainer qdiscs = tch.Install (devices2.Get(0));

  /////////////////////////////////////////
  // Add IP addresses.
  NS_LOG_INFO("Assigning IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0i1 = ipv4.Assign(devices1);

  ipv4.SetBase("191.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i2 = ipv4.Assign(devices2);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  /////////////////////////////////////////
  // Create apps.
  NS_LOG_INFO("Creating applications.");
  NS_LOG_INFO("  Bulk send.");

  // Well-known port for server.
  uint16_t port = 911;
  
  Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType",
                     StringValue(TCP_PROTOCOL));
  // Source (at node 0).
  BulkSendHelper source("ns3::TcpSocketFactory",
                        InetSocketAddress(i1i2.GetAddress(1), port));
  // Set the amount of data to send in bytes (0 for unlimited).
  source.SetAttribute("MaxBytes", UintegerValue(0));
  source.SetAttribute("SendSize", UintegerValue(PACKET_SIZE));
  ApplicationContainer apps = source.Install(nodes.Get(0));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));

  // Sink (at node 2).
  PacketSinkHelper sink("ns3::TcpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), port));
  apps = sink.Install(nodes.Get(2));
  apps.Start(Seconds(START_TIME));
  apps.Stop(Seconds(STOP_TIME));
  Ptr<PacketSink> p_sink = DynamicCast<PacketSink> (apps.Get(0)); // 4 stats

  /////////////////////////////////////////
  // Setup tracing (as appropriate).
  if (ENABLE_TRACE) {
    NS_LOG_INFO("Enabling trace files.");
    AsciiTraceHelper ath;
    p2p.EnableAsciiAll(ath.CreateFileStream("trace.tr"));
  }  
  if (ENABLE_PCAP) {
    NS_LOG_INFO("Enabling pcap files.");
    p2p.EnablePcapAll("shark", true);
  }

  AsciiTraceHelper ath;
  Ptr<Queue<Packet> > queue = StaticCast<PointToPointNetDevice> (devices2.Get (0))->GetQueue ();
  Ptr<OutputStreamWrapper> streamPacketsInQueue = ath.CreateFileStream (queueDiscType + "-PacketsInDeviceQueue.csv");
  queue->TraceConnectWithoutContext ("PacketsInQueue",MakeBoundCallback (&PacketsInQueueTrace, streamPacketsInQueue));

  Ptr<QueueDisc> q = qdiscs.Get(0);
  Ptr<OutputStreamWrapper> streamPacketsInQueueDisc = ath.CreateFileStream (queueDiscType + "-PacketsInQueueDisc.csv");
  q->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&TcPacketsInQueueTrace, streamPacketsInQueueDisc));

  // Trace changes to the RTT
  Ptr<OutputStreamWrapper> Rtt_n0 = ath.CreateFileStream ("Rtt_n0.csv");
  Simulator::Schedule(Seconds(0.001), Config::ConnectWithoutContext, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeBoundCallback (&RttTracer, Rtt_n0));

  /*
  //Trace congestion machine state
  Ptr<OutputStreamWrapper> CongState_n0 = ath.CreateFileStream ("CongState_n0.txt");
  Simulator::Schedule(Seconds(0.001), Config::ConnectWithoutContext, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongState", MakeBoundCallback (&CongStateTracer, CongState_n0));  
  */
  
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
  NS_LOG_INFO("---------------- Stop ------------------------");

  /////////////////////////////////////////
  // Ouput stats.
  NS_LOG_INFO("Total bytes received: " << p_sink->GetTotalRx());
  double tput = p_sink->GetTotalRx() / (STOP_TIME - START_TIME);
  tput *= 8;          // Convert to bits.
  tput /= 1000000.0;  // Convert to Mb/s
  NS_LOG_INFO("Throughput: " << tput << " Mb/s");
  NS_LOG_INFO("Done.");

  // Done.
  Simulator::Destroy();
  return 0;
}
