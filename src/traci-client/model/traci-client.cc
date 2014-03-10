/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "traci-client.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <traci-server/TraCIConstants.h>
#include "ns3/log.h"
#define BUILD_TCPIP
#include <unistd.h>
#include <foreign/tcpip/storage.h>
#include <foreign/tcpip/socket.h>
#include <limits.h>
//#include "TraCIAPI.h"  //

namespace ns3 
{

  NS_LOG_COMPONENT_DEFINE ("TraciClient");

  using namespace std;
  using namespace tcpip;

  /* ... */



  TraciClient::TraciClient() :
    socket("localhost", 0)

  {

  }

  TraciClient::~TraciClient()
  {
    //delete socket;
  }

  bool
    TraciClient::connect(std::string host, int port)
    {
      std::stringstream msg;
      //socket = new tcpip::Socket(host, port);
      socket = tcpip::Socket(host, port);
      //    socket.set_blocking(true);
      sleep(2);
      try
      {
        socket.connect();
      }
      catch (SocketException &e)
      {
        msg << "#Error while connecting: " << e.what();
        errorMsg(msg);
        return false;
      }

      return true;
    }

  void
    TraciClient::errorMsg(std::stringstream& msg)
    {
      cerr << msg.str() << endl;

    }

  bool
    TraciClient::close()
    {
      socket.close();
      return true;
    }

  void
    TraciClient::commandClose()
    {


      send_commandClose();
      NS_LOG_DEBUG ("-> Command sent: <Close>:" << std::endl);
      try {
        tcpip::Storage inMsg;
        std::string acknowledgement;
        check_resultState(inMsg, CMD_CLOSE, false, &acknowledgement);
        NS_LOG_DEBUG (acknowledgement << std::endl);
      } catch (tcpip::SocketException& e) {
        NS_LOG_DEBUG (e.what() << std::endl);
      }
    }
      /*
      tcpip::Storage outMsg;
      tcpip::Storage inMsg;
      std::stringstream msg;

      if (socket.port() == 0)
      {
        msg << "#Error while sending command: no connection to server";
        errorMsg(msg);
        return;
      }

      // command length
      outMsg.writeUnsignedByte(1 + 1);
      // command id
      outMsg.writeUnsignedByte(CMD_CLOSE);

      // send request message
      try
      {
        socket.sendExact(outMsg);
      }
      catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

      // receive answer message
      try
      {
        socket.receiveExact(inMsg);
      }
      catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }

      // validate result state
      if (!reportResultState(inMsg, CMD_CLOSE))
      {
        return;
      }
    }
    */

  bool
    TraciClient::reportResultState(tcpip::Storage& inMsg, int command, bool ignoreCommandId)
    {
      uint32_t cmdLength;
      int cmdId;
      int resultType;
      uint32_t cmdStart;
      std::string msg;

      try
      {
        cmdStart = inMsg.position();
        cmdLength = inMsg.readUnsignedByte();
        cmdId = inMsg.readUnsignedByte();
        if (cmdId != command && !ignoreCommandId)
        {
          //			answerLog << "#Error: received status response to command: "
          //					<< cmdId << " but expected: " << command << endl;
          return false;
        }
        resultType = inMsg.readUnsignedByte();
        msg = inMsg.readString();
      }
      catch (std::invalid_argument e)
      {
        int p = inMsg.position();
        NS_LOG_DEBUG( "#Error: an exception was thrown while reading result state message"
            << endl << "---" << p << endl);
        return false;
      }
      switch (resultType)
      {
        case RTYPE_ERR:
          NS_LOG_DEBUG( ".. Answered with error to command (" << cmdId
              << "), [description: " << msg << "]" << endl);
          return false;
        case RTYPE_NOTIMPLEMENTED:
          NS_LOG_DEBUG(".. Sent command is not implemented (" << cmdId
              << "), [description: " << msg << "]" << endl);
          return false;
        case RTYPE_OK:
          //	  NS_LOG_DEBUG(".. Command acknowledged (" << cmdId
          //				<< "), [description: " << msg << "]" << endl);
          break;
        default:
          NS_LOG_DEBUG(".. Answered with unknown result code(" << resultType
              << ") to command(" << cmdId << "), [description: " << msg
              << "]" << endl);
          return false;
      }
      if ((cmdStart + cmdLength) != inMsg.position())
      {
        NS_LOG_DEBUG("#Error: command at position " << cmdStart
            << " has wrong length" << endl);
        return false;
      }

      return true;
    }

  void
    TraciClient::submission(int start, int stop)
    {

      tcpip::Storage outMsg, inMsg;
      std::stringstream msg;
      std::string s = "*";
      outMsg.writeUnsignedByte(0);
      outMsg.writeInt(5 + 1 + 4 + 4 + 4 + (int) s.length() + 1 + 3);
      outMsg.writeUnsignedByte(CMD_SUBSCRIBE_SIM_VARIABLE);
      outMsg.writeInt((start));
      outMsg.writeInt((stop));
      outMsg.writeString(s);
      outMsg.writeUnsignedByte(3);
      //  outMsg.writeUnsignedByte(VAR_LOADED_VEHICLES_IDS);
      outMsg.writeUnsignedByte(VAR_TIME_STEP);
      outMsg.writeUnsignedByte(VAR_DEPARTED_VEHICLES_IDS);
      outMsg.writeUnsignedByte(VAR_ARRIVED_VEHICLES_IDS);

      // send request message
      try
      {
        socket.sendExact(outMsg);
      }
      catch (SocketException &e)
      {
        msg << "Error while sending command: " << e.what();
        errorMsg(msg);
        return;
      }

      try {
        socket.receiveExact(inMsg);
        //tcpip::Storage inMsg;
        std::string acknowledgement;
        check_resultState(inMsg, CMD_SUBSCRIBE_SIM_VARIABLE, false, &acknowledgement);
        NS_LOG_DEBUG (acknowledgement << std::endl);
        return;
      } catch (tcpip::SocketException& e) {
        NS_LOG_DEBUG (e.what() << std::endl);
        return;
      }

      // receive answer message
      /*
      try
      {
        if (!reportResultState(inMsg, CMD_SUBSCRIBE_SIM_VARIABLE))
        {
          return;
        }
      }
      catch (SocketException &e)
      {
        msg << "Error while receiving command: " << e.what();
        errorMsg(msg);
        return;
      }
      */

      // don't validate anything from the answer
    }

