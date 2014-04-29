/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/core-config.h"
#include "simulator.h"
#include "simulator-impl.h"
#include "scheduler.h"
#include "map-scheduler.h"
#include "event-impl.h"
#include "settings.h"

#include "ptr.h"
#include "string.h"
#include "object-factory.h"
#include "global-value.h"
#include "assert.h"
#include "log.h"

#include <math.h>
#include <fstream>
#include <list>
#include <vector>
#include <iostream>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("Simulator");

namespace ns3 {

  GlobalValue g_simTypeImpl = GlobalValue ("SimulatorImplementationType", 
      "The object class to use as the simulator implementation",
      StringValue ("ns3::DefaultSimulatorImpl"),
      MakeStringChecker ());

  GlobalValue g_schedTypeImpl = GlobalValue ("SchedulerType", 
      "The object class to use as the scheduler implementation",
      TypeIdValue (MapScheduler::GetTypeId ()),
      MakeTypeIdChecker ());

  static void
    TimePrinter (std::ostream &os)
    {
      os << Simulator::Now ().GetSeconds () << "s";
    }

  static void
    NodePrinter (std::ostream &os)
    {
      if (Simulator::GetContext () == 0xffffffff)
      {
        os << "-1";
      }
      else
      {
        os << Simulator::GetContext ();
      }
    }

  static SimulatorImpl **PeekImpl (void)
  {
    static SimulatorImpl *impl = 0;
    return &impl;
  }

  static SimulatorImpl * GetImpl (void)
  {
    SimulatorImpl **pimpl = PeekImpl ();
    /* Please, don't include any calls to logging macros in this function
     * or pay the price, that is, stack explosions.
     */
    if (*pimpl == 0)
    {
      {
        ObjectFactory factory;
        StringValue s;

        g_simTypeImpl.GetValue (s);
        factory.SetTypeId (s.Get ());
        *pimpl = GetPointer (factory.Create<SimulatorImpl> ());
      }
      {
        ObjectFactory factory;
        StringValue s;
        g_schedTypeImpl.GetValue (s);
        factory.SetTypeId (s.Get ());
        (*pimpl)->SetScheduler (factory);
      }

      //
      // Note: we call LogSetTimePrinter _after_ creating the implementation
      // object because the act of creation can trigger calls to the logging 
      // framework which would call the TimePrinter function which would call 
      // Simulator::Now which would call Simulator::GetImpl, and, thus, get us 
      // in an infinite recursion until the stack explodes.
      //
      LogSetTimePrinter (&TimePrinter);
      LogSetNodePrinter (&NodePrinter);
    }
    return *pimpl;
  }

  void
    Simulator::Destroy (void)
    {
      NS_LOG_FUNCTION_NOARGS ();

      SimulatorImpl **pimpl = PeekImpl (); 
      if (*pimpl == 0)
      {
        return;
      }
      /* Note: we have to call LogSetTimePrinter (0) below because if we do not do
       * this, and restart a simulation after this call to Destroy, (which is 
       * legal), Simulator::GetImpl will trigger again an infinite recursion until
       * the stack explodes.
       */
      LogSetTimePrinter (0);
      LogSetNodePrinter (0);
      (*pimpl)->Destroy ();
      (*pimpl)->Unref ();
      *pimpl = 0;
    }

  void
    Simulator::SetScheduler (ObjectFactory schedulerFactory)
    {
      NS_LOG_FUNCTION (schedulerFactory);
      GetImpl ()->SetScheduler (schedulerFactory);
    }

  bool 
    Simulator::IsFinished (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      return GetImpl ()->IsFinished ();
    }

  Time
    Simulator::Next (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      return GetImpl ()->Next ();
    }

  void 
    Simulator::Run (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      GetImpl ()->Run ();
    }

  void 
    Simulator::RunOneEvent (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      GetImpl ()->RunOneEvent ();
    }

  void 
    Simulator::Stop (void)
    {
      NS_LOG_LOGIC ("stop");
      GetImpl ()->Stop ();
    }

  void 
    Simulator::Stop (Time const &time)
    {
      NS_LOG_FUNCTION (time);
      GetImpl ()->Stop (time);
    }

  Time
    Simulator::Now (void)
    {
      /* Please, don't include any calls to logging macros in this function
       * or pay the price, that is, stack explosions.
       */
      return GetImpl ()->Now ();
    }

  Time
    Simulator::GetDelayLeft (const EventId &id)
    {
      NS_LOG_FUNCTION (&id);
      return GetImpl ()->GetDelayLeft (id);
    }

