/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TRACI_CLIENT_H
#define TRACI_CLIENT_H

#include <string>
#include <sstream>
#include <vector>

#include <foreign/tcpip/socket.h>
#include <utils/common/ToString.h>
//#include "TraCIAPI.h"  // added 2013-12-06 Fri 02:01 PM

#include "ns3/core-module.h"
#include "ns3/network-module.h"

#include "ns3/object.h"


namespace ns3
{
  struct Position2D
  {
    float x;
    float y;
  };

  struct Position3D
  {
    float x;
    float y;
    float z;
  };

  struct PositionRoadMap
  {
    std::string roadId;
    float pos;
    int laneId;
  };

  struct BoundingBox
  {
    Position2D lowerLeft;
    Position2D upperRight;
  };

  typedef std::vector<Position2D> Polygon;

  struct TLPhase
  {
    std::string precRoadId;
    std::string succRoadId;
    int phase;
  };

  typedef std::vector<TLPhase> TLPhaseList;


  /**
   * Client interface to the TraCI server that allow controlling SUMO from a remote host/process.
   */
  class TraciClient: public ns3::Object
  {

  public:

    TraciClient();

    ~TraciClient();
    // -----------------------------------------------------------------------------------------------------------------
    //                                                API RE-WRITE
    //2013-12-06 Fri 02:23 PM
    // -----------------------------------------------------------------------------------------------------------------
    void CommandGetVariableString (int domId, int varId, const std::string &objId, std::string& value, tcpip::Storage* addData=0);
    void commandGetVariableStringList (int domId, int varId, const std::string &objId, std::vector<std::string> value, tcpip::Storage* addData=0);
    void CommandGetVariableUbyte (int domId, int varId, const std::string &objId, int& value, tcpip::Storage* addData=0);
    void CommandGetVariableByte (int domId, int varId, const std::string &objId, int& value, tcpip::Storage* addData=0);
    void CommandGetVariableInteger (int domId, int varId, const std::string &objId, int& value, tcpip::Storage* addData=0);
    void CommandGetVariableFloat (int domId, int varId, const std::string &objId, float& value, tcpip::Storage* addData=0);
    void CommandGetVariableDouble (int domId, int varId, const std::string &objId, double& value, tcpip::Storage* addData=0);
    void commandGetVariablePosition2D (int domId, int varId, const std::string &objId, Position2D value, tcpip::Storage* addData=0);


    void send_commandSimulationStep(int time);

    /** @brief Validates the result state of a command
     * @param[in] inMsg The buffer to read the message from
     * @param[in] command The original command id
     * @param[in] ignoreCommandId Whether the returning command id shall be validated
     * @param[in] acknowledgement Pointer to an existing string into which the acknowledgement message shall be inserted
     */
    void check_resultState(tcpip::Storage& inMsg, int command, bool ignoreCommandId = false, std::string* acknowledgement = 0);
    void check_commandGetResult(tcpip::Storage& inMsg, int command, int expectedType = -1, bool ignoreCommandId = false);
    void send_commandGetVariable(int domID, int varID, const std::string& objID, tcpip::Storage* add = 0);



    /**
     * Connect the client to a running instance of SUMO on given host and port.
     *
     * @param host The hostname of the machine SUMO is running on.
     * @param port The port of the machine SUMO is running on.
     * @return True if the connection is successful.
     */
    bool
    connect(std::string host = "localhost", int port = 1234); // taken care of. 2013-12-08 Sun 03:04 PM

    /** @brief Sends a Close command
     */
    void send_commandClose();

    void
    errorMsg(std::stringstream&);

    /**
     * Sends a close command to the server so as the simulation ends.
     */
    void
    commandClose();

    /**
     * Actually closes the communication socket.
     * @return True if it is okay.
     */
    bool
    close();

    /**
     * Ask SUMO for a simulation step (1 second
     * @param targetTime
     * @param time
     * @param in
     * @param out
     * @return
     */
    bool
    simulationStep(int targetTime, int & time, std::vector<std::string> & in, std::vector<std::string> & out);
    // -----------------------------------------------------------------------------------------------------------------


    /**
     * Report errors in the response of the server if any.
     * @param
     * @param
     * @param ignoreCommandId
     * @return
     */
    // use check_resultState instead!!!!!
    bool
    reportResultState(tcpip::Storage&, int, bool ignoreCommandId = false);


    /**
     * Register a submission of receiving lists of injected and removed vehicles, and also the current time.
     *
     * @param start The date at which the submission starts.
     * @param stop The date at which the submission stops.
     */
    void
    submission(int start, int stop); // seems to be fine

    /**
     * Read the result of a command and outputs it in the logger.
     * @param the response from the server.
     * @param the type of data awaited.
     * @return if the format of the answer is OK.
     */
    bool
    readAndReportTypeDependent(tcpip::Storage &, int); // seems to be fine


    // use CommandGetVariableStringList instead
    bool
    getStringList(u_int8_t dom, u_int8_t cmd, const std::string & node, std::vector<std::string> &);

    // use CommandGetVariableString instead
    bool
    getString(u_int8_t dom, u_int8_t cmd, const std::string &, std::string &);

    // use CommandGetVariableFloat instead
    float
    getFloat(u_int8_t dom, u_int8_t cmd, const std::string & node);

    // use commandGetVariablePosition2D instead
    Position2D
    getPosition2D(std::string &veh);

    /**
     * Simulate the crash of a vehicle, setting it's speed to 0 and changing it's color to black.
     * @param nodeId Vehicle's id
     */
   void
    crash(std::string &);

    /**
     * Change the color of a vehicle with r,g,b values (0..255).
     * @param nodeId Vehicle's id
     * @param r Red component of the color
     * @param g Green component of the color
     * @param b Blue component of the color
     */
    void changeColor(std::string &, int,int,int);

    /**
     * All-in-one methods that changes a road (edge) travel time for a given vehicle; then it forces the re-computation of a route, and it finally ask for the new route.
     * @param nodeId
     * @param roadId
     * @param travelTime
     */
    void
   changeRoad(std::string nodeId, std::string roadId, float travelTime);



// XXXXXXXXXXXXXXXXXXXXXX
    void
    changeRoute(std::string nodeId, std::vector<std::string> stringList);



  private:
    tcpip::Socket socket;

  };

}

#endif /* TRACI_CLIENT_H */