  bool
    TraciClient::simulationStep(int targetTime, int & time, std::vector<std::string> & in, std::vector<std::string> & out)
    {

      /*
         std::stringstream msg;
         if (socket.port() == 0)
         {
         msg << "#Error while sending command: no connection to server";
         errorMsg(msg);
         return false;
         }
         tcpip::Storage outMsg;
      // command length
      outMsg.writeUnsignedByte(1 + 1 + 4);
      // command id
      outMsg.writeUnsignedByte(CMD_SIMSTEP2);
      outMsg.writeInt(targetTime);
      // send request message
      socket.sendExact(outMsg);
      */

      //SUMOTime is of type int.  2013-12-07 Sat 10:06 PM
      send_commandSimulationStep(targetTime);
      NS_LOG_DEBUG (" simulation step time: "<< targetTime<<endl);
      tcpip::Storage inMsg;
      try 
      {
        std::string acknowledgement;
        check_resultState(inMsg, CMD_SIMSTEP2, false, &acknowledgement);
        NS_LOG_DEBUG (acknowledgement << std::endl);
        try 
        {
          int noSubscriptions = inMsg.readInt(); // number of subscriptions
          for (int s = 0; s < noSubscriptions; ++s) {
            // for each subsribed response, we check it, and get our interested variables




            try 
            {
              int length = inMsg.readUnsignedByte();
              if (length == 0) {
                length = inMsg.readInt();
              }
              int cmdId = inMsg.readUnsignedByte();
              if (cmdId >= RESPONSE_SUBSCRIBE_INDUCTIONLOOP_VARIABLE && cmdId <= RESPONSE_SUBSCRIBE_GUI_VARIABLE) {
                // try to get information on what are the vehicles that has finished their sumo simulation
                // what are the vehicles that has just started their sumo simulation.
                if (cmdId == RESPONSE_SUBSCRIBE_SIM_VARIABLE) 
                {
                  NS_LOG_DEBUG ( "  CommandID=" << cmdId);
                  string objectId = inMsg.readString (); // explicitly read the object ID in case we use optimized option waf and disabled NS_LOG_DEBUG. 
                  NS_LOG_DEBUG ("  ObjectID=" << objectId );
                  unsigned int varNo = inMsg.readUnsignedByte();
                  NS_LOG_DEBUG ("  #variables=" << varNo << std::endl);
                  for (unsigned int i = 0; i < varNo; ++i) 
                  {
                    unsigned char variableId = inMsg.readUnsignedByte ();
                    NS_LOG_DEBUG ("      VariableID=" << (int)variableId);
                    bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
                    NS_LOG_DEBUG ("      ok=" << ok);
                    int valueDataType = inMsg.readUnsignedByte();
                    NS_LOG_DEBUG (" valueDataType=" << valueDataType);

                    //-----denoting vehicles entering sumo simulation
                    if (variableId == VAR_DEPARTED_VEHICLES_IDS && valueDataType == TYPE_STRINGLIST)
                    {
                      vector<string> s = inMsg.readStringList ();
                      for (vector<string>::iterator _it = s.begin (); _it != s.end (); ++ _it)
                      {
                        in.push_back (*_it);
                      }
                    }
                    //-----denoting vehicles finished their sumo simulation
                    else if (variableId == VAR_ARRIVED_VEHICLES_IDS && valueDataType == TYPE_STRINGLIST)
                    {
                      vector<string> s = inMsg.readStringList ();
                      for (vector<string>::iterator _it = s.begin (); _it != s.end (); ++ _it)
                      {
                        out.push_back (*_it);
                      }
                    }
                    //----------- Get The simulation time step.
                    else if (variableId == VAR_TIME_STEP && ok)
                    {
                      time = inMsg.readInt();
                      NS_LOG_DEBUG( " Time value (ms): " << time << endl);
                    }
                    else
                    {
                      readAndReportTypeDependent(inMsg, valueDataType);
                    }
                  }
                }
              } 
              else if (cmdId >= RESPONSE_SUBSCRIBE_INDUCTIONLOOP_CONTEXT && cmdId <= RESPONSE_SUBSCRIBE_GUI_CONTEXT) 
              {
                NS_LOG_DEBUG ("  CommandID=" << cmdId);
                NS_LOG_DEBUG ("  ObjectID=" << inMsg.readString());
                NS_LOG_DEBUG ("  Domain=" << inMsg.readUnsignedByte());
                unsigned int varNo = inMsg.readUnsignedByte();
                NS_LOG_DEBUG ("  #variables=" << varNo << std::endl);
                unsigned int objNo = inMsg.readInt();
                NS_LOG_DEBUG ("  #objects=" << objNo << std::endl);
                for (unsigned int j = 0; j < objNo; ++j) 
                {
                  NS_LOG_DEBUG ("   ObjectID=" << inMsg.readString() << std::endl);
                  for (unsigned int i = 0; i < varNo; ++i) 
                  {
                    NS_LOG_DEBUG ("      VariableID=" << inMsg.readUnsignedByte());
                    bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
                    NS_LOG_DEBUG ("      ok=" << ok);
                    int valueDataType = inMsg.readUnsignedByte();
                    NS_LOG_DEBUG (" valueDataType=" << valueDataType);
                    readAndReportTypeDependent(inMsg, valueDataType);
                  }
                }
              } else {
                NS_LOG_DEBUG ("#Error: received response with command id: " << cmdId << " but expected a subscription response (0xe0-0xef / 0x90-0x9f)" << std::endl);
                return false;
              }
            } catch (std::invalid_argument& e) {
              NS_LOG_DEBUG ("#Error while reading message:" << e.what() << std::endl);
              return false;
            }
            return true;
          }
        } catch (std::invalid_argument& e) {
          NS_LOG_DEBUG (" #Error while reading message:" << e.what() << std::endl);
          return false;
        }
        return true;
      } catch (tcpip::SocketException& e) 
      {
        NS_LOG_DEBUG (" "<<e.what () << endl);
        return false;
      }
    }

