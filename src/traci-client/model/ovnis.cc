/**
 *
 *
 * Copyright (c) 2010-2011 University of Luxembourg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @file ovnis.cpp
 * @date Apr 21, 2010
 *
 * @author Yoann Pigné
 */
#include <iostream>
#include <cmath>
#include <math.h>
#include <time.h>
//
// ----- NS-3 related includes
#include "ns3/physim-wifi-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-helper.h"
#include "ns3/nqos-wifi-mac-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-net-device.h"
#include "ns3/mobility-helper.h"
#include "ns3/names.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/traci-client.h"
#include "ns3/road-map-module.h"
#include "ns3/settings.h"
//
// ----- SUMO related includes
#include <traci-server/TraCIConstants.h>

//
// ----- Ovnis application includes
#include "ovnis.h"

using namespace std;



namespace ns3
{
  NS_LOG_COMPONENT_DEFINE ("Ovnis");
  NS_OBJECT_ENSURE_REGISTERED (Ovnis);

  TypeId
  Ovnis::GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::Ovnis") .SetParent<Object> () .AddConstructor<Ovnis> ()
      .AddAttribute("Application", "Application used in devices",
          StringValue("ns3::Application"), MakeStringAccessor(&Ovnis::m_application), MakeStringChecker())
      .AddAttribute("SumoConfig","The configuration file for running SUMO",
          StringValue(SUMO_CONFIG), MakeStringAccessor(&Ovnis::sumoConfig), MakeStringChecker())
      .AddAttribute("NodeFile","The configuration file for *.nod.xml",
          StringValue("cross.nod.xml"), MakeStringAccessor(&Ovnis::nodeFile), MakeStringChecker())
      .AddAttribute("EdgeFile","The configuration file for *.edg.xml",
          StringValue("cross.edg.xml"), MakeStringAccessor(&Ovnis::edgeFile), MakeStringChecker())
      .AddAttribute("SumoHost", "The host machine on which SUMO will run",
          StringValue(SUMO_HOST), MakeStringAccessor(&Ovnis::sumoHost), MakeStringChecker())
      .AddAttribute("StartTime", "Start time in the simulation scale (in seconds)",
          IntegerValue(0),MakeIntegerAccessor(&Ovnis::startTime), MakeIntegerChecker<int> (0))
      .AddAttribute("StopTime", "Stop time in the simulation scale (in seconds)",
          IntegerValue(0), MakeIntegerAccessor(&Ovnis::stopTime), MakeIntegerChecker<int> (0))
      .AddAttribute("CommunicationRange", "Communication range used to subdivide the simulation space (in meters)",
          DoubleValue(500.0), MakeDoubleAccessor(&Ovnis::communicationRange), MakeDoubleChecker<double> (0.0))
      .AddAttribute("StartSumo", "Does OVNIS have to start SUMO or not?",
          BooleanValue(), MakeBooleanAccessor(&Ovnis::startSumo), MakeBooleanChecker())
      .AddAttribute("SumoPath", "The system path where the SUMO executable is located",
          StringValue(SUMO_PATH), MakeStringAccessor(&Ovnis::sumoPath), MakeStringChecker());

