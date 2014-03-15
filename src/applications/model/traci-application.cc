#include "traci-application.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/random-variable.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/mac48-address.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/event-id.h"
#include <traci-server/TraCIConstants.h>
#include "ns3/mobility-module.h"
#include "ns3/traci-client-module.h"
#include "ns3/physim-wifi-module.h"
#include "ns3/wifi-module.h"

#ifndef YANS_WIFI
//#define YANS_WIFI
#endif


NS_LOG_COMPONENT_DEFINE ("TraciApplication");
namespace ns3
{
  NS_OBJECT_ENSURE_REGISTERED(TraciApplication);

  TypeId TraciApplication::GetTypeId()
  {
    static TypeId tid = TypeId ("ns3::TraciApplication")
      .SetParent<Application>()
      .AddConstructor<TraciApplication>()
      .AddAttribute ("ApplicationStopTime", "The time at which the simulation stops", TimeValue (Seconds(0)),
          MakeTimeAccessor (&TraciApplication::m_stopTime), MakeTimeChecker ())
      ;
    return tid;
  }

  TraciApplication::TraciApplication ()
  {
    //std::cout<<" creating traci-application" << std::endl;
  }

  TraciApplication::~TraciApplication()
  {
  }
  void TraciApplication::StateInfoFetch ()
  {
    //std::cout<<"stop time: "<< m_stopTime<< std::endl;
    //std::cout<<" Fetching state information"<< std::endl;
    if (Simulator::Now () >= m_stopTime)
    {
      return;
    }
    else
    {
      // ask for my current edge
      std::string new_edge;
      traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_ROAD_ID, m_name, new_edge);
      std::cout<<Simulator::Now ()<<" name: " << m_name <<" new edge: "<< new_edge<< std::endl;
      // if I changed edge then I ask for new speed limits
      if (new_edge.empty())
      {
        // if new_edge is empty, then there is a problem. The vehicle has been removed from
        //std::cout <<Simulator::Now () << " "<< m_name << " can't access it cunning edge. It has probably been removed in SUMO. Let stop it." << std::endl;
        //m_actionEvent = Simulator::Schedule (Seconds(1), &TraciApplication::StateInfoFetch, this);
        m_stopTime == Simulator::Now();
        return;
      }
      if (new_edge != m_edge)
      {
        m_edge = new_edge;
        std::string lane;
        traciClient ->getString(CMD_GET_VEHICLE_VARIABLE, VAR_LANE_ID, m_name, lane);
        m_maxSpeed = traciClient ->getFloat(CMD_GET_LANE_VARIABLE, VAR_MAXSPEED, lane);
        NS_LOG_DEBUG(m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed" << m_maxSpeed);
        //std::cout<<m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed" << m_maxSpeed << std::endl;
      }
    }
    m_actionEvent = Simulator::Schedule (Seconds(1), &TraciApplication::StateInfoFetch, this);
  }

  void TraciApplication::GetEdgeInfo ()
  {
    traciClient->getString (CMD_GET_VEHICLE_VARIABLE, VAR_ROAD_ID, m_name, m_edge);

    std::string lane;
    traciClient->getString (CMD_GET_VEHICLE_VARIABLE, VAR_LANE_ID, m_name, lane);
    m_maxSpeed = traciClient->getFloat (CMD_GET_LANE_VARIABLE, VAR_MAXSPEED, lane);
  }

  void TraciApplication::StartApplication (void)
  {
    //std::cout<<" starting traci-application" << std::endl;
    traciClient = Names::Find<TraciClient>("TraciClient");
    Ptr<Object> object = GetNode ();
    mobilityModel = object->GetObject<ConstantVelocityMobilityModel> ();
    m_name = Names::FindName (GetNode ());
    traciClient->getStringList (CMD_GET_VEHICLE_VARIABLE, VAR_EDGES, m_name, m_route);
    m_actionEvent = Simulator::Schedule (Seconds(m_random.GetValue (0, 5)), &TraciApplication::StateInfoFetch, this);
    //std::cout<<" node name: "<< m_name << std::endl;
    m_nextEventId = Simulator::Schedule (MilliSeconds (PAKCET_GENERATION_INTERVAL), &TraciApplication::GenerateTraffic, this);
  }
  void TraciApplication::StopApplication (void)
  {
    m_stopTime = Seconds (0);
  }

  void TraciApplication::GenerateTraffic ()
  {
    // need to stop application, terminate simulation
    if ( m_stopTime <= Simulator::Now ())
    {
      return;
    }

    if (GetNode () == NULL )
    {
      return;
    }
    Ptr<AdhocWifiMac> mac = GetNode ()->GetDevice (DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetMac ()->GetObject<AdhocWifiMac> ();
    
    /*
#if !defined (YANS_WIFI)
    Ptr<PhySimWifiPhy> phy = GetNode ()->GetDevice(DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetPhy ()->GetObject<PhySimWifiPhy> ();
#else
    Ptr<YansWifiPhy> phy = GetNode ()->GetDevice(DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetPhy ()->GetObject<YansWifiPhy> ();
#endif
    */
    Mac48Address addr1 = Mac48Address::GetBroadcast ();
    Ptr<Packet> pkt = Create<Packet> (DEFAULT_PACKET_LENGTH);
    mac->Enqueue (pkt, addr1);
    m_nextEventId = Simulator::Schedule (MilliSeconds (PAKCET_GENERATION_INTERVAL), &TraciApplication::GenerateTraffic, this);
  }
}