  /*
     tcpip::Storage outMsg;
     tcpip::Storage inMsg;
     std::stringstream msg;

     if (socket.port() == 0)
     {
     msg << "#Error while sending command: no connection to server";
     errorMsg(msg);
     return false;
     }

  // command length
  outMsg.writeUnsignedByte(1 + 1 + 4);
  // command id
  outMsg.writeUnsignedByte(CMD_SIMSTEP2);
  outMsg.writeInt((targetTime));
  // send request message
  try
  {
  socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
  msg << "--Error while sending command: " << e.what();
  errorMsg(msg);
  return false;
  }
  // receive answer message
  try
  {
  socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
  msg << "Error while receiving command: " << e.what();
  errorMsg(msg);
  return false;
  }
  // validate result state
  if (!reportResultState(inMsg, CMD_SIMSTEP2))
  {
  return false;
  }
  // validate answer message
  try
  {
  int noSubscriptions = inMsg.readInt(); // number of subscriptions 2013-12-07 Sat 08:58 PM
  for (int s = 0; s < noSubscriptions; ++s)
  {

  try
  {
  uint32_t respStart = inMsg.position();
  //                  std::cout << respStart << endl;
  int extLength = inMsg.readUnsignedByte();
  //                  std::cout << extLength << endl;
  int respLength = inMsg.readInt();
  //                  std::cout << respLength << endl ;

  int cmdId = inMsg.readUnsignedByte();
  //                 std::cout<<cmdId<<endl<< "--------------" << endl;

  if (cmdId < 0xe0 || cmdId > 0xef)
  {
  NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId
  << " but expected a subscription response (0xe0-0xef)" << endl);
  return false;
  }
  NS_LOG_DEBUG( "  CommandID=" << cmdId);
  string oId = inMsg.readString();
  NS_LOG_DEBUG("  ObjectID=" << oId);
  unsigned int varNo = inMsg.readUnsignedByte();
  NS_LOG_DEBUG( "  #variables=" << varNo << endl);

  switch (cmdId)
  {
    case RESPONSE_SUBSCRIBE_VEHICLE_VARIABLE:
      for (int i = 0; i < varNo; ++i)
      {
        if (inMsg.readUnsignedByte() == ID_LIST && inMsg.readUnsignedByte() == RTYPE_OK)
        {
          // value data type
          inMsg.readUnsignedByte();

          vector<string> s = inMsg.readStringList();

          NS_LOG_DEBUG( " string list value: [ " << endl);
          for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
          {
            if (i != s.begin())
            {
              NS_LOG_DEBUG(", ");
            }
            NS_LOG_DEBUG( "'" << *i);// << "' (");
            //                                  Position2D *p = &silentAskPosition2D(*i);
            //                                  NS_LOG_DEBUG(p->x << "," << p->y << "):" << silentAskRoad(*i));
          }
          NS_LOG_DEBUG( " ]" << endl);

        }
      }
      break;

    case RESPONSE_SUBSCRIBE_SIM_VARIABLE:
      for (int i = 0; i < varNo; ++i)
      {
        int varId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( "      VariableID=" << varId);
        bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
        int valueDataType = inMsg.readUnsignedByte();

        //----------  Get the list of vehicle that entered the simulation.
        if (varId == VAR_DEPARTED_VEHICLES_IDS && ok)
        {

          vector<string> s = inMsg.readStringList();

          NS_LOG_DEBUG( " string list value: [ " << endl);
          for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
          {
            if (i != s.begin())
            {
              NS_LOG_DEBUG( ", ");
            }
            NS_LOG_DEBUG( "'" << *i);

            in.push_back((*i));

            //                                  if (rand() % 100 > 80)
            //                                    {
            //                                      silentChangeRoute(*i, "middle", (double) INT_MAX);
            //                                      NS_LOG_DEBUG( "(changed)");
            //                                    }
          }
          NS_LOG_DEBUG( " ]" << endl);

        }
        //-------------- Get the list of vehicles that finished their trip and got out of the simulation.
        else if (varId == VAR_ARRIVED_VEHICLES_IDS && ok)
        {
          vector<string> s = inMsg.readStringList();

          NS_LOG_DEBUG( " string list value: [ " << endl);
          for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
          {
            if (i != s.begin())
            {
              NS_LOG_DEBUG( ", ");
            }
            NS_LOG_DEBUG( "'" << *i);

            out.push_back((*i));

            //                                  if (rand() % 100 > 80)
            //                                    {
            //                                      silentChangeRoute(*i, "middle", (double) INT_MAX);
            //                                      NS_LOG_DEBUG( "(changed)");
            //                                    }
          }
          NS_LOG_DEBUG( " ]" << endl);

        }
        //----------- Get The simulation time step.
        else if (varId == VAR_TIME_STEP && ok)
        {
          time = inMsg.readInt();
          NS_LOG_DEBUG( " Time value (ms): " << time << endl);
        }
        else
        {
          readAndReportTypeDependent(inMsg, valueDataType);
        }
      }
      break;

    default:

      for (uint32_t i = 0; i < varNo; ++i)
      {
        int vId = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( "      VariableID=" << vId);
        bool ok = inMsg.readUnsignedByte() == RTYPE_OK;
        NS_LOG_DEBUG( "      ok=" << ok);
        int valueDataType = inMsg.readUnsignedByte();
        NS_LOG_DEBUG( " valueDataType=" << valueDataType);
        readAndReportTypeDependent(inMsg, valueDataType);
      }
  }
}
catch (std::invalid_argument e)
{
  NS_LOG_DEBUG( "#Error while reading message:" << e.what() << endl);
  return false;
}

}
}
catch (std::invalid_argument e)
{
  NS_LOG_DEBUG( "#Error while reading message:" << e.what() << endl);
  return false;
}
return true;
}
*/