  EventId
    Simulator::Schedule (Time const &time, const Ptr<EventImpl> &ev)
    {
      NS_LOG_FUNCTION (time << ev);
      return DoSchedule (time, GetPointer (ev));
    }

  EventId
    Simulator::ScheduleNow (const Ptr<EventImpl> &ev)
    {
      NS_LOG_FUNCTION (ev);
      return DoScheduleNow (GetPointer (ev));
    }
  void
    Simulator::ScheduleWithContext (uint32_t context, const Time &time, EventImpl *impl)
    {
      return GetImpl ()->ScheduleWithContext (context, time, impl);
    }
  EventId
    Simulator::ScheduleDestroy (const Ptr<EventImpl> &ev)
    {
      NS_LOG_FUNCTION (ev);
      return DoScheduleDestroy (GetPointer (ev));
    }
  EventId 
    Simulator::DoSchedule (Time const &time, EventImpl *impl)
    {
      return GetImpl ()->Schedule (time, impl);
    }
  EventId 
    Simulator::DoScheduleNow (EventImpl *impl)
    {
      return GetImpl ()->ScheduleNow (impl);
    }
  EventId 
    Simulator::DoScheduleDestroy (EventImpl *impl)
    {
      return GetImpl ()->ScheduleDestroy (impl);
    }


  EventId
    Simulator::Schedule (Time const &time, void (*f)(void))
    {
      NS_LOG_FUNCTION (time << f);
      return DoSchedule (time, MakeEvent (f));
    }

  void
    Simulator::ScheduleWithContext (uint32_t context, Time const &time, void (*f)(void))
    {
      NS_LOG_FUNCTION (time << context << f);
      return ScheduleWithContext (context, time, MakeEvent (f));
    }

  EventId
    Simulator::ScheduleNow (void (*f)(void))
    {
      NS_LOG_FUNCTION (f);
      return DoScheduleNow (MakeEvent (f));
    }

  EventId
    Simulator::ScheduleDestroy (void (*f)(void))
    {
      NS_LOG_FUNCTION (f);
      return DoScheduleDestroy (MakeEvent (f));
    }

  void
    Simulator::Remove (const EventId &ev)
    {
      NS_LOG_FUNCTION (&ev);
      if (*PeekImpl () == 0)
      {
        return;
      }
      return GetImpl ()->Remove (ev);
    }

  void
    Simulator::Cancel (const EventId &ev)
    {
      NS_LOG_FUNCTION (&ev);
      if (*PeekImpl () == 0)
      {
        return;
      }
      return GetImpl ()->Cancel (ev);
    }

  bool 
    Simulator::IsExpired (const EventId &id)
    {
      NS_LOG_FUNCTION (&id);
      if (*PeekImpl () == 0)
      {
        return true;
      }
      return GetImpl ()->IsExpired (id);
    }

  Time Now (void)
  {
    NS_LOG_FUNCTION_NOARGS ();
    return Time (Simulator::Now ());
  }

  Time 
    Simulator::GetMaximumSimulationTime (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      return GetImpl ()->GetMaximumSimulationTime ();
    }

  uint32_t
    Simulator::GetContext (void)
    {
      return GetImpl ()->GetContext ();
    }

  uint32_t
    Simulator::GetSystemId (void)
    {
      NS_LOG_FUNCTION_NOARGS ();

      if (*PeekImpl () != 0)
      {
        return GetImpl ()->GetSystemId ();
      }
      else
      {
        return 0;
      }
    }

  void
    Simulator::SetImplementation (Ptr<SimulatorImpl> impl)
    {
      if (*PeekImpl () != 0)
      {
        NS_FATAL_ERROR ("It is not possible to set the implementation after calling any Simulator:: function. Call Simulator::SetImplementation earlier or after Simulator::Destroy.");
      }
      *PeekImpl () = GetPointer (impl);
      // Set the default scheduler
      ObjectFactory factory;
      StringValue s;
      g_schedTypeImpl.GetValue (s);
      factory.SetTypeId (s.Get ());
      impl->SetScheduler (factory);
      //
      // Note: we call LogSetTimePrinter _after_ creating the implementation
      // object because the act of creation can trigger calls to the logging 
      // framework which would call the TimePrinter function which would call 
      // Simulator::Now which would call Simulator::GetImpl, and, thus, get us 
      // in an infinite recursion until the stack explodes.
      //
      LogSetTimePrinter (&TimePrinter);
      LogSetNodePrinter (&NodePrinter);
    }
  Ptr<SimulatorImpl>
    Simulator::GetImplementation (void)
    {
      return GetImpl ();
    }
  //==================================for vPRKS=======================================
  std::vector<NodeSignalMap> Simulator::m_signalMaps;

