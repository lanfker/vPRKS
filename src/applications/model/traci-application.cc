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
#include "ns3/wifi-module.h"
#include <sstream>
#include "ns3/settings.h"

#ifndef YANS_WIFI
#define YANS_WIFI
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
    if (HostRunning () ==  false )
    {
      StopApplication ();
      return;
    }
    if (Simulator::Now () >= m_stopTime)
    {
      return;
    }
    else
    {
      // ask for my current edge
      std::string new_edge;

      traciClient ->CommandGetVariableString (CMD_GET_VEHICLE_VARIABLE, VAR_ROAD_ID, m_name, new_edge);
      //std::cout<<Simulator::Now ()<<" name: " << m_name <<" new edge: "<< new_edge<< std::endl;
      // if I changed edge then I ask for new speed limits
      if (new_edge.empty())
      {
        // if new_edge is empty, then there is a problem. The vehicle has been removed from
        //std::cout <<Simulator::Now () << " "<< m_name << " can't access it cunning edge. It has probably been removed in SUMO. Let stop it." << std::endl;
        m_stopTime == Simulator::Now();
        return;
      }
      if (new_edge != m_edge)
      {
        m_edge = new_edge;
        std::string lane;
        traciClient ->CommandGetVariableString (CMD_GET_VEHICLE_VARIABLE, VAR_LANE_ID, m_name, lane);
        //std::cout<<Simulator::Now ()<<" name: " << m_name <<" lane: "<< lane<< std::endl;

        traciClient ->CommandGetVariableDouble(CMD_GET_LANE_VARIABLE, VAR_MAXSPEED, lane, m_maxSpeed);
        NS_LOG_DEBUG(m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed " << m_maxSpeed);

        //std::cout<<m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed" << m_maxSpeed << std::endl;
      }
      else
      {
        Position2D pos;
        traciClient -> commandGetVariablePosition2D (CMD_GET_VEHICLE_VARIABLE, VAR_POSITION, m_name, pos,  0);
        //std::cout<<" position for "<<m_name<<" is ("<<pos.x<<", "<<pos.y<<")"<<std::endl;
      }
    }
    m_actionEvent = Simulator::Schedule (Seconds(1), &TraciApplication::StateInfoFetch, this);
  }


  void TraciApplication::StartApplication (void)
  {
    traciClient = Names::Find<TraciClient>("TraciClient");
    Ptr<Object> object = GetNode ();
    mobilityModel = object->GetObject<ConstantVelocityMobilityModel> ();
    m_name = Names::FindName (GetNode ());
    //Ptr<AdhocWifiMac> mac = GetNode ()->GetDevice (DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetMac ()->GetObject<AdhocWifiMac> ();
    //mac->PrintAddress ();

    traciClient->commandGetVariableStringList(CMD_GET_VEHICLE_VARIABLE, VAR_EDGES, m_name, m_route);
    m_actionEvent = Simulator::Schedule (Seconds(m_random.GetValue (0, 5)), &TraciApplication::StateInfoFetch, this);
    m_nextEventId = Simulator::Schedule (MilliSeconds (m_random.GetValue (0, PAKCET_GENERATION_INTERVAL)), &TraciApplication::GenerateTraffic, this);
  }
  void TraciApplication::StopApplication (void)
  {
    m_stopTime = Seconds (0);
  }

  void TraciApplication::GenerateTraffic ()
  {
    
    if (HostRunning () ==  false )
    {
      StopApplication ();
      return;
    }
    // need to stop application, terminate simulation
    if ( m_stopTime <= Simulator::Now ())
    {
      return;
    }

    if ( GetNode ()->GetDevice (DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetMac () == NULL)
    {
      return;
    }
    Ptr<AdhocWifiMac> mac = GetNode ()->GetDevice (DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetMac ()->GetObject<AdhocWifiMac> ();
   
    Mac48Address addr1 = Mac48Address::GetBroadcast ();
    
    //--------------------Get position and angle ---------------------------------
    traciClient ->commandGetVariablePosition2D (CMD_GET_VEHICLE_VARIABLE, VAR_POSITION, m_name, m_position);
    //std::cout<<" positio: "<< m_position.x <<" "<< m_position.y << std::endl;
    traciClient->CommandGetVariableDouble (CMD_GET_VEHICLE_VARIABLE, VAR_ANGLE, m_name, m_angle);
    mac->SetAngle (m_angle );
    mac->SetPosition (m_position.x, m_position.y);
    if ( m_edge.empty () == false)
    {
      mac->SetEdge (m_edge);
    }


    uint8_t * payload = new uint8_t[DEFAULT_PACKET_LENGTH];
    PayloadBuffer buff = PayloadBuffer (payload);
    double txPower = 0;
    double rxPower = 0;
    buff.WriteDouble (txPower); // will fill in exact value at PHY layer before transmission
    buff.WriteDouble (rxPower); // will fill in axact value at PHY layer after reception
    buff.WriteDouble (m_angle);
    buff.WriteDouble (m_position.x);
    buff.WriteDouble (m_position.y);
    //std::cout<<"sending: "<<m_angle<<" "<<m_position.x <<" "<< m_position.y << std::endl;
    Ptr<Packet> pkt = Create<Packet> (payload, DEFAULT_PACKET_LENGTH);

    mac->Enqueue (pkt, addr1);
    delete [] payload;
    m_nextEventId = Simulator::Schedule (MilliSeconds (PAKCET_GENERATION_INTERVAL), &TraciApplication::GenerateTraffic, this);
  }

  bool TraciApplication::HostRunning ()
  {
    if (GetNode () == NULL )
    {
      return false;
    }
    else
    {
      return true;
    }
  }

}