  float
TraciClient::getFloat(u_int8_t dom, u_int8_t cmd, const std::string & node)
{
  tcpip::Storage outMsg, inMsg;
  std::stringstream msg;
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
  outMsg.writeUnsignedByte(dom);
  outMsg.writeUnsignedByte(cmd);
  outMsg.writeString(node);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "+++Error while sending command: " << e.what();
    errorMsg(msg);
    return false;
  }
  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }


  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, dom, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return false;
  }

  // validate result state
  /*
  if (!reportResultState(inMsg, dom))
  {
    return false;
  }
  */
  // validate answer message
  try
  {
    uint32_t respStart = inMsg.position();
    uint32_t extLength = inMsg.readUnsignedByte();
    uint32_t respLength = inMsg.readInt();
    uint32_t cmdId = inMsg.readUnsignedByte();
    if (cmdId != (dom + 0x10))
    {
      NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
            + 0x10) << endl);
      return false;
    }
    NS_LOG_DEBUG( "  CommandID=" << cmdId);
    int vId = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( "  VariableID=" << vId);
    string oId = inMsg.readString();
    NS_LOG_DEBUG( "  ObjectID=" << oId);
    int valueDataType = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " valueDataType=" << valueDataType);

    //data should be string list
    //          readAndReportTypeDependent(inMsg, valueDataType);
    if (valueDataType == TYPE_FLOAT)
    {
      float f = inMsg.readFloat();
      NS_LOG_DEBUG( " float value:  " <<f<< endl);
      return f;
    }
    else
    {

      return 0;
    }

  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }
  return 0;

}

  bool
TraciClient::getString(u_int8_t dom, u_int8_t cmd, const std::string & node, std::string & value)
{
  tcpip::Storage outMsg, inMsg;
  std::stringstream msg;
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
  outMsg.writeUnsignedByte(dom);
  outMsg.writeUnsignedByte(cmd);
  outMsg.writeString(node);
  std::cout<<" sending out: "<< " dom: "<< (uint32_t) dom << " cmd: "<< (uint32_t) cmd <<" node: "<< node <<" value: "<< value<< std::endl;
  // send request message

  if (socket.port() == 0)
  {

    std::cerr << "Error while sending command: no connection to server";
    std::flush(std::cerr);

  }
  try
  {
    socket.sendExact(outMsg);

  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return false;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }

  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, dom, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return false;
  }
  // validate result state
  /*
  if (!reportResultState(inMsg, dom))
  {
    return false;
  }
  */

  // validate answer message
  try
  {
    /*
       int respStart = inMsg.position();
       int extLength = inMsg.readUnsignedByte();
       int respLength = inMsg.readInt();
       */
    int length = inMsg.readUnsignedByte ();
    if (length == 0)
    {
      length = inMsg.readInt ();
    }
    int cmdId = inMsg.readUnsignedByte();
    std::cout<<" Length: "<< length <<" cmdId: "<< cmdId <<" dom + 0x10: "<< (dom + 0x10) << std::endl;
    if (cmdId != (dom + 0x10))
    {
      NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
            + 0x10) << endl);
      return false;
    }
    NS_LOG_DEBUG( "  CommandID=" << cmdId);
    int vId = inMsg.readUnsignedByte();
    std::cout<<" receiving: "<< " vid: "<< vId;
    NS_LOG_DEBUG( "  VariableID=" << vId);
    string oId = inMsg.readString();
    std::cout<<" node: "<< oId ;
    NS_LOG_DEBUG( "  ObjectID=" << oId);
    int valueDataType = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " valueDataType=" << valueDataType);
    std::cout<<" commandid: "<< cmdId <<" receiving, vid: "<< vId <<" node: "<< oId << " valueDateType: "<< valueDataType << std::endl;

    //data should be string list
    //          readAndReportTypeDependent(inMsg, valueDataType);
    if (valueDataType == TYPE_STRING)
    {
      value.assign(inMsg.readString());
      std::cout<<" value: "<< value << std::endl;
      NS_LOG_DEBUG( " string value:  " <<value<< std::endl);
      return true;
    }
    else
    {

      return 0;
    }

  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }
  return 0;

}

  bool
