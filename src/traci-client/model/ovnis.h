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
 *
 * @file ovnis.h
 * @date Apr 21, 2010
 *
 * @author Yoann Pigné
 */

#ifndef OVNIS_H_
#define OVNIS_H_


//
// ----- NS-3 related includes
#include <ns3/object.h>
#include "ns3/traci-client-module.h"
#include "ns3/physim-wifi-module.h"

#include "ns3/wifi-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/propagation-module.h"

//--------------------------------------CONSTANT DEFINE----------------------------------
#ifndef PI
#define PI 3.14159265
#endif
#ifndef YANS_WIFI
#define YANS_WIFI
#endif


/// interval of time between 2 active decisions about JAMEs
#ifndef PROACTIVE_INTERVAL
#define PROACTIVE_INTERVAL  5
#endif


/// period of the movement steps
#ifndef MOVE_INTERVAL
#define MOVE_INTERVAL  1
#endif

/// The system path where the SUMO executable is located
#ifndef SUMO_PATH
#define SUMO_PATH "/usr/local/bin/sumo"
#endif

#ifndef SUMO_HOST
#define SUMO_HOST "localhost"
#endif

#ifndef SUMO_CONFIG
#define SUMO_CONFIG "./test.sumo.cfg"
#endif
//-----------------------------------CONSTANT DEFINE FINISHED-----------------------------------------

using namespace std;
//using namespace traciclient;

namespace ns3
{
  class StatInput
  {
  public:
    double sum;
    int inputs;
  };

  class Ovnis : public ns3::Object
  {
  public:

    static TypeId
    GetTypeId(void);

    Ovnis();

    virtual
    ~Ovnis();

    /**
     * Launches the simulation
     */
    void
    run();

    void
    globalStat(const string & name, double value);

    /**
     * utility function. Printing...
     *
     * @param list
     * @return
     */
    static std::string
    outList(std::vector<std::string>& list);

    /**
     * get the current time in the simulation (SUMO and ns3). In milliseconds
     */
    long
    getCurrentTime();

  protected:

    virtual void
    DoDispose(void);

    virtual void
    DoStart(void);

    void
    initializeNetwork();

    void
    initializeLowLayersNetwork();

    void
    initializeHighLayersNetwork();

    void
    CreateNetworkDevices(NodeContainer node_container);

    void
    DestroyNetworkDevices(NodeContainer to_destroy);

    void
    trafficSimulationStep();

    void
    updateInOutVehicles();

    void
    uppdateVehiclesPositions();

    void
    move();


    void
    computeStats();

    /**
     *  current simulation time in ms
     */
    int currentTime;

    // for statistics
    typedef map<string, StatInput> statType;
    statType stats;
    uint64_t start, end;
    struct timespec tp;

    /**
     * the network interface to communicate with SUMO
     */
    Ptr<TraciClient> traciClient;

    /**
     * list of vehicles actually running in at a given time of simulation
     */
    std::vector<std::string> runningVehicles;

    /**
     * list of new vehicles for current simulation step
     */
    std::vector<std::string> in;
    /**
     * list of vehicles to remove from simulation step at this step
     */
    std::vector<std::string> out;
    /**
     * list of all vehicles loaded by SUMO. Not sure it is useful anymore...
     */
    std::vector<std::string> loaded;

    NodeContainer node_container;

    // network
#if !defined (YANS_WIFI)
    PhySimWifiPhyHelper phy;
    PhySimWifiChannelHelper wifiChannel;
    Ptr<PhySimWifiChannel> channel;
#else
    YansWifiPhyHelper phy;
    YansWifiChannelHelper wifiChannel;
    Ptr<YansWifiChannel> channel;
#endif
    NqosWifiMacHelper mac;
    WifiHelper wifi;
    //Ipv4AddressHelper address;
    /**
     * the channel used by all devices
     */

    /**
     * name of the application to be installed on devices
     */
    std::string m_application;
    ObjectFactory m_application_factory;

    /**
     * The configuration file for running SUMO
     */
    std::string sumoConfig;

    /**
     * The host machine on which SUMO will run
     */
    std::string sumoHost;

    /**
     * The system path where the SUMO executable is located
     */
    std::string sumoPath;

    /**
     * The port number (network) on the host machine SUMO will run on
     */
    int port;

    bool verbose;

    /**
     * Start time in the simulation scale (in seconds)
     */
    int startTime;

    /**
     * Stop time in the simulation scale (in seconds)
     */
    int stopTime;

    /**
     * Communication range used to subdivide the simulation space (in meters)
     */
    double communicationRange;
    double boundaries[2];

    /**
     * Do we start SUMO?
     */
    bool startSumo;

  };
}
#endif /* OVNIS_H_ */