  void Simulator::UpdateSignalMap (uint16_t nodeId, std::vector<SignalMapItem> localSignalMap)
  {
    for (std::vector<NodeSignalMap>::iterator it = m_signalMaps.begin (); it != m_signalMaps.end (); ++ it)
    {
      if ( it->nodeId == nodeId )
      {
        it->localSignalMap.clear ();
        for (std::vector<SignalMapItem>::iterator _it = localSignalMap.begin (); _it != localSignalMap.end (); ++ _it)
        {
          it->localSignalMap.push_back (*_it);
        }
        return;
      }
    }
    NodeSignalMap nodeSignalMap;
    nodeSignalMap.nodeId = nodeId;
    nodeSignalMap.localSignalMap = localSignalMap;
    m_signalMaps.push_back (nodeSignalMap);
  }

  std::vector<SignalMapItem> Simulator::GetSignalMap (uint16_t nodeId)
  {
    for (std::vector<NodeSignalMap>::iterator it = m_signalMaps.begin (); it != m_signalMaps.end (); ++ it)
    {
      if ( it->nodeId == nodeId )
      {
        return it->localSignalMap;
      }
    }
    //if no record found? (although this cannot happen)
    std::vector<SignalMapItem> vec; 
    return vec;
  }

  void Simulator::PrintSignalMaps(uint16_t nodeId)
  {
    std::cout<<"signal map for node: "<< nodeId << std::endl;
    for (std::vector<NodeSignalMap>::iterator it = m_signalMaps.begin (); it != m_signalMaps.end (); ++ it)
    {
      if (it->nodeId == nodeId)
      {
        for (std::vector<SignalMapItem>::iterator _it = it->localSignalMap.begin (); _it != it->localSignalMap.end (); ++ _it)
        {
          std::cout<<"from: "<< _it->from<<" to: "<< _it->to <<" atten: "<< _it->attenuation <<" angle: "<< _it->angle <<" exclusion region: "<< _it->exclusionRegion <<" x: "<< _it->x <<" y: "<< _it->y<<" edge: "<< _it->edge << std::endl;
        }

        return;
      }
    }
  }

  double Simulator::GetExclusionRegion (uint16_t sender, uint16_t receiver)
  {
    for (std::vector<NodeSignalMap>::iterator it = m_signalMaps.begin (); it != m_signalMaps.end (); ++ it)
    {
      if (it->nodeId == receiver) // Exclusion region information at the receiver side is the most accurate copy
      {
        for (std::vector<SignalMapItem>::iterator _it = it->localSignalMap.begin (); _it != it->localSignalMap.end (); ++ _it)
        {
          if ( _it->from == sender && _it->to == receiver)
          {
            return _it->exclusionRegion;
          }
        }
      }
    }
    //if not found.. not likely
    return DEFAULT_EXCLUSION_REGION;
  }


  std::vector<NodeStatus> Simulator::m_nodeStatusTable;
  void Simulator::UpdateNodeStatus (uint16_t nodeId, NodeStatus nodeStatus)
  {
    for (std::vector<NodeStatus>::iterator it = m_nodeStatusTable.begin (); it != m_nodeStatusTable.end (); ++ it)
    {
      if ( it->nodeId == nodeId)
      {
        it->angle = nodeStatus.angle;
        it->begin = nodeStatus.begin;
        it->end = nodeStatus.end;
        it->x = nodeStatus.x;
        it->y = nodeStatus.y;
        return;
      }
    }
    m_nodeStatusTable.push_back (nodeStatus);
  }

  void Simulator::GetSlotsForNode (uint16_t nodeId, uint16_t &begin, uint16_t &end)
  {
    for (std::vector<NodeStatus>::iterator it = m_nodeStatusTable.begin (); it != m_nodeStatusTable.end (); ++ it)
    {
      if ( nodeId == it->nodeId)
      {
        begin = it->begin;
        end = it->end;
        return;
      }
    }
    begin = 0;
    end = 0;
  }
  NodeStatus Simulator::GetNodeStatus (uint16_t nodeId)
  {
    for (std::vector<NodeStatus>::iterator it = m_nodeStatusTable.begin (); it != m_nodeStatusTable.end (); ++ it)
    {
      if ( nodeId == it->nodeId)
      {
        return *it;
      }
    }
    NodeStatus status;
    status.nodeId = nodeId;
    status.angle = 0;
    status.begin = 0;
    status.end = FRAME_LENGTH;
    status.x = 0;
    status.y = 0;
    return status;
  }