TraciClient::getStringList(u_int8_t dom, u_int8_t cmd, const std::string & node, std::vector<std::string>& list)
{
  tcpip::Storage outMsg, inMsg;
  std::stringstream msg;
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) node.length()));
  outMsg.writeUnsignedByte(dom);
  outMsg.writeUnsignedByte(cmd);
  outMsg.writeString(node);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return false;
  }
  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }
  // validate result state

  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, dom, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return false;
  }
  /*
  if (!reportResultState(inMsg, dom))
  {
    return false;
  }
  */
  // validate answer message
  try
  {
    int respStart = inMsg.position();
    int extLength = inMsg.readUnsignedByte();
    int respLength = inMsg.readInt();
    int cmdId = inMsg.readUnsignedByte();
    if (cmdId != (dom + 0x10))
    {
      NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId << "but expected: " << (int) (dom
            + 0x10) << endl);
      return false;
    }
    NS_LOG_DEBUG( "  CommandID=" << cmdId);
    int vId = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( "  VariableID=" << vId);
    string oId = inMsg.readString();
    NS_LOG_DEBUG( "  ObjectID=" << oId);
    int valueDataType = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " valueDataType=" << valueDataType);

    //data should be string list
    //          readAndReportTypeDependent(inMsg, valueDataType);
    if (valueDataType == TYPE_STRINGLIST)
    {
      vector<string> s = inMsg.readStringList();
      NS_LOG_DEBUG( " string list value: [ " << endl);
      for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
      {
        if (i != s.begin())
        {
          NS_LOG_DEBUG( ", ");
        }
        //                          std::cout<<(*i)<<endl;
        list.push_back((*i));
        NS_LOG_DEBUG( '"' << *i << '"');
      }
      NS_LOG_DEBUG( " ]" << endl);
    }
    else
    {

      return false;
    }

  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return false;
  }
  return true;
}

  bool
TraciClient::readAndReportTypeDependent(tcpip::Storage &inMsg, int valueDataType)
{
  if (valueDataType == TYPE_UBYTE)
  {
    int ubyte = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " Unsigned Byte Value: " << ubyte << endl);
  }
  else if (valueDataType == TYPE_BYTE)
  {
    int byte = inMsg.readByte();
    NS_LOG_DEBUG( " Byte value: " << byte << endl);
  }
  else if (valueDataType == TYPE_INTEGER)
  {
    int integer = inMsg.readInt();
    NS_LOG_DEBUG( " Int value: " << integer << endl);
  }
  else if (valueDataType == TYPE_FLOAT)
  {
    float floatv = inMsg.readFloat();
    //        if (floatv < 0.1 && floatv > 0)
    //          {
    //            answerLog.setf(std::ios::scientific, std::ios::floatfield);
    //          }
    NS_LOG_DEBUG( " float value: " << floatv << endl);
  }
  else if (valueDataType == TYPE_DOUBLE)
  {
    double doublev = inMsg.readDouble();
    NS_LOG_DEBUG( " Double value: " << doublev << endl);
  }
  else if (valueDataType == TYPE_BOUNDINGBOX)
  {
    BoundingBox box;
    box.lowerLeft.x = inMsg.readFloat();
    box.lowerLeft.y = inMsg.readFloat();
    box.upperRight.x = inMsg.readFloat();
    box.upperRight.y = inMsg.readFloat();
    NS_LOG_DEBUG( " BoundaryBoxValue: lowerLeft x=" << box.lowerLeft.x
        << " y=" << box.lowerLeft.y << " upperRight x="
        << box.upperRight.x << " y=" << box.upperRight.y << endl);
  }
  else if (valueDataType == TYPE_POLYGON)
  {
    int length = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " PolygonValue: ");
    for (int i = 0; i < length; i++)
    {
      float x = inMsg.readFloat();
      float y = inMsg.readFloat();
      NS_LOG_DEBUG( "(" << x << "," << y << ") ");
    }
    NS_LOG_DEBUG( endl);
  }
  else if (valueDataType == POSITION_3D)
  {
    float x = inMsg.readFloat();
    float y = inMsg.readFloat();
    float z = inMsg.readFloat();
    NS_LOG_DEBUG( " Position3DValue: " << std::endl);
    NS_LOG_DEBUG( " x: " << x << " y: " << y << " z: " << z << std::endl);
  }
  else if (valueDataType == POSITION_ROADMAP)
  {
    std::string roadId = inMsg.readString();
    float pos = inMsg.readFloat();
    int laneId = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " RoadMapPositionValue: roadId=" << roadId << " pos="
        << pos << " laneId=" << laneId << std::endl);
  }
  else if (valueDataType == TYPE_TLPHASELIST)
  {
    int length = inMsg.readUnsignedByte();
    NS_LOG_DEBUG( " TLPhaseListValue: length=" << length << endl);
    for (int i = 0; i < length; i++)
    {
      string pred = inMsg.readString();
      string succ = inMsg.readString();
      int phase = inMsg.readUnsignedByte();
      NS_LOG_DEBUG( " precRoad=" << pred << " succRoad=" << succ
          << " phase=");
      switch (phase)
      {
        case TLPHASE_RED:
          NS_LOG_DEBUG( "red" << endl);
          break;
        case TLPHASE_YELLOW:
          NS_LOG_DEBUG("yellow" << endl);
          break;
        case TLPHASE_GREEN:
          NS_LOG_DEBUG( "green" << endl);
          break;
        default:
          NS_LOG_DEBUG( "#Error: unknown phase value" << (int) phase
              << endl);
          return false;
      }
    }
  }
  else if (valueDataType == TYPE_STRING)
  {
    string s = inMsg.readString();
    NS_LOG_DEBUG( " string value: " << s << endl);
  }
  else if (valueDataType == TYPE_STRINGLIST)
  {
    vector<string> s = inMsg.readStringList();
    NS_LOG_DEBUG( " string list value: [ " << endl);
    for (vector<string>::iterator i = s.begin(); i != s.end(); ++i)
    {
      if (i != s.begin())
      {
        NS_LOG_DEBUG( ", ");
      }
      NS_LOG_DEBUG( '"' << *i << '"');
    }
    NS_LOG_DEBUG( " ]" << endl);
  }
  else if (valueDataType == TYPE_COMPOUND)
  {
    int no = inMsg.readInt();
    NS_LOG_DEBUG( " compound value with " << no << " members: [ " << endl);
    for (int i = 0; i < no; ++i)
    {
      int currentValueDataType = inMsg.readUnsignedByte();
      NS_LOG_DEBUG( " valueDataType=" << currentValueDataType);
      readAndReportTypeDependent(inMsg, currentValueDataType);
    }
    NS_LOG_DEBUG( " ]" << endl);
  }
  else if (valueDataType == POSITION_2D)
  {
    float xv = inMsg.readFloat();
    float yv = inMsg.readFloat();
    NS_LOG_DEBUG( " position value: (" << xv << "," << yv << ")" << endl);
  }
  else if (valueDataType == TYPE_COLOR)
  {
    int r = inMsg.readUnsignedByte();
    int g = inMsg.readUnsignedByte();
    int b = inMsg.readUnsignedByte();
    int a = inMsg.readUnsignedByte();
    NS_LOG_DEBUG(" color value: (" << r << "," << g << "," << b << "," << a
        << ")" << endl);
  }
  else
  {
    NS_LOG_DEBUG( "#Error: unknown valueDataType!" << endl);
    return false;
  }
  return true;
}

  Position2D
