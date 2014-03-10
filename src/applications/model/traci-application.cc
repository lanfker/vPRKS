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
        std::cout <<Simulator::Now () << " "<< m_name << " can't access it cunning edge. It has probably been removed in SUMO. Let stop it." << std::endl;
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
        std::cout<<m_name<<" moving to new edge "<< new_edge<<" lane "<< lane << " max-speed" << m_maxSpeed << std::endl;
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
  }
  void TraciApplication::StopApplication (void)
  {
  }
}
