
/* This file is build for testing TraCI such that NS-3 may be work with SUMO cooperatively. 
 * 2013-12-02 Mon 10:20 AM
 */

#ifndef TRACI_APPLICATION_H
#define TRACI_APPLICATION_H

#include "ns3/application.h"
#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/event-id.h"
#include <string>
#include "ns3/random-variable.h"
#include "ns3/socket.h"
#include "ns3/mac48-address.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/traci-client.h"
#include "ns3/mobility-module.h"
#include "ns3/physim-wifi-module.h"

namespace ns3{

  const uint32_t DEFAULT_WIFI_DEVICE_INDEX = 0;
  const uint32_t DEFAULT_PACKET_LENGTH = 100;
  const uint64_t PAKCET_GENERATION_INTERVAL = 100;// in milliseconds. This is the default interval for safety message.


  class TraciApplication: public Application
  {
    public:
      static TypeId GetTypeId(void);
      TraciApplication ();
      virtual ~TraciApplication();
      void StateInfoFetch ();
      void GetEdgeInfo ();
      void GenerateTraffic ();
      bool HostRunning ();
    private:
      virtual void StartApplication (void);
      virtual void StopApplication (void);
      Time m_stopTime;
      Ptr<TraciClient> traciClient;
      Ptr<ns3::ConstantVelocityMobilityModel> mobilityModel;
      EventId m_actionEvent;
      UniformVariable m_random;
      std::vector<std::string> m_route;
      std::string m_name;
      std::string m_edge;
      double m_maxSpeed;
      float m_speed;
      EventId m_nextEventId;
      Position2D m_position;
      double m_angle;
  };

}


#endif //TRACI_APPLICATION_H 