TraciClient::getPosition2D(std::string &veh)

{
  Position2D p;
  tcpip::Storage outMsg, inMsg;
  std::stringstream msg;
  if (socket.port() == 0)
  {
    msg << "#Error while sending command: no connection to server";
    errorMsg(msg);
    return p;
  }
  // command length
  outMsg.writeUnsignedByte(1 + 1 + 1 + 4 + (int) veh.length());
  // command id
  outMsg.writeUnsignedByte(CMD_GET_VEHICLE_VARIABLE);
  // variable id
  outMsg.writeUnsignedByte(VAR_POSITION);
  // object id
  outMsg.writeString(veh);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return p;
  }


  try {
    //tcpip::Storage inMsg;
    socket.receiveExact(inMsg);
    std::string acknowledgement;
    check_resultState(inMsg, CMD_GET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return p;
  }

  // receive answer message
  /*
  try
  {
    socket.receiveExact(inMsg);
    if (!reportResultState(inMsg, CMD_GET_VEHICLE_VARIABLE))
    {
      return p;
    }
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return p;
  }
  */
  // validate result state
  try
  {
    uint32_t respStart = inMsg.position();
    int extLength = inMsg.readUnsignedByte();
    int respLength = inMsg.readInt();
    int cmdId = inMsg.readUnsignedByte();
    if (cmdId != (CMD_GET_VEHICLE_VARIABLE + 0x10))
    {
      NS_LOG_DEBUG( "#Error: received response with command id: " << cmdId
          << "but expected: " << (int) (CMD_GET_VEHICLE_VARIABLE
            + 0x10) << endl);
      return p;
    }
    //  VariableID=" <<
    inMsg.readUnsignedByte();
    //answerLog << "  ObjectID=" <<
    inMsg.readString();
    //int valueDataType =
    inMsg.readUnsignedByte();

    p.x = inMsg.readFloat();
    p.y = inMsg.readFloat();
    //answerLog << xv << "," << yv << endl;
    return p;
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return p;
  }

}


// XXXXXXXXXXXXXXXXXXX
void
TraciClient::changeRoute(std::string nodeId,std::vector<std::string> stringList){
  tcpip::Storage outMsg;
  tcpip::Storage inMsg;
  std::stringstream msg;

  if (socket.port() == 0)
  {
    msg << "#Error while sending command: no connection to server";
    errorMsg(msg);
    return;
  }

  int size_of_strings=0;
  for (std::vector<std::string>::iterator i = stringList.begin(); i != stringList.end(); ++i)
  {
    size_of_strings+=(4+(*i).size());
  }
  // command length
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4 +
      size_of_strings );
  // command id
  outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);

  outMsg.writeUnsignedByte(VAR_ROUTE);
  // vehicle id
  outMsg.writeString(nodeId);
  //type of value (string list) : byte
  outMsg.writeUnsignedByte(TYPE_STRINGLIST);

  //number of edges in the route : int
  outMsg.writeInt(stringList.size());

  for (std::vector<std::string>::iterator i = stringList.begin(); i != stringList.end(); ++i)
  {
    outMsg.writeString((*i));
  }

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }

  // validate result state

  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return;
  }
  /*
  if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
  {
    return;
  }
  */
}


  void