    return tid;
  }

  Ovnis::Ovnis() :
    runningVehicles(), in(), out(), loaded()
  {
    m_slot = 0;
  }

  Ovnis::~Ovnis()
  {
    traciClient->commandClose();
    traciClient->close();
  }

  void
  Ovnis::DoDispose()
  {
    computeStats();
    Object::DoDispose();
  }

  void
  Ovnis::DoStart(void)
  {
    NS_LOG_FUNCTION(Simulator::Now().GetSeconds());
    Names::Add("Ovnis", this);

    //start = mach_absolute_time();

    currentTime = startTime*1000;




    // retrieve SUMO network port number and simulation boundaries from config files.
    XMLSumoConfParser::parseConfiguration(sumoConfig); //, &port, boundaries);

    std::cout<<" port in ovnis: " << XMLSumoConfParser::port << std::endl;

    // Start SUMO ?
    // If our simulation script indicate the local machine has sumo installed, and configuration files are copied to ns3/scratch
    // we can run SUMO locally. By providing a boolean value (true) to \param startSumo.
    if (startSumo)
    {
//      sumoHost = "localhost";
      if (fork() == 0)
      {
        char buff[512];
        ofstream out;
        out.open("scratch/sumo_output.log");
        out << "Output log file from sumo's execution.";
        FILE *fp;
        if ((fp = popen((sumoPath + " -c " + sumoConfig + " 2>&1").c_str(), "r")) == NULL)
        {
          cerr << "Files or processes cannot be created" << endl;
          return;
        }
        while (fgets(buff, sizeof(buff), fp) != NULL)
        {
          out << buff;
          ;
          out.flush();
        }
        // close the pipe
        pclose(fp);
        exit(0);
      }
    }

    //------------------------------------------------------------------------------------------------
    NodeXmlParser::ParseNodeXmlFile (nodeFile);
    EdgeXmlParser::ParseEdgeXmlFile (edgeFile);


    //------------------------------------------------------------------------------------------------



    /*
     if (fork() == 0)
     {
     std::stringstream s_port;
     s_port << port;
     execl(sumoPath.c_str(), sumoPath.c_str(), "-c", sumoConfig.c_str(), (char *) 0);
     printf("Error. Could not launch SUMO. Please check or set the SUMO path (%s)\n.", sumoPath.c_str());
     }
     */

    traciClient = CreateObject<TraciClient> ();
    Names::Add("TraciClient", traciClient);

    //std::cout<<"Connecting to sumo with host: "<<sumoHost << " port: "<< port << std::endl;
    traciClient->connect(sumoHost, XMLSumoConfParser::port);
    //std::cout<<" sumoConfig: "<< sumoConfig <<" port: "<< port << std::endl;

    // submissions : started, stopped
    traciClient->submission(startTime*1000, stopTime*1000);
    //std::cout<<" sumbitted"<< std::endl;

    // First run. Reaches the startTime
    traciClient->simulationStep(startTime*1000, currentTime, in, out);
    //std::cout<<" simulation step!"<< std::endl;

    //application
    m_application_factory.SetTypeId(m_application);
    m_application_factory.Set ("ApplicationStopTime", TimeValue (Seconds(stopTime)));
    //std::cout<<" application: "<< m_application<< std::endl;     // is ns3::TraciApplication

    // Initialize ns-3 devices
    // will not use Ipv4 networks
    initializeNetwork();

    //For vehicles that have just departed, include them in our simulation
    //For vehicles that have just arrived, exclude them from out simulation
    updateInOutVehicles();

    //Simulator::Schedule(Seconds(0), &Ovnis::run, this);

    Object::DoStart ();
  }

  std::string
  Ovnis::outList(std::vector<std::string>& list)
  {
    std::stringstream out;
    for (std::vector<std::string>::iterator i = list.begin(); i != list.end(); ++i)
    {
      if (i != list.begin())
        out << ", ";
      out << (*i);
    }
    out << std::endl;
    return out.str();
  }

  void
  Ovnis::CreateNetworkDevices(NodeContainer node_container)
  {
    NetDeviceContainer devices = wifi.Install(phy, mac, node_container);

    // We do not need Ipv4 in our vPRKS simulation
    /*
    InternetStackHelper stack;
    stack.Install(node_container);
    Ipv4InterfaceContainer wifiInterfaces;
    wifiInterfaces = address.Assign(devices);
    */
  }

  void
  Ovnis::DestroyNetworkDevices()
  {

    for (std::vector<Ptr<Node> >::iterator i = m_toDestroy.begin(); i != m_toDestroy.end(); ++i)
    {

      Ptr<Node> n = (*i);
      /*
       * do not need Ipv4
      Ptr<Ipv4> ipv4 = n->GetObject<Ipv4>();
      for (uint32_t j = 0; j < n->GetNDevices(); ++j)
      {
        int32_t ifIndex = ipv4->GetInterfaceForDevice(n->GetDevice(j));
        ipv4->SetDown(ifIndex);
      }
      */
      for (uint32_t i = 0; i < n->GetNApplications(); ++i)
      {
        n->GetApplication(i)->SetStopTime(Simulator::Now());
        n->GetApplication(i)->Dispose();
      }
      Ptr<NetDevice> d = n->GetDevice(0);
      Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice>(d);
      Ptr<WifiPhy> wp = wd->GetPhy();
#if !defined (YANS_WIFI)
      Ptr<PhySimWifiPhy> ywp = DynamicCast<PhySimWifiPhy>(wp);
#else
      Ptr<YansWifiPhy> ywp = DynamicCast<YansWifiPhy>(wp);
#endif
      ywp->Dispose ();
      channel->Remove(ywp);
      //delete ywp;

      uint32_t number_of_device = n->GetNDevices ();
      for (uint32_t i = 0; i < number_of_device; ++ i)
      {
        //Ptr<AdhocWifiMac> mac = n->GetDevice (DEFAULT_WIFI_DEVICE_INDEX)->GetObject<WifiNetDevice> ()->GetMac ()->GetObject<AdhocWifiMac> ();
        //mac->ClearMacLow ();
        n->GetDevice (i)->Dispose (); // distroy every net device that has been aggregated to the node.

      }
    }
    m_toDestroy.clear ();
  }

  void
  Ovnis::updateInOutVehicles()
  {
    NS_LOG_FUNCTION_NOARGS();

    // ------ remove the eventually removed vehicles while added in the inserted list (especially a the beginning)
    std::vector<std::string>::iterator i = out.begin();
    while (i != out.end())
    {
      vector<string>::iterator it;
      it = std::find(in.begin(), in.end(), (*i));
      //std::cout<<Simulator::Now ()<<" vehicle "<< *i << " removed "<< std::endl;
      if (it != in.end())
      {
        in.erase(it);
        i = out.erase(i);
      }
      else
      {
        ++i;
      }
    }

    // -------- create new nodes and start applications.
    NodeContainer node_container;
    node_container.Create(in.size());
    int j = 0;
    for (vector<string>::iterator i = in.begin(); i != in.end(); ++i)
    {
      std::cout<<Simulator::Now ()<<" vehicle "<< *i << " added "<< std::endl;
      Names::Add("Nodes", (*i), node_container.Get(j));
      ++j;
    }

    //-- Mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(node_container);

    //--  Network
    CreateNetworkDevices(node_container);

    //-- Application
    for (NodeContainer::Iterator i = node_container.Begin(); i != node_container.End(); ++i)
    {
      Ptr<Application> app = m_application_factory.Create<Application>();
      (*i)->AddApplication(app);
      app->SetStartTime(Seconds(1.0)); // allow some delay to let possible setup procedures take place
    }

    // -------- set 'down' interfaces of terminated vehicles and remove nodes from the channel.
    //NodeContainer to_destroy;
    for (std::vector<std::string>::iterator i = out.begin(); i != out.end(); ++i)
    {
      Ptr<Node> n = Names::Find<Node>((*i));
      m_toDestroy.push_back (n);
      std::cout<<" add one in m_toDestroy: "<< *i << std::endl;
    }
    //DestroyNetworkDevices();

    // -------- update the set of running vehicles

    for (std::vector<std::string>::iterator i = out.begin(); i != out.end(); ++i)
    {
      vector<string>::iterator it;

      it = std::find(runningVehicles.begin(), runningVehicles.end(), (*i));
      if (it != runningVehicles.end())
      {

        runningVehicles.erase(it);
      }
    }

    runningVehicles.insert(runningVehicles.end(), in.begin(), in.end());

    in.clear();
    out.clear();

  }

  void
  Ovnis::initializeNetwork()
  {
    NS_LOG_FUNCTION_NOARGS();
    // LOW - setup phy/channel
    initializeLowLayersNetwork();
    // HIGH - setup wifi
    initializeHighLayersNetwork();
  }

  void
  Ovnis::initializeLowLayersNetwork()
  {
    NS_LOG_FUNCTION_NOARGS();
#if !defined (YANS_WIFI)
    phy = PhySimWifiPhyHelper::Default();
    ObjectFactory factory1;
 
    channel = CreateObject<PhySimWifiUniformChannel> ();
    factory1.SetTypeId("ns3::PhySimVehicularChannelPropagationLoss");
    //factory1.SetTypeId("ns3::PhySimConstantPropagationLoss");
    channel->SetPropagationLossModel(factory1.Create<PhySimPropagationLossModel> ());
#else
    phy = YansWifiPhyHelper::Default();
    ObjectFactory factory1;
    channel = CreateObject<YansWifiChannel> ();
 
    factory1.SetTypeId("ns3::LogDistancePropagationLossModel");
    factory1.Set("Exponent", DoubleValue (PATH_LOSS_EXPONENT));
    channel->SetPropagationLossModel(factory1.Create<PropagationLossModel> ());

#endif
    ObjectFactory factory2;
    factory2.SetTypeId("ns3::ConstantSpeedPropagationDelayModel");
    channel->SetPropagationDelayModel(factory2.Create<PropagationDelayModel> ());
    //channel->updateArea(boundaries[0], boundaries[1], communicationRange);
    phy.Set ("TxPowerStart",  DoubleValue(DEFAULT_POWER));
    phy.Set ("TxPowerEnd",  DoubleValue(DEFAULT_POWER + 1));
    phy.Set ("TxPowerLevels", UintegerValue (2));
    phy.Set ("ChannelNumber", UintegerValue (CONTROL_CHANNEL));
    phy.SetChannel(channel);

  }
  void
  Ovnis::initializeHighLayersNetwork()
  {
    NS_LOG_FUNCTION_NOARGS();

    wifi = WifiHelper::Default();
    wifi.SetStandard(WIFI_PHY_STANDARD_80211p_SCH);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate6Mbps"));
    //address.SetBase("10.0.0.0", "255.0.0.0");
    mac = NqosWifiMacHelper::Default();
    mac.SetType("ns3::AdhocWifiMac");

  }
  void
  Ovnis::uppdateVehiclesPositions()
  {
    NS_LOG_FUNCTION_NOARGS();

    for (vector<string>::iterator i = runningVehicles.begin(); i != runningVehicles.end(); ++i)
    {
      Ptr<Node> node = Names::Find<Node>((*i));
      Ptr<Object> object = node;
      Ptr<ConstantVelocityMobilityModel> model = object->GetObject<ConstantVelocityMobilityModel> ();

      Position2D newPos;

      traciClient ->commandGetVariablePosition2D (CMD_GET_VEHICLE_VARIABLE, VAR_POSITION, (*i), newPos);
      double  newSpeed;
      traciClient->CommandGetVariableDouble (CMD_GET_VEHICLE_VARIABLE, VAR_SPEED, (*i), newSpeed);
      double newAngle;
      traciClient->CommandGetVariableDouble (CMD_GET_VEHICLE_VARIABLE, VAR_ANGLE, (*i), newAngle);

      Vector velocity(newSpeed * cos((newAngle + 90) * PI / 180.0), newSpeed * sin((newAngle - 90) * PI / 180.0), 0.0);
      Vector position(newPos.x, newPos.y, 0.0);

      model->SetPosition(position);
      // Note : set the velocity AFTER the position . Else velocity is reset.
      model->SetVelocity(velocity);

      /*
      Ptr<NetDevice> d = node->GetDevice(0);
      Ptr<WifiNetDevice> wd = DynamicCast<WifiNetDevice> (d);
      Ptr<WifiPhy> wp = wd->GetPhy();
      Ptr<YansWifiPhy> ywp = DynamicCast<YansWifiPhy> (wp);
      channel->updatePhy(ywp);
      globalStat("VehicleSpeed", newSpeed);
      */
    }
  }

  void
  Ovnis::trafficSimulationStep()
  {
    NS_LOG_FUNCTION(Simulator::Now().GetSeconds());

    //  end = mach_absolute_time();
    //  mach_absolute_difference(end, start, &tp);

    //  globalStat("StepDuration",(double)(((tp.tv_sec*1000000000)+tp.tv_nsec) / 1000000.0));
    //  start=end;

    //std::cout<<" checking if a new slot has come" << std::endl;
    if ( IsSlotUpdated () == true)
    {
      DestroyNetworkDevices();
    }

    // real loop until stop time
    traciClient->simulationStep(currentTime+1000, currentTime, in, out);
    //traciClient->simulationStep(Simulator::Now ().GetMilliSeconds (), currentTime, in, out);
    //std::cout<<"simulator::now (): "<< Simulator::Now ().GetMilliSeconds () <<" currentTime: "<< currentTime << std::endl;
    /*
    for (std::vector<string>::iterator it = in.begin (); it != in.end (); ++ it )
    {
      std::cout<<" in: "<< *it ;
    }
    std::cout<<std::endl;

    for (std::vector<string>::iterator it = out.begin (); it != out.end (); ++ it)
    {
      std::cout<<" out: "<< *it;
    }
    std::cout<<std::endl;
    */

    // update running vehicles
    updateInOutVehicles();

    // move nodes for real
    uppdateVehiclesPositions();

    if (currentTime < (stopTime*1000))
    {
      //Simulator::Schedule(MilliSeconds(SLOT_LENGTH), &Ovnis::trafficSimulationStep, this);
      Simulator::Schedule(MilliSeconds(150), &Ovnis::trafficSimulationStep, this);
    }
    else
    {

      //stop applications
      for (std::vector<std::string>::iterator i = runningVehicles.begin(); i != runningVehicles.end(); ++i)
      {
        Ptr<Node> node = Names::Find<Node>((*i));

        uint32_t number_of_device = node->GetNDevices ();
        for (uint32_t i = 0; i < number_of_device; ++ i)
        {
          node->GetDevice (i)->Dispose (); // distroy every net device that has been aggregated to the node.
        }
        for (uint32_t i = 0; i < node->GetNApplications(); ++i)
        {
          node->GetApplication(i)->SetStopTime(Simulator::Now());
        }
      }
    }

    computeStats();

  }

  void
  Ovnis::globalStat(const string & key, double value)
  {
    //NS_LOG_FUNCTION_NOARGS();
    map<string, StatInput>::iterator iter = stats.find(key);
    if (iter != stats.end())
    {
      iter->second.sum += value;
      iter->second.inputs++;
    }
    else
    {
      StatInput si;
      si.inputs = 1;
      si.sum = value;
      stats[key] = si;
    }
  }

  void
  Ovnis::computeStats()
  {
    NS_LOG_FUNCTION_NOARGS();

    //  if(stats.find("SentData") != stats.end())
    //  cout<<"XXX "<<stats.find("StepDuration")->second.sum<<" "<<stats.find("SentData")->second.sum<<endl;


    for (statType::iterator i = stats.begin(); i != stats.end(); i++)
    {
      //cout << (*i).first << " " << currentTime << " " << (*i).second.sum << " " << (*i).second.inputs << endl;
      (*i).second.inputs = 0;
      (*i).second.sum = 0;
    }

  }

  void
  Ovnis::move()
  {
    NS_LOG_FUNCTION(Simulator::Now().GetSeconds());

    for (NodeContainer::Iterator i = node_container.Begin(); i != node_container.End(); ++i)
    {

      Ptr<Object> object = (*i);
      Ptr<ConstantVelocityMobilityModel> model = object->GetObject<ConstantVelocityMobilityModel> ();
      Vector pos = model->GetPosition();
      //Vector vel = model->GetVelocity();

      //  cout << "pos: "<<pos.x <<  " " << pos.y << endl;
      //  cout << "vel: "<<vel.x << " " << vel.y << endl;
    }

    if (currentTime < (stopTime*1000))
    {
      Simulator::Schedule(MilliSeconds(MOVE_INTERVAL), &Ovnis::move, this);
    }
  }

  void
  Ovnis::run ()
  {
    NS_LOG_FUNCTION(Simulator::Now().GetSeconds());

    Ovnis::Start ();

    if (currentTime < (stopTime*1000))
    {
      Simulator::Schedule(Seconds(1.0), &Ovnis::trafficSimulationStep, this);
      Simulator::Schedule(Seconds(1.1), &Ovnis::move, this);

    }

  }

  long
  Ovnis::getCurrentTime()
  {
    return currentTime;
  }

  bool Ovnis::IsSlotUpdated ()
  {

    uint64_t slot = Simulator::Now ().GetNanoSeconds () / (SLOT_LENGTH * 1000);
    if ( slot > m_slot)
    {
      m_slot = slot;
      return true;
    }
    else
    {
      return false;
    }
  }
}