  void Simulator::PrintNodeStatus (uint16_t nodeId)
  {
    std::cout<<"print node status for node: "<< nodeId << std::endl;
    for (std::vector<NodeStatus>::iterator it = m_nodeStatusTable.begin (); it != m_nodeStatusTable.end (); ++ it)
    {
      if ( nodeId == it->nodeId)
      {
        std::cout<<"nodeId: "<<it->nodeId <<" angle: "<< it->angle <<" begin: "<< it->begin <<" end: "<< it->end <<" x: "<< it->x <<" y: "<< it->y << std::endl;
        return;
      }
    }
  }

  std::vector<NodeSendingStatus> Simulator::m_sendingNodes;
  void Simulator::RemoveSendingNode (NodeSendingStatus nodeSendingStatus)
  {
    for (std::vector<NodeSendingStatus>::iterator it = m_sendingNodes.begin (); it != m_sendingNodes.end () ; ++ it)
    {
      if ( it->nodeId == nodeSendingStatus.nodeId && it->sendingSlot == nodeSendingStatus.sendingSlot)
      {
        m_sendingNodes.erase (it);
        return;
      }
    }
  }
  void Simulator::AddSendingNode (NodeSendingStatus nodeSendingStatus)
  {
    for (std::vector<NodeSendingStatus>::iterator it = m_sendingNodes.begin (); it != m_sendingNodes.end () ; ++ it)
    {
      if ( it->nodeId == nodeSendingStatus.nodeId && it->sendingSlot == nodeSendingStatus.sendingSlot)
      {
        return;
      }
    }
    // ideally, the vector m_sendingNodes should not contain the node that we are going to add.
    m_sendingNodes.push_back (nodeSendingStatus);
  }
  //Every node should check this at each slot
  bool Simulator::IsReceiverInCurrentSlot (uint16_t nodeId, int16_t slot)
  {
    for (std::vector<NodeSendingStatus>::iterator it = m_sendingNodes.begin (); it != m_sendingNodes.end () ; ++ it)
    {
      if ( it->sendingSlot == slot)
      {
        std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap  (it->nodeId);
        for (std::vector<SignalMapItem>::iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
        {
          if ( _it->from == nodeId) // 
          {
            double rxPower = DEFAULT_POWER + TX_GAIN - _it->attenuation;
            if ( rxPower >= LINK_SELECTION_THRESHOLD)
            {
              return true;
            }
            else
            {
              return false;
            }
          }
        }
      }
    }
    return false;
  }
  bool Simulator::CheckIfTwoNodesConflict (uint16_t sender, uint16_t neighbor)
  {
    bool returnValue=false;
    std::vector<SignalMapItem> senderSignalMap = Simulator::GetSignalMap (sender);
    std::vector<uint16_t> receivers;
    //Find out who are the receivers first.
    for (std::vector<SignalMapItem>::iterator it = senderSignalMap.begin (); it != senderSignalMap.end (); ++ it)
    {
      double rxPower = DEFAULT_POWER + TX_GAIN - it->attenuation;
      if ( rxPower >= LINK_SELECTION_THRESHOLD)
      {
        receivers.push_back (it->from);
      }
    }
    //For each receiver, check if @neighbor is in its Exclusion Region
    for (std::vector<uint16_t>::iterator it = receivers.begin (); it != receivers.end (); ++ it)
    {
      std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap (*it); // get receiver signal map
      for (std::vector<SignalMapItem>::iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
      {
        if ( _it->from == sender && _it->to == *it)
        {
          double exclusionRegion = _it->exclusionRegion; // the link Exclusion region is found.
          // need a method to fetch nodes in exclusion region. 
          std::vector<uint16_t> nodesInExclusionRegion;
          Simulator::GetNodesInExclusionRegion ( *it, exclusionRegion, nodesInExclusionRegion); // find out who are in Exclusion Region
          if ( find (nodesInExclusionRegion.begin (), nodesInExclusionRegion.end (), neighbor) != nodesInExclusionRegion.end ())
          {
            return true;
          }
        }
      }
    }
    //Bi-directional Exclusion Region
    
    return returnValue;
  }
  void Simulator::GetNodesInExclusionRegion (uint16_t node, double exclusionRegion, std::vector<uint16_t> &vec)
  {
      std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap (node); // get receiver signal map
      for (std::vector<SignalMapItem>::iterator it = signalMap.begin (); it != signalMap.end (); ++ it)
      {
        double rxPower = DEFAULT_POWER + TX_GAIN - it->attenuation;
        if ( rxPower >= exclusionRegion)
        {
          vec.push_back (it->from);
        }
      }
  }

} // namespace ns3