TraciClient::changeRoad(std::string nodeId, std::string roadId, float travelTime)
{
  tcpip::Storage outMsg;
  tcpip::Storage inMsg;
  std::stringstream msg;

  if (socket.port() == 0)
  {
    msg << "#Error while sending command: no connection to server";
    errorMsg(msg);
    return;
  }

  // command length
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4 + (1 + 4) + (1 + 4) + (1 + (4
          + (int) roadId.length())) + (1 + 4));
  // command id
  outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
  // var id VAR_EDGE_TRAVELTIME
  outMsg.writeUnsignedByte(VAR_EDGE_TRAVELTIME);
  // vehicle id
  outMsg.writeString(nodeId);
  //type of value (compound)
  outMsg.writeUnsignedByte(TYPE_COMPOUND);

  // compoung value for edge travael time;
  //number of elements (always=4)
  outMsg.writeInt(4);

  //int begin
  outMsg.writeUnsignedByte(TYPE_INTEGER);
  outMsg.writeInt(0);
  //int end
  outMsg.writeUnsignedByte(TYPE_INTEGER);
  outMsg.writeInt(INT_MAX);
  //string edge
  outMsg.writeUnsignedByte(TYPE_STRING);
  outMsg.writeString(roadId);
  //float value
  outMsg.writeUnsignedByte(TYPE_FLOAT);
  outMsg.writeFloat(travelTime);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }
  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return;
  }

  // validate result state
  /*
  if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
  {
    return;
  }
  */

  outMsg.reset();
  inMsg.reset();

  // command length
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4);
  // command id
  outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
  // var id VAR_EDGE_TRAVELTIME
  outMsg.writeUnsignedByte(CMD_REROUTE_TRAVELTIME);
  // vehicle id
  outMsg.writeString(nodeId);
  //type of value (compound)
  outMsg.writeUnsignedByte(TYPE_COMPOUND);

  // Compound value for reroute on  travel time;
  //number of elements (always=4)
  outMsg.writeInt(0);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }
  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return;
  }

  // validate result state
  /*
  if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
  {
    return;
  }
  */

}

  void
TraciClient::crash(std::string &nodeId)
{
  tcpip::Storage outMsg;
  tcpip::Storage inMsg;
  std::stringstream msg;

  if (socket.port() == 0)
  {
    msg << "#Error while sending command: no connection to server";
    errorMsg(msg);
    return;
  }

  // command length
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 8 );
  // command id
  outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
  // var id VAR_EDGE_TRAVELTIME
  outMsg.writeUnsignedByte(VAR_SPEED);
  // vehicle id
  outMsg.writeString(nodeId);
  // Type of value (double)
  outMsg.writeUnsignedByte(TYPE_DOUBLE);
  outMsg.writeDouble(0.0001);

  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }
  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return;
  }

  // validate result state
  /*
  if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
  {
    return;
  }
  */

  outMsg.reset();
  inMsg.reset();

  changeColor(nodeId, 0, 0, 0);

}

  void
TraciClient::changeColor(std::string &nodeId, int r, int g, int b)
{
  tcpip::Storage outMsg;
  tcpip::Storage inMsg;
  std::stringstream msg;

  if (socket.port() == 0)
  {
    msg << "#Error while sending command: no connection to server";
    errorMsg(msg);
    return;
  }
  outMsg.writeUnsignedByte(1 + 1 + 1 + (4 + (int) nodeId.length()) + 1 + 4);
  // command id
  outMsg.writeUnsignedByte(CMD_SET_VEHICLE_VARIABLE);
  // var id VAR_EDGE_TRAVELTIME
  outMsg.writeUnsignedByte(VAR_COLOR);
  // vehicle id
  outMsg.writeString(nodeId);
  // Type of value (double)
  outMsg.writeUnsignedByte(TYPE_COLOR);
  outMsg.writeUnsignedByte(r);
  outMsg.writeUnsignedByte(g);
  outMsg.writeUnsignedByte(b);
  outMsg.writeUnsignedByte(1);
  // send request message
  try
  {
    socket.sendExact(outMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while sending command: " << e.what();
    errorMsg(msg);
    return;
  }

  // receive answer message
  try
  {
    socket.receiveExact(inMsg);
  }
  catch (SocketException &e)
  {
    msg << "Error while receiving command: " << e.what();
    errorMsg(msg);
    return;
  }

  // validate result state

  try {
    //tcpip::Storage inMsg;
    std::string acknowledgement;
    check_resultState(inMsg, CMD_SET_VEHICLE_VARIABLE, false, &acknowledgement);
    NS_LOG_DEBUG (acknowledgement << std::endl);
  } catch (tcpip::SocketException& e) {
    NS_LOG_DEBUG (e.what() << std::endl);
    return;
  }
  /*
  if (!reportResultState(inMsg, CMD_SET_VEHICLE_VARIABLE))
  {
    return;
  }
  */
}
//------------------------------------------------------------------------------------------------------------------

void TraciClient::CommandGetVariableString (int domId, int varId, const std::string &objId, 
    string& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_STRING)
    {
      value = inMsg.readString ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::commandGetVariableStringList (int domId, int varId, const std::string &objId, std::vector<std::string> value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_STRINGLIST)
    {
        std::vector<std::string> s = inMsg.readStringList();
        //answerLog << " string list value: [ " << std::endl;
        for (std::vector<std::string>::iterator i = s.begin(); i != s.end(); ++i) {
          value.push_back (*i);
        }
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::CommandGetVariableUbyte (int domId, int varId, const std::string &objId, 
    int& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_UBYTE)
    {
      value = inMsg.readUnsignedByte ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::CommandGetVariableByte (int domId, int varId, const std::string &objId, 
    int& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_BYTE)
    {
      value = inMsg.readByte ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}
void TraciClient::CommandGetVariableInteger (int domId, int varId, const std::string &objId, 
    int& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_INTEGER)
    {
      value = inMsg.readInt ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

    void TraciClient::commandGetVariablePosition2D (int domId, int varId, const std::string &objId, Position2D value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == POSITION_2D)
    {
      value.x = inMsg.readDouble ();
      value.y = inMsg.readDouble ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::CommandGetVariableFloat (int domId, int varId, const std::string &objId, 
    float& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_FLOAT)
    {
      value = inMsg.readFloat ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::CommandGetVariableDouble (int domId, int varId, const std::string &objId, 
    double& value, tcpip::Storage* addData)
{
  send_commandGetVariable (domId, varId, objId, addData);    

  tcpip::Storage inMsg;
  try {
    std::string acknowledgement;
    check_resultState (inMsg, domId, false, &acknowledgement);
  }
  catch (tcpip::SocketException& e)
  {
    NS_LOG_DEBUG( e.what ());
  }
  check_commandGetResult (inMsg, domId, -1, false);
  try
  {
    int variableId = inMsg.readUnsignedByte ();
    std::string objectId = inMsg.readString ();
    int valueDataType = inMsg.readUnsignedByte ();
    if (valueDataType == TYPE_DOUBLE)
    {
      value = inMsg.readDouble ();
    }
    else
    {
      return;
    }
    //readAndReportTypeDependent (inMsg, valueDataType);
  }
  catch (tcpip::SocketException& e)
  {
    std::stringstream msg;
    msg <<" Error while receiving command: "<< e.what ();
    errorMsg (msg);
    return;
  }
}

void TraciClient::send_commandSimulationStep(int time)
{
  tcpip::Storage outMsg;
  // command length
  outMsg.writeUnsignedByte(1 + 1 + 4);
  // command id
  outMsg.writeUnsignedByte(CMD_SIMSTEP2);
  outMsg.writeInt(time);
  // send request message
  socket.sendExact(outMsg);
}




void
TraciClient::check_resultState(tcpip::Storage& inMsg, int command, bool ignoreCommandId, std::string* acknowledgement) 
{
    socket.receiveExact(inMsg);
    int cmdLength;
    int cmdId;
    int resultType;
    int cmdStart;
    std::string msg;
    try {
        cmdStart = inMsg.position();
        cmdLength = inMsg.readUnsignedByte();
        cmdId = inMsg.readUnsignedByte();
        if (command != cmdId && !ignoreCommandId) {
            throw tcpip::SocketException("#Error: received status response to command: " + toString(cmdId) + " but expected: " + toString(command));
        }
        resultType = inMsg.readUnsignedByte();
        msg = inMsg.readString();
    } catch (std::invalid_argument&) {
        throw tcpip::SocketException("#Error: an exception was thrown while reading result state message");
    }
    switch (resultType) {
        case RTYPE_ERR:
            throw tcpip::SocketException(".. Answered with error to command (" + toString(command) + "), [description: " + msg + "]");
        case RTYPE_NOTIMPLEMENTED:
            throw tcpip::SocketException(".. Sent command is not implemented (" + toString(command) + "), [description: " + msg + "]");
        case RTYPE_OK:
            if (acknowledgement != 0) {
                (*acknowledgement) = ".. Command acknowledged (" + toString(command) + "), [description: " + msg + "]";
            }
            break;
        default:
            throw tcpip::SocketException(".. Answered with unknown result code(" + toString(resultType) + ") to command(" + toString(command) + "), [description: " + msg + "]");
    }
    if ((cmdStart + cmdLength) != (int) inMsg.position()) {
        throw tcpip::SocketException("#Error: command at position " + toString(cmdStart) + " has wrong length");
    }
}

void
TraciClient::check_commandGetResult(tcpip::Storage& inMsg, int command, int expectedType, bool ignoreCommandId)
{
    inMsg.position(); // respStart
    int length = inMsg.readUnsignedByte();
    if (length == 0) {
        length = inMsg.readInt();
    }
    int cmdId = inMsg.readUnsignedByte();
    if (!ignoreCommandId && cmdId != (command + 0x10)) {
        throw tcpip::SocketException("#Error: received response with command id: " + toString(cmdId) + "but expected: " + toString(command + 0x10));
    }
    if (expectedType >= 0) {
        int valueDataType = inMsg.readUnsignedByte();
        if (valueDataType != expectedType) {
            throw tcpip::SocketException("Expected " + toString(expectedType) + " but got " + toString(valueDataType));
        }
    }
}


void
TraciClient::send_commandGetVariable(int domID, int varID, const std::string& objID, tcpip::Storage* add) 
{
    if (socket.port () == 0) {
        throw tcpip::SocketException("Socket is not initialised");
    }
    tcpip::Storage outMsg;
    // command length
    unsigned int length = 1 + 1 + 1 + 4 + (int) objID.length();
    if (add != 0) {
        length += (int)add->size();
    }
    outMsg.writeUnsignedByte(length);
    // command id
    outMsg.writeUnsignedByte(domID);
    // variable id
    outMsg.writeUnsignedByte(varID);
    // object id
    outMsg.writeString(objID);
    // additional values
    if (add != 0) {
        outMsg.writeStorage(*add);
    }
    // send request message
    socket.sendExact(outMsg);
}

void
TraciClient::send_commandClose()
{
    tcpip::Storage outMsg;
    // command length
    outMsg.writeUnsignedByte(1 + 1);
    // command id
    outMsg.writeUnsignedByte(CMD_CLOSE);
    socket.sendExact(outMsg);
}
}

