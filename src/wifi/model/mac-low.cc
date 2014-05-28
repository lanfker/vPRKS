/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/double.h"

#include "mac-low.h"
#include "wifi-phy.h"
#include "wifi-mac-trailer.h"
#include "qos-utils.h"
#include "edca-txop-n.h"
#include "matrix.h"
#include "payload-buffer.h"
#include "signal-map.h"
#include "ns3/mobility-model.h"
#include "yans-wifi-phy.h"
#include "double-regression.h"
#include "link-estimator.h"
#include "ns3/settings.h"
#include <cstdlib>
#include "exclusion-region-helper.h"
#include "controller.h"
#include <cmath>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("MacLow");


#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[mac=" << m_self << "] "


namespace ns3 {
  //const uint32_t DEFAULT_PACKET_LENGTH = 100;

  class SnrTag : public Tag
  {
    public:
      static TypeId GetTypeId (void);
      virtual TypeId GetInstanceTypeId (void) const;

      virtual uint32_t GetSerializedSize (void) const;
      virtual void Serialize (TagBuffer i) const;
      virtual void Deserialize (TagBuffer i);
      virtual void Print (std::ostream &os) const;

      void Set (double snr);
      double Get (void) const;
    private:
      double m_snr;
  };

  TypeId
    SnrTag::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::SnrTag")
        .SetParent<Tag> ()
        .AddConstructor<SnrTag> ()
        .AddAttribute ("Snr", "The snr of the last packet received",
            DoubleValue (0.0),
            MakeDoubleAccessor (&SnrTag::Get),
            MakeDoubleChecker<double> ())
        ;
      return tid;
    }
  TypeId
    SnrTag::GetInstanceTypeId (void) const
    {
      return GetTypeId ();
    }

  uint32_t
    SnrTag::GetSerializedSize (void) const
    {
      return sizeof (double);
    }
  void
    SnrTag::Serialize (TagBuffer i) const
    {
      i.WriteDouble (m_snr);
    }
  void
    SnrTag::Deserialize (TagBuffer i)
    {
      m_snr = i.ReadDouble ();
    }
  void
    SnrTag::Print (std::ostream &os) const
    {
      os << "Snr=" << m_snr;
    }
  void
    SnrTag::Set (double snr)
    {
      m_snr = snr;
    }
  double
    SnrTag::Get (void) const
    {
      return m_snr;
    }


  MacLowTransmissionListener::MacLowTransmissionListener ()
  {
  }
  MacLowTransmissionListener::~MacLowTransmissionListener ()
  {
  }
  void
    MacLowTransmissionListener::GotBlockAck (const CtrlBAckResponseHeader *blockAck,
        Mac48Address source)
    {
    }
  void
    MacLowTransmissionListener::MissedBlockAck (void)
    {
    }
  MacLowDcfListener::MacLowDcfListener ()
  {
  }
  MacLowDcfListener::~MacLowDcfListener ()
  {
  }

  MacLowBlockAckEventListener::MacLowBlockAckEventListener ()
  {
  }
  MacLowBlockAckEventListener::~MacLowBlockAckEventListener ()
  {
  }

  MacLowTransmissionParameters::MacLowTransmissionParameters ()
    : m_nextSize (0),
    m_waitAck (ACK_NONE),
    m_sendRts (false),
    m_overrideDurationId (Seconds (0))
  {
  }
  void
    MacLowTransmissionParameters::EnableNextData (uint32_t size)
    {
      m_nextSize = size;
    }
  void
    MacLowTransmissionParameters::DisableNextData (void)
    {
      m_nextSize = 0;
    }
  void
    MacLowTransmissionParameters::EnableOverrideDurationId (Time durationId)
    {
      m_overrideDurationId = durationId;
    }
  void
    MacLowTransmissionParameters::DisableOverrideDurationId (void)
    {
      m_overrideDurationId = Seconds (0);
    }
  void
    MacLowTransmissionParameters::EnableSuperFastAck (void)
    {
      m_waitAck = ACK_SUPER_FAST;
    }
  void
    MacLowTransmissionParameters::EnableBasicBlockAck (void)
    {
      m_waitAck = BLOCK_ACK_BASIC;
    }
  void
    MacLowTransmissionParameters::EnableCompressedBlockAck (void)
    {
      m_waitAck = BLOCK_ACK_COMPRESSED;
    }
  void
    MacLowTransmissionParameters::EnableMultiTidBlockAck (void)
    {
      m_waitAck = BLOCK_ACK_MULTI_TID;
    }
  void
    MacLowTransmissionParameters::EnableFastAck (void)
    {
      m_waitAck = ACK_FAST;
    }
  void
    MacLowTransmissionParameters::EnableAck (void)
    {
      m_waitAck = ACK_NORMAL;
    }
  void
    MacLowTransmissionParameters::DisableAck (void)
    {
      m_waitAck = ACK_NONE;
    }
  void
    MacLowTransmissionParameters::EnableRts (void)
    {
      m_sendRts = true;
    }
  void
    MacLowTransmissionParameters::DisableRts (void)
    {
      m_sendRts = false;
    }
  bool
    MacLowTransmissionParameters::MustWaitAck (void) const
    {
      return (m_waitAck != ACK_NONE);
    }
  bool
    MacLowTransmissionParameters::MustWaitNormalAck (void) const
    {
      return (m_waitAck == ACK_NORMAL);
    }
  bool
    MacLowTransmissionParameters::MustWaitFastAck (void) const
    {
      return (m_waitAck == ACK_FAST);
    }
  bool
    MacLowTransmissionParameters::MustWaitSuperFastAck (void) const
    {
      return (m_waitAck == ACK_SUPER_FAST);
    }
  bool
    MacLowTransmissionParameters::MustWaitBasicBlockAck (void) const
    {
      return (m_waitAck == BLOCK_ACK_BASIC) ? true : false;
    }
  bool
    MacLowTransmissionParameters::MustWaitCompressedBlockAck (void) const
    {
      return (m_waitAck == BLOCK_ACK_COMPRESSED) ? true : false;
    }
  bool
    MacLowTransmissionParameters::MustWaitMultiTidBlockAck (void) const
    {
      return (m_waitAck == BLOCK_ACK_MULTI_TID) ? true : false;
    }
  bool
    MacLowTransmissionParameters::MustSendRts (void) const
    {
      return m_sendRts;
    }
  bool
    MacLowTransmissionParameters::HasDurationId (void) const
    {
      return (m_overrideDurationId != Seconds (0));
    }
  Time
    MacLowTransmissionParameters::GetDurationId (void) const
    {
      NS_ASSERT (m_overrideDurationId != Seconds (0));
      return m_overrideDurationId;
    }
  bool
    MacLowTransmissionParameters::HasNextPacket (void) const
    {
      return (m_nextSize != 0);
    }
  uint32_t
    MacLowTransmissionParameters::GetNextPacketSize (void) const
    {
      NS_ASSERT (HasNextPacket ());
      return m_nextSize;
    }

  std::ostream &operator << (std::ostream &os, const MacLowTransmissionParameters &params)
  {
    os << "["
      << "send rts=" << params.m_sendRts << ", "
      << "next size=" << params.m_nextSize << ", "
      << "dur=" << params.m_overrideDurationId << ", "
      << "ack=";
    switch (params.m_waitAck)
    {
      case MacLowTransmissionParameters::ACK_NONE:
        os << "none";
        break;
      case MacLowTransmissionParameters::ACK_NORMAL:
        os << "normal";
        break;
      case MacLowTransmissionParameters::ACK_FAST:
        os << "fast";
        break;
      case MacLowTransmissionParameters::ACK_SUPER_FAST:
        os << "super-fast";
        break;
      case MacLowTransmissionParameters::BLOCK_ACK_BASIC:
        os << "basic-block-ack";
        break;
      case MacLowTransmissionParameters::BLOCK_ACK_COMPRESSED:
        os << "compressed-block-ack";
        break;
      case MacLowTransmissionParameters::BLOCK_ACK_MULTI_TID:
        os << "multi-tid-block-ack";
        break;
    }
    os << "]";
    return os;
  }


  /***************************************************************
   *         Listener for PHY events. Forwards to MacLow
   ***************************************************************/


  class PhyMacLowListener : public ns3::WifiPhyListener
  {
    public:
      PhyMacLowListener (ns3::MacLow *macLow)
        : m_macLow (macLow)
      {
      }
      virtual ~PhyMacLowListener ()
      {
      }
      virtual void NotifyRxStart (Time duration)
      {
      }
      virtual void NotifyRxEndOk (void)
      {
      }
      virtual void NotifyRxEndError (void)
      {
      }
      virtual void NotifyTxStart (Time duration)
      {
      }
      virtual void NotifyMaybeCcaBusyStart (Time duration)
      {
      }
      virtual void NotifySwitchingStart (Time duration)
      {
        m_macLow->NotifySwitchingStartNow (duration);
      }
    private:
      ns3::MacLow *m_macLow;
  };


  MacLow::MacLow ()
    : m_normalAckTimeoutEvent (),
    m_fastAckTimeoutEvent (),
    m_superFastAckTimeoutEvent (),
    m_fastAckFailedTimeoutEvent (),
    m_blockAckTimeoutEvent (),
    m_ctsTimeoutEvent (),
    m_sendCtsEvent (),
    m_sendAckEvent (),
    m_sendDataEvent (),
    m_waitSifsEvent (),
    m_currentPacket (0),
    m_listener (0)
  {
    NS_LOG_FUNCTION (this);
    m_lastNavDuration = Seconds (0);
    m_lastNavStart = Seconds (0);
    m_promisc = false;
    m_sequenceNumber = 0;
    m_currentSlot = 0;
    m_angle = 0;
    m_nextSendingSlot = 0;
    // Update slot number when it should be incremented
    Simulator::Schedule (MicroSeconds (SLOT_LENGTH), &MacLow::GetCurrentSlot, this);
    //Simulator::Schedule (Seconds (50), &MacLow::PrintAddress, this);
    //Simulator::Schedule (Seconds (START_PROCESS_TIME), &MacLow::CalculateSchedule, this);
  }

  MacLow::~MacLow ()
  {
    NS_LOG_FUNCTION (this);
  }

  void
    MacLow::SetupPhyMacLowListener (Ptr<WifiPhy> phy)
    {
      m_phyMacLowListener = new PhyMacLowListener (this);
      phy->RegisterListener (m_phyMacLowListener);
    }


  void
    MacLow::DoDispose (void)
    {
      NS_LOG_FUNCTION (this);
      m_normalAckTimeoutEvent.Cancel ();
      m_fastAckTimeoutEvent.Cancel ();
      m_superFastAckTimeoutEvent.Cancel ();
      m_fastAckFailedTimeoutEvent.Cancel ();
      m_blockAckTimeoutEvent.Cancel ();
      m_ctsTimeoutEvent.Cancel ();
      m_sendCtsEvent.Cancel ();
      m_sendAckEvent.Cancel ();
      m_sendDataEvent.Cancel ();
      m_waitSifsEvent.Cancel ();
      m_phy = 0;
      m_stationManager = 0;
      delete m_phyMacLowListener;
      m_phyMacLowListener = 0;
    }

  void
    MacLow::CancelAllEvents (void)
    {
      NS_LOG_FUNCTION (this);
      bool oneRunning = false;
      if (m_normalAckTimeoutEvent.IsRunning ())
      {
        m_normalAckTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_fastAckTimeoutEvent.IsRunning ())
      {
        m_fastAckTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_superFastAckTimeoutEvent.IsRunning ())
      {
        m_superFastAckTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_fastAckFailedTimeoutEvent.IsRunning ())
      {
        m_fastAckFailedTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_blockAckTimeoutEvent.IsRunning ())
      {
        m_blockAckTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_ctsTimeoutEvent.IsRunning ())
      {
        m_ctsTimeoutEvent.Cancel ();
        oneRunning = true;
      }
      if (m_sendCtsEvent.IsRunning ())
      {
        m_sendCtsEvent.Cancel ();
        oneRunning = true;
      }
      if (m_sendAckEvent.IsRunning ())
      {
        m_sendAckEvent.Cancel ();
        oneRunning = true;
      }
      if (m_sendDataEvent.IsRunning ())
      {
        m_sendDataEvent.Cancel ();
        oneRunning = true;
      }
      if (m_waitSifsEvent.IsRunning ())
      {
        m_waitSifsEvent.Cancel ();
        oneRunning = true;
      }
      if (oneRunning && m_listener != 0)
      {
        m_listener->Cancel ();
        m_listener = 0;
      }
    }

  void
    MacLow::SetPhy (Ptr<WifiPhy> phy)
    {
      m_phy = phy;
      m_phy->SetReceiveOkCallback (MakeCallback (&MacLow::ReceiveOk, this));
      m_phy->SetReceiveErrorCallback (MakeCallback (&MacLow::ReceiveError, this));
      SetupPhyMacLowListener (phy);
      m_phy->GetObject<YansWifiPhy> () -> SetAddress (m_self);

      /*
      for (std::vector<RoadMapEdge>::iterator it = EdgeXmlParser::m_mapEdges.begin (); it != EdgeXmlParser::m_mapEdges.end (); ++ it)
      {
        std::cout<<"edge: "<< it->edgeId <<" from ("<<it->from.xCoordinate <<","<<it->from.yCoordinate <<") to ("<< it->to.xCoordinate <<","<<it->to.yCoordinate <<")"<< std::endl;
      }
      */
    }
  void
    MacLow::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> manager)
    {
      m_stationManager = manager;
    }

  void
    MacLow::SetAddress (Mac48Address ad)
    {
      //std::cout<<" m_self: "<< m_self <<" ad: "<< ad << std::endl;
      m_self = ad;
      if ( ad.GetNodeId () == 0)
      {
        m_setChannelEvent.Cancel ();
        m_phy = 0;
      }
    }
  void
    MacLow::SetAckTimeout (Time ackTimeout)
    {
      m_ackTimeout = ackTimeout;
    }
  void
    MacLow::SetBasicBlockAckTimeout (Time blockAckTimeout)
    {
      m_basicBlockAckTimeout = blockAckTimeout;
    }
  void
    MacLow::SetCompressedBlockAckTimeout (Time blockAckTimeout)
    {
      m_compressedBlockAckTimeout = blockAckTimeout;
    }
  void
    MacLow::SetCtsTimeout (Time ctsTimeout)
    {
      m_ctsTimeout = ctsTimeout;
    }
  void
    MacLow::SetSifs (Time sifs)
    {
      m_sifs = sifs;
    }
  void
    MacLow::SetSlotTime (Time slotTime)
    {
      m_slotTime = slotTime;
    }
  void
    MacLow::SetPifs (Time pifs)
    {
      m_pifs = pifs;
    }
  void
    MacLow::SetBssid (Mac48Address bssid)
    {
      m_bssid = bssid;
    }
  void
    MacLow::SetPromisc (void)
    {
      m_promisc = true;
    }
  Mac48Address
    MacLow::GetAddress (void) const
    {
      return m_self;
    }
  Time
    MacLow::GetAckTimeout (void) const
    {
      return m_ackTimeout;
    }
  Time
    MacLow::GetBasicBlockAckTimeout () const
    {
      return m_basicBlockAckTimeout;
    }
  Time
    MacLow::GetCompressedBlockAckTimeout () const
    {
      return m_compressedBlockAckTimeout;
    }
  Time
    MacLow::GetCtsTimeout (void) const
    {
      return m_ctsTimeout;
    }
  Time
    MacLow::GetSifs (void) const
    {
      return m_sifs;
    }
  Time
    MacLow::GetSlotTime (void) const
    {
      return m_slotTime;
    }
  Time
    MacLow::GetPifs (void) const
    {
      return m_pifs;
    }
  Mac48Address
    MacLow::GetBssid (void) const
    {
      return m_bssid;
    }

  void
    MacLow::SetRxCallback (Callback<void,Ptr<Packet>,const WifiMacHeader *> callback)
    {
      m_rxCallback = callback;
    }
  void
    MacLow::RegisterDcfListener (MacLowDcfListener *listener)
    {
      m_dcfListeners.push_back (listener);
    }


  void
    MacLow::StartTransmission (Ptr<const Packet> packet,
        const WifiMacHeader* hdr,
        MacLowTransmissionParameters params,
        MacLowTransmissionListener *listener)
    {
      NS_LOG_FUNCTION (this << packet << hdr << params << listener);
      /* m_currentPacket is not NULL because someone started
       * a transmission and was interrupted before one of:
       *   - ctsTimeout
       *   - sendDataAfterCTS
       * expired. This means that one of these timers is still
       * running. They are all cancelled below anyway by the
       * call to CancelAllEvents (because of at least one
       * of these two timer) which will trigger a call to the
       * previous listener's cancel method.
       *
       * This typically happens because the high-priority
       * QapScheduler has taken access to the channel from
       * one of the Edca of the QAP.
       */
      m_currentPacket = packet->Copy ();
      m_currentHdr = *hdr;
      CancelAllEvents ();
      m_listener = listener;
      m_txParams = params;


      //NS_ASSERT (m_phy->IsStateIdle ());

      NS_LOG_DEBUG ("startTx size=" << GetSize (m_currentPacket, &m_currentHdr) <<
          ", to=" << m_currentHdr.GetAddr1 () << ", listener=" << m_listener);

      if (m_txParams.MustSendRts ())
      {
        SendRtsForPacket ();
      }
      else
      {
        if ( Simulator::Now () < Seconds (START_PROCESS_TIME))
          SendDataPacket ();
      }

      /* When this method completes, we have taken ownership of the medium. */
      //NS_ASSERT (m_phy->IsStateTx ());
    }

  void
    MacLow::ReceiveError (Ptr<const Packet> packet, double rxSnr)
    {
      NS_LOG_FUNCTION (this << packet << rxSnr);
      NS_LOG_DEBUG ("rx failed ");
      if (m_txParams.MustWaitFastAck ())
      {
        NS_ASSERT (m_fastAckFailedTimeoutEvent.IsExpired ());
        m_fastAckFailedTimeoutEvent = Simulator::Schedule (GetSifs (),
            &MacLow::FastAckFailedTimeout, this);
      }
      return;
    }

  void
    MacLow::NotifySwitchingStartNow (Time duration)
    {
      NS_LOG_DEBUG ("switching channel. Cancelling MAC pending events");
      m_stationManager->Reset ();
      CancelAllEvents ();
      if (m_navCounterResetCtsMissed.IsRunning ())
      {
        m_navCounterResetCtsMissed.Cancel ();
      }
      m_lastNavStart = Simulator::Now ();
      m_lastNavDuration = Seconds (0);
      m_currentPacket = 0;
      m_listener = 0;
    }

  void
    MacLow::ReceiveOk (Ptr<Packet> packet, double rxSnr, WifiMode txMode, WifiPreamble preamble)
    {
      NS_LOG_FUNCTION (this << packet << rxSnr << txMode << preamble);
      /* A packet is received from the PHY.
       * When we have handled this packet,
       * we handle any packet present in the
       * packet queue.
       */
      WifiMacHeader hdr;
      packet->RemoveHeader (hdr);

      uint16_t sender = hdr.GetAddr2 ().GetNodeId ();
      uint16_t receiver = m_self.GetNodeId ();
      Ptr<MobilityModel> selfMobilityModel = m_phy->GetObject<YansWifiPhy> () -> GetMobility () -> GetObject <MobilityModel> ();
      Vector position = selfMobilityModel -> GetPosition ();
      //-----------------------------------------------------
      uint8_t* temp = new uint8_t[DEFAULT_PACKET_LENGTH];
      uint32_t copyBytes = packet->CopyData (temp, DEFAULT_PACKET_LENGTH);
      PayloadBuffer buff = PayloadBuffer (temp);
      //For TxPower, The Actual Dbm Value Is Updated At The Physical Layer.
      double txPower = buff.ReadDouble (); // with txGain added
      double rxPower = buff.ReadDouble (); // with rxGain added
      double angle = buff.ReadDouble ();
      double x = buff.ReadDouble ();
      double y = buff.ReadDouble ();
      m_txPower = txPower; //dBm
      uint32_t edgeLength = buff.ReadU8 ();
      std::string edge = buff.ReadString (edgeLength);
      m_signalMap.UpdateVehicleStatus (sender, angle, x, y, edge);
      //std::cout<<hdr.GetAddr2 ().GetNodeId ()<<" x: "<< x <<" y: "<< y << std::endl;
      //std::cout<<"sender: "<< hdr.GetAddr2 ().GetNodeId () << " receiver: "<< m_self.GetNodeId () <<" dist: "<< sqrt (pow (m_positionX - x,2) + pow (m_positionY - y, 2)) <<" atten: "<< txPower - rxPower <<std::endl;
      /*
      if ( Simulator::Now () > Seconds (START_PROCESS_TIME ) && (m_phy->GetChannelNumber () == DATA_CHANNEL ))
      {
        std::cout<<m_self.GetNodeId () << " is receiving" << std::endl;
      }
      else if ( Simulator::Now () > Seconds (START_PROCESS_TIME ) && (m_phy->GetChannelNumber () == CONTROL_CHANNEL))
      {
        std::cout<<m_self.GetNodeId () << " in control channel, receiving with rxPower: "<< rxPower<< " txPower: "<< txPower << std::endl;
      }
      */


      //============================Read shared Density info==============================
      /*
      uint32_t densityCount = buff.ReadU8 ();
      for (uint32_t i = 0; i < densityCount; ++ i)
      {
        uint32_t edgeIdSize = buff.ReadU8 ();
        std::string edgeId = buff.ReadString (edgeIdSize);
        double density = buff.ReadDouble ();
      }
      */


      /*
         ObservationItem obsItem;

         obsItem.senderX = x;
         obsItem.senderY = y;
         obsItem.receiverX =  position.x;
         obsItem.receiverY =  position.y;
         obsItem.averageAttenuation = txPower - rxPower;
         obsItem.timeStamp = Simulator::Now ();
         m_observation.AppendObservation (hdr.GetAddr2 ().GetNodeId (), m_self.GetNodeId (), obsItem);
         */

      //==============================Signal Map Sample==============================================
      SignalMapItem signalMapItem;
      signalMapItem.selfx = m_positionX;
      signalMapItem.selfy = m_positionY;
      signalMapItem.to = m_self.GetNodeId ();
      signalMapItem.from = hdr.GetAddr2 ().GetNodeId ();
      signalMapItem.attenuation = txPower - rxPower;
      signalMapItem.timeStamp = Simulator::Now ();
      signalMapItem.angle = angle;
      signalMapItem.x = x;
      signalMapItem.y = y;
      signalMapItem.edge = edge;
      m_signalMap.AddOrUpdate (signalMapItem);
      // Here, we also fake signal map for now. Will update later.
      Simulator::UpdateSignalMap (m_self.GetNodeId (), m_signalMap.GetSignalMap ());
      //Simulator::PrintSignalMaps(m_self.GetNodeId ());
      //============================================================================================


      bool isPrevNavZero = IsNavZero ();
      NS_LOG_DEBUG ("duration/id=" << hdr.GetDuration ());
      NotifyNav (hdr, txMode, preamble);
      if (hdr.IsRts ())
      {
        /* see section 9.2.5.7 802.11-1999
         * A STA that is addressed by an RTS frame shall transmit a CTS frame after a SIFS
         * period if the NAV at the STA receiving the RTS frame indicates that the medium is
         * idle. If the NAV at the STA receiving the RTS indicates the medium is not idle,
         * that STA shall not respond to the RTS frame.
         */
        if (isPrevNavZero
            && hdr.GetAddr1 () == m_self)
        {
          NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", schedule CTS");
          NS_ASSERT (m_sendCtsEvent.IsExpired ());
          m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
              rxSnr, txMode);
          m_sendCtsEvent = Simulator::Schedule (GetSifs (),
              &MacLow::SendCtsAfterRts, this,
              hdr.GetAddr2 (),
              hdr.GetDuration (),
              txMode,
              rxSnr);
        }
        else
        {
          NS_LOG_DEBUG ("rx RTS from=" << hdr.GetAddr2 () << ", cannot schedule CTS");
        }
      }
      else if (hdr.IsCts ()
          && hdr.GetAddr1 () == m_self
          && m_ctsTimeoutEvent.IsRunning ()
          && m_currentPacket != 0)
      {
        NS_LOG_DEBUG ("receive cts from=" << m_currentHdr.GetAddr1 ());
        SnrTag tag;
        packet->RemovePacketTag (tag);
        m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
            rxSnr, txMode);
        m_stationManager->ReportRtsOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
            rxSnr, txMode, tag.Get ());

        m_ctsTimeoutEvent.Cancel ();
        NotifyCtsTimeoutResetNow ();
        m_listener->GotCts (rxSnr, txMode);
        NS_ASSERT (m_sendDataEvent.IsExpired ());
        m_sendDataEvent = Simulator::Schedule (GetSifs (),
            &MacLow::SendDataAfterCts, this,
            hdr.GetAddr1 (),
            hdr.GetDuration (),
            txMode);
      }
      else if (hdr.IsAck ()
          && hdr.GetAddr1 () == m_self
          && (m_normalAckTimeoutEvent.IsRunning ()
            || m_fastAckTimeoutEvent.IsRunning ()
            || m_superFastAckTimeoutEvent.IsRunning ())
          && m_txParams.MustWaitAck ())
      {
        NS_LOG_DEBUG ("receive ack from=" << m_currentHdr.GetAddr1 ());
        SnrTag tag;
        packet->RemovePacketTag (tag);
        m_stationManager->ReportRxOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
            rxSnr, txMode);
        m_stationManager->ReportDataOk (m_currentHdr.GetAddr1 (), &m_currentHdr,
            rxSnr, txMode, tag.Get ());
        bool gotAck = false;
        if (m_txParams.MustWaitNormalAck ()
            && m_normalAckTimeoutEvent.IsRunning ())
        {
          m_normalAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
        if (m_txParams.MustWaitFastAck ()
            && m_fastAckTimeoutEvent.IsRunning ())
        {
          m_fastAckTimeoutEvent.Cancel ();
          NotifyAckTimeoutResetNow ();
          gotAck = true;
        }
        if (gotAck)
        {
          m_listener->GotAck (rxSnr, txMode);
        }
        if (m_txParams.HasNextPacket ())
        {
          m_waitSifsEvent = Simulator::Schedule (GetSifs (),
              &MacLow::WaitSifsAfterEndTx, this);
        }
      }
      else if (hdr.IsBlockAck () && hdr.GetAddr1 () == m_self
          && (m_txParams.MustWaitBasicBlockAck () || m_txParams.MustWaitCompressedBlockAck ())
          && m_blockAckTimeoutEvent.IsRunning ())
      {
        NS_LOG_DEBUG ("got block ack from " << hdr.GetAddr2 ());
        CtrlBAckResponseHeader blockAck;
        packet->RemoveHeader (blockAck);
        m_blockAckTimeoutEvent.Cancel ();
        m_listener->GotBlockAck (&blockAck, hdr.GetAddr2 ());
      }
      else if (hdr.IsBlockAckReq () && hdr.GetAddr1 () == m_self)
      {
        CtrlBAckRequestHeader blockAckReq;
        packet->RemoveHeader (blockAckReq);
        if (!blockAckReq.IsMultiTid ())
        {
          uint8_t tid = blockAckReq.GetTidInfo ();
          AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), tid));
          if (it != m_bAckAgreements.end ())
          {
            //Update block ack cache
            BlockAckCachesI i = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), tid));
            NS_ASSERT (i != m_bAckCaches.end ());
            (*i).second.UpdateWithBlockAckReq (blockAckReq.GetStartingSequence ());

            NS_ASSERT (m_sendAckEvent.IsExpired ());
            /* See section 11.5.3 in IEEE802.11 for mean of this timer */
            ResetBlockAckInactivityTimerIfNeeded (it->second.first);
            if ((*it).second.first.IsImmediateBlockAck ())
            {
              NS_LOG_DEBUG ("rx blockAckRequest/sendImmediateBlockAck from=" << hdr.GetAddr2 ());
              m_sendAckEvent = Simulator::Schedule (GetSifs (),
                  &MacLow::SendBlockAckAfterBlockAckRequest, this,
                  blockAckReq,
                  hdr.GetAddr2 (),
                  hdr.GetDuration (),
                  txMode);
            }
            else
            {
              NS_FATAL_ERROR ("Delayed block ack not supported.");
            }
          }
          else
          {
            NS_LOG_DEBUG ("There's not a valid agreement for this block ack request.");
          }
        }
        else
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      }
      else if (hdr.IsCtl ())
      {
        NS_LOG_DEBUG ("rx drop " << hdr.GetTypeString ());
      }
      else if (hdr.GetAddr1 () == m_self)
      {
        m_stationManager->ReportRxOk (hdr.GetAddr2 (), &hdr,
            rxSnr, txMode);

        if (hdr.IsQosData () && StoreMpduIfNeeded (packet, hdr))
        {
          /* From section 9.10.4 in IEEE802.11:
             Upon the receipt of a QoS data frame from the originator for which
             the Block Ack agreement exists, the recipient shall buffer the MSDU
             regardless of the value of the Ack Policy subfield within the
             QoS Control field of the QoS data frame. */
          if (hdr.IsQosAck ())
          {
            AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
            RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequence (),
                hdr.GetAddr2 (), hdr.GetQosTid ());
            RxCompleteBufferedPacketsUntilFirstLost (hdr.GetAddr2 (), hdr.GetQosTid ());
            NS_ASSERT (m_sendAckEvent.IsExpired ());
            m_sendAckEvent = Simulator::Schedule (GetSifs (),
                &MacLow::SendAckAfterData, this,
                hdr.GetAddr2 (),
                hdr.GetDuration (),
                txMode,
                rxSnr);
          }
          else if (hdr.IsQosBlockAck ())
          {
            AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
            /* See section 11.5.3 in IEEE802.11 for mean of this timer */
            ResetBlockAckInactivityTimerIfNeeded (it->second.first);
          }
          return;
        }
        else if (hdr.IsQosData () && hdr.IsQosBlockAck ())
        {
          /* This happens if a packet with ack policy Block Ack is received and a block ack
             agreement for that packet doesn't exist.

             From section 11.5.3 in IEEE802.11e:
             When a recipient does not have an active Block ack for a TID, but receives
             data MPDUs with the Ack Policy subfield set to Block Ack, it shall discard
             them and shall send a DELBA frame using the normal access
             mechanisms. */
          AcIndex ac = QosUtilsMapTidToAc (hdr.GetQosTid ());
          m_edcaListeners[ac]->BlockAckInactivityTimeout (hdr.GetAddr2 (), hdr.GetQosTid ());
          return;
        }
        else if (hdr.IsQosData () && hdr.IsQosNoAck ())
        {
          NS_LOG_DEBUG ("rx unicast/noAck from=" << hdr.GetAddr2 ());
        }
        else if (hdr.IsData () || hdr.IsMgt ())
        {
          NS_LOG_DEBUG ("rx unicast/sendAck from=" << hdr.GetAddr2 ());
          NS_ASSERT (m_sendAckEvent.IsExpired ());
          m_sendAckEvent = Simulator::Schedule (GetSifs (),
              &MacLow::SendAckAfterData, this,
              hdr.GetAddr2 (),
              hdr.GetDuration (),
              txMode,
              rxSnr);
        }
        goto rxPacket;
      }
      else if (hdr.GetAddr1 ().IsGroup ())
      { 

        if ( Simulator::Now () > Seconds (START_PROCESS_TIME) && m_phy->GetChannelNumber () == DATA_CHANNEL)
          std::cout<<Simulator::Now () <<" node: "<< m_self.GetNodeId () << " received a packet from: " << hdr.GetAddr2 ().GetNodeId () << std::endl;
        int64_t receivedNextSendingSlot = buff.ReadU64 ();
        //std::cout<<m_self.GetNodeId () <<" received sending slot: " << receivedNextSendingSlot <<" from "<< hdr.GetAddr2 ().GetNodeId ()<< std::endl;

        UpdateSendingStatus (hdr.GetAddr2().GetNodeId (), receivedNextSendingSlot);
        uint8_t size = buff.ReadU8 ();
        for (uint8_t i = 0; i < size; ++ i)
        {
          uint16_t nodeId = buff.ReadU16 ();
          int64_t sendingSlot = (int64_t)buff.ReadU64 ();
          UpdateSendingStatus (nodeId, sendingSlot);
          //std::cout<<m_self.GetNodeId () <<" node: "<< nodeId << " slot: "<< sendingSlot <<" from: "<< hdr.GetAddr2 ().GetNodeId () << std::endl;
        }
        size = buff.ReadU8 (); // exclusion region count
        for (uint8_t i = 0; i < size; ++ i)
        {
          LinkExclusionRegion item;
          item.sender = buff.ReadU16 ();
          item.receiver = buff.ReadU16 ();
          item.currentExclusionRegion = buff.ReadDouble ();
          item.version = buff.ReadU8 ();
          item.senderX = buff.ReadDouble ();
          item.senderY = buff.ReadDouble ();
          item.receiverX = buff.ReadDouble ();
          item.receiverY = buff.ReadDouble ();
          //std::cout<<m_self.GetNodeId () <<" sender: " << item.sender <<" receiver: "<< item.receiver <<" exclusion: "<< item.currentExclusionRegion<<" version: "<< (uint32_t)item.version <<" self: "<<m_self.GetNodeId () <<" senderx: "<< item.senderX <<" senderY: "<< item.senderY << " receiverx: "<< item.receiverX <<" receivery: "<< item.receiverY<< std::endl;
          m_exclusionRegionHelper.AddOrUpdateExclusionRegion (item);
        }
        //m_signalMap.SortAccordingToInComingAttenuation ();
        /*
        if ( m_self.GetNodeId () == 30 )
        {
          std::cout<<Simulator::Now () <<" "<<m_self.GetNodeId () <<" m_position.x: "<< m_positionX <<" m_position.y: "<< m_positionY << std::endl;
          m_signalMap.PrintSignalMap (m_self.GetNodeId ());
        }
        */
        delete [] temp;

        //---------------------------------------------
        //

        //====BROADCAST MESSAGE================
        //std::cout<<" which channel: "<< m_phy->GetChannelNumber () << std::endl;

        if ( m_phy->GetChannelNumber () == DATA_CHANNEL)
        {
          LinkEstimator linkEstimator;

          double linkDistance = Simulator::GetDistanceBetweenTwoNodes (sender, receiver);

          if ( linkDistance <= MAX_LINK_DISTANCE)
          {
            m_linkEstimator.AddSequenceNumber (hdr.GetSequenceNumber (), sender, receiver, Simulator::Now ());
            bool pdrUpdated = m_linkEstimator.IsPdrUpdated (sender, receiver, LINKE_ESTIMATOR_WINDOW_SIZE); // 20 as window size
            //std::cout<<" is pdr updated: "<< pdrUpdated << std::endl;
            if ( pdrUpdated == true)
            {
              LinkEstimationItem _item = m_linkEstimator.GetLinkEstimationItem (sender, receiver);
              if ( _item.sender != 0 && _item.receiver != 0)
              {
                LinkExclusionRegion linkExclusionRegionRecord = m_exclusionRegionHelper.GetExclusionRegionRecord (sender, receiver);
                linkExclusionRegionRecord.senderX = x;
                linkExclusionRegionRecord.senderY = y;
                linkExclusionRegionRecord.receiverX = m_positionX;
                linkExclusionRegionRecord.receiverY = m_positionY;
                if ( linkExclusionRegionRecord.sender == 0 && linkExclusionRegionRecord.receiver == 0)
                {
                  linkExclusionRegionRecord.sender = sender;
                  linkExclusionRegionRecord.receiver = receiver;
                  linkExclusionRegionRecord.distance = linkDistance;
                  linkExclusionRegionRecord.version = 0;
                  linkExclusionRegionRecord.currentExclusionRegion = DEFAULT_EXCLUSION_REGION_WATT;
                }
                //Check distance, if displacement is greater than average displacement, use estimated exclusion region value.
                double displacement = linkDistance - linkExclusionRegionRecord.distance;
                linkExclusionRegionRecord.distance = linkDistance;
                double exclusionRegionUpdated = false;
                if ( _item.estimationCount > 5 && displacement > 10) //use estimation
                {

                  DoubleRegression doubleRegression;
                  double estimatedExclusionRegion = doubleRegression.ParameterEstimation (sender, receiver, x, y, m_positionX, m_positionY, m_exclusionRegionHelper);
                  if ( estimatedExclusionRegion != 0) // estimation success
                  {
                    exclusionRegionUpdated = true;
                    linkExclusionRegionRecord.currentExclusionRegion = estimatedExclusionRegion;
                  }
                  std::cout<<m_self.GetNodeId ()<<" estimated exclusion region: "<< estimatedExclusionRegion << std::endl;
                }
                m_exclusionRegionHelper.AddOrUpdateExclusionRegion (linkExclusionRegionRecord);
                if ( exclusionRegionUpdated == false) //estimation failure or do not need to estimate. use this part of the logic.
                {
                  bool conditionTwoMeet = false;
                  double deltaInterferenceDb = m_minimumVarianceController.GetDeltaInterference (DESIRED_PDR, _item.ewmaPdr, _item.instantPdr, conditionTwoMeet);
                  std::cout<<Simulator::Now () <<" "<<m_self.GetNodeId () <<" "<< Simulator::Now () << " deltaInterferenceDb: "<< deltaInterferenceDb<<" ewmapdr: "<< _item.ewmaPdr <<" instantpdr: "<< _item.instantPdr <<" link length: "<<linkDistance << " estimationCount: "<< _item.estimationCount<< std::endl;

                  std::vector<SignalMapItem> signalMapVec = Simulator::GetSignalMap (m_self.GetNodeId ());
                  SignalMap signalMap = SignalMap (signalMapVec);
                  double interferenceW = m_phy->GetObject<YansWifiPhy> ()->ComputeInterferenceWhenReceivingData ();
                  double exclusionRegion = m_exclusionRegionHelper.AdaptExclusionRegion (signalMap, deltaInterferenceDb, linkExclusionRegionRecord, DEFAULT_POWER, interferenceW, _item.ewmaPdr);
                  m_signalMap.UpdateExclusionRegion (sender, receiver, exclusionRegion);
                  Simulator::UpdateLinkExclusionRegion (sender, receiver, exclusionRegion);
                  std::cout<<" link sender: "<< sender <<" receiver: "<< receiver << " exclusionRegion: "<< exclusionRegion<< std::endl;
                }
              }
            }
          }
          else
          {
            m_linkEstimator.ClearSequenceNumbers (sender, receiver);
          }
        }



        ObservationItem obsItem;

        obsItem.senderX = x;
        obsItem.senderY = y;
        obsItem.receiverX =  position.x;
        obsItem.receiverY =  position.y;
        obsItem.averageAttenuation = txPower - rxPower;
        obsItem.timeStamp = Simulator::Now ();
        m_observation.AppendObservation (hdr.GetAddr2 ().GetNodeId (), m_self.GetNodeId (), obsItem);

        m_observation.RemoveExpireItems (Seconds(OBSERVATION_EXPIRATION_TIME), MAX_OBSERVATION_ITEMS_PER_LINK);

        /*
         * For testing Double regression the method
         if ( m_self.GetNodeId () == 20)
         {
         DoubleRegression doubleRegresion;
         NodeStatus senderStatus = Simulator::GetNodeStatus (23);
         NodeStatus receiverStatus = Simulator::GetNodeStatus (20);

         if ( senderStatus.x != 0 || receiverStatus.x != 0)
         {
         double at = doubleRegresion.AttenuationEstimation (1, 2, senderStatus.x, senderStatus.y, receiverStatus.x, receiverStatus.y, m_observation);
         if ( at != 0)
         {
         double dist = sqrt ( pow (senderStatus.x - receiverStatus.x, 2) + pow (senderStatus.y - receiverStatus.y, 2));
         double ideaAtten = -46.6777 - 10*1.5*log10(dist);
        //std::cout<< Simulator::Now ()<<" atten: "<< at <<" dist: "<< dist <<" ideaAtten: "<< ideaAtten << std::endl;
        std::cout<< at <<" "<< ideaAtten << " "<<at + ideaAtten << " "<< dist <<std::endl;
        }
        }
        }
        */


        if (hdr.IsData () || hdr.IsMgt ())
        {
          NS_LOG_DEBUG ("rx group from=" << hdr.GetAddr2 ());
          goto rxPacket;
        }
        else
        {
          // DROP
        }
      }
      else if (m_promisc)
      {
        NS_ASSERT (hdr.GetAddr1 () != m_self);
        if (hdr.IsData ())
        {
          goto rxPacket;
        }
      }
      else
      {
        //NS_LOG_DEBUG_VERBOSE ("rx not-for-me from %d", GetSource (packet));
      }
      return;
rxPacket:
      WifiMacTrailer fcs;
      packet->RemoveTrailer (fcs);
      //std::cout<<"packet.size: "<< packet->GetSize () << std::endl; // 100
      m_rxCallback (packet, &hdr);
      return;
    }

  uint32_t
    MacLow::GetAckSize (void) const
    {
      WifiMacHeader ack;
      ack.SetType (WIFI_MAC_CTL_ACK);
      return ack.GetSize () + 4;
    }
  uint32_t
    MacLow::GetBlockAckSize (enum BlockAckType type) const
    {
      WifiMacHeader hdr;
      hdr.SetType (WIFI_MAC_CTL_BACKRESP);
      CtrlBAckResponseHeader blockAck;
      if (type == BASIC_BLOCK_ACK)
      {
        blockAck.SetType (BASIC_BLOCK_ACK);
      }
      else if (type == COMPRESSED_BLOCK_ACK)
      {
        blockAck.SetType (COMPRESSED_BLOCK_ACK);
      }
      else if (type == MULTI_TID_BLOCK_ACK)
      {
        //Not implemented
        NS_ASSERT (false);
      }
      return hdr.GetSize () + blockAck.GetSerializedSize () + 4;
    }
  uint32_t
    MacLow::GetRtsSize (void) const
    {
      WifiMacHeader rts;
      rts.SetType (WIFI_MAC_CTL_RTS);
      return rts.GetSize () + 4;
    }
  Time
    MacLow::GetAckDuration (Mac48Address to, WifiMode dataTxMode) const
    {
      WifiMode ackMode = GetAckTxModeForData (to, dataTxMode);
      return m_phy->CalculateTxDuration (GetAckSize (), ackMode, WIFI_PREAMBLE_LONG);
    }
  Time
    MacLow::GetBlockAckDuration (Mac48Address to, WifiMode blockAckReqTxMode, enum BlockAckType type) const
    {
      /*
       * For immediate BlockAck we should transmit the frame with the same WifiMode
       * as the BlockAckReq.
       *
       * from section 9.6 in IEEE802.11e:
       * The BlockAck control frame shall be sent at the same rate and modulation class as
       * the BlockAckReq frame if it is sent in response to a BlockAckReq frame.
       */
      return m_phy->CalculateTxDuration (GetBlockAckSize (type), blockAckReqTxMode, WIFI_PREAMBLE_LONG);
    }
  Time
    MacLow::GetCtsDuration (Mac48Address to, WifiMode rtsTxMode) const
    {
      WifiMode ctsMode = GetCtsTxModeForRts (to, rtsTxMode);
      return m_phy->CalculateTxDuration (GetCtsSize (), ctsMode, WIFI_PREAMBLE_LONG);
    }
  uint32_t
    MacLow::GetCtsSize (void) const
    {
      WifiMacHeader cts;
      cts.SetType (WIFI_MAC_CTL_CTS);
      return cts.GetSize () + 4;
    }
  uint32_t
    MacLow::GetSize (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
    {
      WifiMacTrailer fcs;
      return packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
    }

  WifiMode
    MacLow::GetRtsTxMode (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
    {
      Mac48Address to = hdr->GetAddr1 ();
      return m_stationManager->GetRtsMode (to, hdr, packet);
    }
  WifiMode
    MacLow::GetDataTxMode (Ptr<const Packet> packet, const WifiMacHeader *hdr) const
    {
      Mac48Address to = hdr->GetAddr1 ();
      WifiMacTrailer fcs;
      uint32_t size =  packet->GetSize () + hdr->GetSize () + fcs.GetSerializedSize ();
      return m_stationManager->GetDataMode (to, hdr, packet, size);
    }

  WifiMode
    MacLow::GetCtsTxModeForRts (Mac48Address to, WifiMode rtsTxMode) const
    {
      return m_stationManager->GetCtsMode (to, rtsTxMode);
    }
  WifiMode
    MacLow::GetAckTxModeForData (Mac48Address to, WifiMode dataTxMode) const
    {
      return m_stationManager->GetAckMode (to, dataTxMode);
    }


  Time
    MacLow::CalculateOverallTxTime (Ptr<const Packet> packet,
        const WifiMacHeader* hdr,
        const MacLowTransmissionParameters& params) const
    {
      Time txTime = Seconds (0);
      WifiMode rtsMode = GetRtsTxMode (packet, hdr);
      WifiMode dataMode = GetDataTxMode (packet, hdr);
      if (params.MustSendRts ())
      {
        txTime += m_phy->CalculateTxDuration (GetRtsSize (), rtsMode, WIFI_PREAMBLE_LONG);
        txTime += GetCtsDuration (hdr->GetAddr1 (), rtsMode);
        txTime += Time (GetSifs () * 2);
      }
      uint32_t dataSize = GetSize (packet, hdr);
      txTime += m_phy->CalculateTxDuration (dataSize, dataMode, WIFI_PREAMBLE_LONG);
      if (params.MustWaitAck ())
      {
        txTime += GetSifs ();
        txTime += GetAckDuration (hdr->GetAddr1 (), dataMode);
      }
      return txTime;
    }

  Time
    MacLow::CalculateTransmissionTime (Ptr<const Packet> packet,
        const WifiMacHeader* hdr,
        const MacLowTransmissionParameters& params) const
    {
      Time txTime = CalculateOverallTxTime (packet, hdr, params);
      if (params.HasNextPacket ())
      {
        WifiMode dataMode = GetDataTxMode (packet, hdr);
        txTime += GetSifs ();
        txTime += m_phy->CalculateTxDuration (params.GetNextPacketSize (), dataMode, WIFI_PREAMBLE_LONG);
      }
      return txTime;
    }

  void
    MacLow::NotifyNav (const WifiMacHeader &hdr, WifiMode txMode, WifiPreamble preamble)
    {
      NS_ASSERT (m_lastNavStart <= Simulator::Now ());
      Time duration = hdr.GetDuration ();

      if (hdr.IsCfpoll ()
          && hdr.GetAddr2 () == m_bssid)
      {
        // see section 9.3.2.2 802.11-1999
        DoNavResetNow (duration);
        return;
      }
      // XXX Note that we should also handle CF_END specially here
      // but we don't for now because we do not generate them.
      else if (hdr.GetAddr1 () != m_self)
      {
        // see section 9.2.5.4 802.11-1999
        bool navUpdated = DoNavStartNow (duration);
        if (hdr.IsRts () && navUpdated)
        {
          /**
           * A STA that used information from an RTS frame as the most recent basis to update its NAV setting
           * is permitted to reset its NAV if no PHY-RXSTART.indication is detected from the PHY during a
           * period with a duration of (2 * aSIFSTime) + (CTS_Time) + (2 * aSlotTime) starting at the
           * PHY-RXEND.indication corresponding to the detection of the RTS frame. The “CTS_Time” shall
           * be calculated using the length of the CTS frame and the data rate at which the RTS frame
           * used for the most recent NAV update was received.
           */
          WifiMacHeader cts;
          cts.SetType (WIFI_MAC_CTL_CTS);
          Time navCounterResetCtsMissedDelay =
            m_phy->CalculateTxDuration (cts.GetSerializedSize (), txMode, preamble) +
            Time (2 * GetSifs ()) + Time (2 * GetSlotTime ());
          m_navCounterResetCtsMissed = Simulator::Schedule (navCounterResetCtsMissedDelay,
              &MacLow::NavCounterResetCtsMissed, this,
              Simulator::Now ());
        }
      }
    }

  void
    MacLow::NavCounterResetCtsMissed (Time rtsEndRxTime)
    {
      if (m_phy->GetLastRxStartTime () > rtsEndRxTime)
      {
        DoNavResetNow (Seconds (0.0));
      }
    }

  void
    MacLow::DoNavResetNow (Time duration)
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->NavReset (duration);
      }
      m_lastNavStart = Simulator::Now ();
      m_lastNavStart = duration;
    }
  bool
    MacLow::DoNavStartNow (Time duration)
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->NavStart (duration);
      }
      Time newNavEnd = Simulator::Now () + duration;
      Time oldNavEnd = m_lastNavStart + m_lastNavDuration;
      if (newNavEnd > oldNavEnd)
      {
        m_lastNavStart = Simulator::Now ();
        m_lastNavDuration = duration;
        return true;
      }
      return false;
    }
  void
    MacLow::NotifyAckTimeoutStartNow (Time duration)
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->AckTimeoutStart (duration);
      }
    }
  void
    MacLow::NotifyAckTimeoutResetNow ()
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->AckTimeoutReset ();
      }
    }
  void
    MacLow::NotifyCtsTimeoutStartNow (Time duration)
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->CtsTimeoutStart (duration);
      }
    }
  void
    MacLow::NotifyCtsTimeoutResetNow ()
    {
      for (DcfListenersCI i = m_dcfListeners.begin (); i != m_dcfListeners.end (); i++)
      {
        (*i)->CtsTimeoutReset ();
      }
    }

  void
    MacLow::ForwardDown (Ptr<const Packet> packet, const WifiMacHeader* hdr,
        WifiMode txMode)
    {
      NS_LOG_FUNCTION (this << packet << hdr << txMode);
      NS_LOG_DEBUG ("send " << hdr->GetTypeString () <<
          ", to=" << hdr->GetAddr1 () <<
          ", size=" << packet->GetSize () <<
          ", mode=" << txMode <<
          ", duration=" << hdr->GetDuration () <<
          ", seq=0x" << std::hex << m_currentHdr.GetSequenceControl () << std::dec);

      if (m_phy->GetChannelNumber () == CONTROL_CHANNEL )
      {
        //=================================if in control channel===========
        std::vector<uint16_t> neighbors;
        m_signalMap.GetOneHopNeighbors (LINK_SELECTION_THRESHOLD, neighbors);
        if (neighbors.size () > 0)
        {
          double maxExclusionRegion = 0;
          for (std::vector<uint16_t>::iterator it = neighbors.begin (); it != neighbors.end (); ++ it)
          { 
            double tempExclusionRegion = m_signalMap.GetLinkExclusionRegionValue (m_self.GetNodeId (), *it);
            if ( maxExclusionRegion >= tempExclusionRegion)
            {
              maxExclusionRegion = tempExclusionRegion;
            }
          }
          uint32_t txPower=0;
          double attenuation = DEFAULT_POWER - maxExclusionRegion;
          txPower = (uint32_t)attenuation + DELIVERY_100_SNR;
          txPower = txPower - DEFAULT_POWER;
          //std::cout<<" txPowerLevel: "<< txPower << std::endl;
          //txPower = 0; // to test double regression
          //double powerDbm = m_phy->GetPowerDbm (txPower);

          //=============================Control Channel Power Level Update. 
          //This Is Due To Power Control, And Is Used For Signal Map Update.
          
          Ptr<Packet> pkt = packet->Copy ();
          WifiMacHeader _hdr;
          WifiMacTrailer _fcs;
          pkt->RemoveHeader (_hdr);
          pkt->RemoveTrailer (_fcs);
          uint8_t payload[DEFAULT_PACKET_LENGTH];
          pkt->CopyData (payload, DEFAULT_PACKET_LENGTH);
          PayloadBuffer buff = PayloadBuffer (payload);
          //uint8_t txPowerLevel = 0;//default tx Power level
          buff.WriteDouble (m_phy->GetObject<YansWifiPhy> ()->GetPowerDbm (txPower) + TX_GAIN);
          //std::cout<<" encoded: "<< m_phy->GetObject<YansWifiPhy> ()->GetPowerDbm (txPower) + TX_GAIN << std::endl;

          pkt = Create<Packet> (payload, DEFAULT_PACKET_LENGTH);
          pkt->AddHeader (_hdr);
          pkt->AddTrailer (_fcs);

          m_phy->SendPacket (pkt, txMode, WIFI_PREAMBLE_LONG, (uint8_t) txPower);
          //======================== POWER control for control signal =================
        }
        else
          m_phy->SendPacket (packet, txMode, WIFI_PREAMBLE_LONG, 0);
      }
      else if (m_phy->GetChannelNumber () == DATA_CHANNEL )
      {
        m_phy->SendPacket (packet, txMode, WIFI_PREAMBLE_LONG, 0);
      }
    }

  void
    MacLow::CtsTimeout (void)
    {
      NS_LOG_FUNCTION (this);
      NS_LOG_DEBUG ("cts timeout");
      // XXX: should check that there was no rx start before now.
      // we should restart a new cts timeout now until the expected
      // end of rx if there was a rx start before now.
      m_stationManager->ReportRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      m_currentPacket = 0;
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      listener->MissedCts ();
    }
  void
    MacLow::NormalAckTimeout (void)
    {
      NS_LOG_FUNCTION (this);
      NS_LOG_DEBUG ("normal ack timeout");
      // XXX: should check that there was no rx start before now.
      // we should restart a new ack timeout now until the expected
      // end of rx if there was a rx start before now.
      m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      listener->MissedAck ();
    }
  void
    MacLow::FastAckTimeout (void)
    {
      NS_LOG_FUNCTION (this);
      m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      if (m_phy->IsStateIdle ())
      {
        NS_LOG_DEBUG ("fast Ack idle missed");
        listener->MissedAck ();
      }
      else
      {
        NS_LOG_DEBUG ("fast Ack ok");
      }
    }
  void
    MacLow::BlockAckTimeout (void)
    {
      NS_LOG_FUNCTION (this);
      NS_LOG_DEBUG ("block ack timeout");

      m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      listener->MissedBlockAck ();
    }
  void
    MacLow::SuperFastAckTimeout ()
    {
      NS_LOG_FUNCTION (this);
      m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      if (m_phy->IsStateIdle ())
      {
        NS_LOG_DEBUG ("super fast Ack failed");
        listener->MissedAck ();
      }
      else
      {
        NS_LOG_DEBUG ("super fast Ack ok");
        listener->GotAck (0.0, WifiMode ());
      }
    }

  void
    MacLow::SendRtsForPacket (void)
    {
      NS_LOG_FUNCTION (this);
      /* send an RTS for this packet. */
      WifiMacHeader rts;
      rts.SetType (WIFI_MAC_CTL_RTS);
      rts.SetDsNotFrom ();
      rts.SetDsNotTo ();
      rts.SetNoRetry ();
      rts.SetNoMoreFragments ();
      rts.SetAddr1 (m_currentHdr.GetAddr1 ());
      rts.SetAddr2 (m_self);
      WifiMode rtsTxMode = GetRtsTxMode (m_currentPacket, &m_currentHdr);
      Time duration = Seconds (0);
      if (m_txParams.HasDurationId ())
      {
        duration += m_txParams.GetDurationId ();
      }
      else
      {
        WifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
        duration += GetSifs ();
        duration += GetCtsDuration (m_currentHdr.GetAddr1 (), rtsTxMode);
        duration += GetSifs ();
        duration += m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr),
            dataTxMode, WIFI_PREAMBLE_LONG);
        duration += GetSifs ();
        duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
      }
      rts.SetDuration (duration);

      Time txDuration = m_phy->CalculateTxDuration (GetRtsSize (), rtsTxMode, WIFI_PREAMBLE_LONG);
      Time timerDelay = txDuration + GetCtsTimeout ();

      NS_ASSERT (m_ctsTimeoutEvent.IsExpired ());
      NotifyCtsTimeoutStartNow (timerDelay);
      m_ctsTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::CtsTimeout, this);

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (rts);
      WifiMacTrailer fcs;
      packet->AddTrailer (fcs);

      ForwardDown (packet, &rts, rtsTxMode);
    }

  void
    MacLow::StartDataTxTimers (void)
    {
      if ( Simulator::Now () >= Seconds (START_PROCESS_TIME))
      {
        //std::cout<<m_self.GetNodeId ()<<" m_currentPacket: "<< m_currentPacket <<" hdr: "<< &m_currentHdr<< std::endl;
      }
      WifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
      Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr), dataTxMode, WIFI_PREAMBLE_LONG);
      if (m_txParams.MustWaitNormalAck ())
      {
        Time timerDelay = txDuration + GetAckTimeout ();
        NS_ASSERT (m_normalAckTimeoutEvent.IsExpired ());
        NotifyAckTimeoutStartNow (timerDelay);
        m_normalAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::NormalAckTimeout, this);
      }
      else if (m_txParams.MustWaitFastAck ())
      {
        Time timerDelay = txDuration + GetPifs ();
        NS_ASSERT (m_fastAckTimeoutEvent.IsExpired ());
        NotifyAckTimeoutStartNow (timerDelay);
        m_fastAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::FastAckTimeout, this);
      }
      else if (m_txParams.MustWaitSuperFastAck ())
      {
        Time timerDelay = txDuration + GetPifs ();
        NS_ASSERT (m_superFastAckTimeoutEvent.IsExpired ());
        NotifyAckTimeoutStartNow (timerDelay);
        m_superFastAckTimeoutEvent = Simulator::Schedule (timerDelay,
            &MacLow::SuperFastAckTimeout, this);
      }
      else if (m_txParams.MustWaitBasicBlockAck ())
      {
        Time timerDelay = txDuration + GetBasicBlockAckTimeout ();
        NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
        m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::BlockAckTimeout, this);
      }
      else if (m_txParams.MustWaitCompressedBlockAck ())
      {
        Time timerDelay = txDuration + GetCompressedBlockAckTimeout ();
        NS_ASSERT (m_blockAckTimeoutEvent.IsExpired ());
        m_blockAckTimeoutEvent = Simulator::Schedule (timerDelay, &MacLow::BlockAckTimeout, this);
      }
      else if (m_txParams.HasNextPacket ())
      {
        Time delay = txDuration + GetSifs ();
        NS_ASSERT (m_waitSifsEvent.IsExpired ());
        m_waitSifsEvent = Simulator::Schedule (delay, &MacLow::WaitSifsAfterEndTx, this);
      }
      else
      {
        // since we do not expect any timer to be triggered.
        m_listener = 0;
      }
    }

  void
    MacLow::SendDataPacket (void)
    {
      NS_LOG_FUNCTION (this);
      if ( m_phy->IsStateIdle () == false)
      {
        //std::cout<<" trying to send, channel is not idle, channel number is: "<< m_phy->GetChannelNumber () << std::endl; 
        return;
      }
      if ( Simulator::Now () > Seconds (START_PROCESS_TIME) && m_phy->GetChannelNumber () == DATA_CHANNEL)
        std::cout<<Simulator::Now () <<" "<<m_self.GetNodeId ()<< " is sending "<< std::endl;
      /* send this packet directly. No RTS is needed. */
      StartDataTxTimers ();
      if (Simulator::Now () > Seconds (START_PROCESS_TIME) && m_phy->GetChannelNumber () == DATA_CHANNEL)
      {
        Simulator::PrintReceivers (m_self.GetNodeId () );
      }

      WifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
      Time duration = Seconds (0.0);
      if (m_txParams.HasDurationId ())
      {
        duration += m_txParams.GetDurationId ();
      }
      else
      {
        if (m_txParams.MustWaitBasicBlockAck ())
        {
          duration += GetSifs ();
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), dataTxMode, BASIC_BLOCK_ACK);
        }
        else if (m_txParams.MustWaitCompressedBlockAck ())
        {
          duration += GetSifs ();
          duration += GetBlockAckDuration (m_currentHdr.GetAddr1 (), dataTxMode, COMPRESSED_BLOCK_ACK);
        }
        else if (m_txParams.MustWaitAck ())
        {
          duration += GetSifs ();
          duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
        }
        if (m_txParams.HasNextPacket ())
        {
          duration += GetSifs ();
          duration += m_phy->CalculateTxDuration (m_txParams.GetNextPacketSize (),
              dataTxMode, WIFI_PREAMBLE_LONG);
          if (m_txParams.MustWaitAck ())
          {
            duration += GetSifs ();
            duration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
          }
        }
      }
      m_currentHdr.SetDuration (duration);
      if (m_phy->GetChannelNumber () == DATA_CHANNEL )
      {
        //std::cout<<m_self.GetNodeId () <<" sending sequence number: "<< m_sequenceNumber << std::endl;
        m_currentHdr.SetSequenceNumber (m_sequenceNumber); // set sequence number for packets at data channel.
        m_sequenceNumber ++ ;
        //std::cout<<m_self.GetNodeId () <<" seq_next: "<< m_sequenceNumber << std::endl;
      }

      //---------Add txPower---------------------------------------
      uint8_t payload[DEFAULT_PACKET_LENGTH];
      m_currentPacket->CopyData (payload, DEFAULT_PACKET_LENGTH);
      PayloadBuffer buff = PayloadBuffer (payload);
      uint8_t txPowerLevel = 0;//default tx Power level
      buff.WriteDouble (m_phy->GetObject<YansWifiPhy> () ->GetPowerDbm (txPowerLevel) + TX_GAIN);

      //std::cout<<" encoded: "<< m_phy->GetObject<YansWifiPhy> () ->GetPowerDbm (0) << std::endl;
      buff.ReadDoubles (4); //rxpower, angle, pos.x, pos.y
      buff.WriteU8 ((uint8_t)m_edge.size ());
      buff.WriteString (m_edge);

      //std::cout<<m_self.GetNodeId () <<" send: sending slot: "<< m_nextSendingSlot << std::endl;
      buff.WriteU64 (m_nextSendingSlot);
      SortSendingSlot ();
      std::vector<NodeSendingStatus> vec = GetFirstTwoNodeSendingSlot (GetCurrentSlot ());
      uint8_t size = (uint8_t)vec.size ();
      buff.WriteU8 (size);
      for (uint8_t i = 0; i < size; ++ i)
      {
        buff.WriteU16 (vec[i].nodeId);
        buff.WriteU64 (vec[i].sendingSlot);
        //std::cout<<m_self.GetNodeId () <<" nodeId: "<< vec[i].nodeId << " slot: "<< vec[i].sendingSlot << std::endl;
      }
      //=================Share Exclusion Region Information=====================
      uint32_t remainBytes = buff.CheckRemainBytes (DEFAULT_PACKET_LENGTH);
      uint32_t count = (remainBytes-1)/(13+32);
      std::vector<LinkExclusionRegion> linkExclusionRegionVec = m_exclusionRegionHelper.GetLatestUpdatedItems (count, m_signalMap);
      //std::cout<<" linkexclusionregionvec.size: "<< linkExclusionRegionVec.size () <<" count: "<< count << std::endl;
      buff.WriteU8 ((uint8_t) linkExclusionRegionVec.size ());
      for (uint32_t i = 0; i < linkExclusionRegionVec.size (); ++ i)
      {
        buff.WriteU16 (linkExclusionRegionVec[i].sender);
        buff.WriteU16 (linkExclusionRegionVec[i].receiver);
        buff.WriteDouble (linkExclusionRegionVec[i].currentExclusionRegion);
        buff.WriteU8 (linkExclusionRegionVec[i].version);
        buff.WriteDouble (linkExclusionRegionVec[i].senderX);
        buff.WriteDouble (linkExclusionRegionVec[i].senderY);
        buff.WriteDouble (linkExclusionRegionVec[i].receiverX);
        buff.WriteDouble (linkExclusionRegionVec[i].receiverY);
        //std::cout<<m_self.GetNodeId ()<<" writing sender: "<< linkExclusionRegionVec[i].sender<<" receiver: "<<linkExclusionRegionVec[i].receiver <<" exclusion: "<< linkExclusionRegionVec[i].currentExclusionRegion <<" version: "<< (uint32_t) linkExclusionRegionVec[i].version << " senderX: "<< linkExclusionRegionVec[i].senderX <<" senderY: "<< linkExclusionRegionVec[i].senderY <<" receiverX: "<< linkExclusionRegionVec[i].receiverX <<" receiverY: "<< linkExclusionRegionVec[i].receiverY << std::endl;
      }



      //Updating Node Status============
      NodeStatus status;
      status.nodeId = m_self.GetNodeId ();
      status.begin = m_begin;
      status.end = m_end;
      status.angle = m_angle;
      status.x = m_positionX;
      status.y = m_positionY;
      Simulator::UpdateNodeStatus (m_self.GetNodeId (), status);
      //Simulator::PrintNodeStatus (m_self.GetNodeId ());


      m_currentPacket = Create<Packet> (payload, DEFAULT_PACKET_LENGTH);
      //-----------------------------------------------------------

      m_currentPacket->AddHeader (m_currentHdr);
      WifiMacTrailer fcs;
      m_currentPacket->AddTrailer (fcs);

      ForwardDown (m_currentPacket, &m_currentHdr, dataTxMode);
      m_currentPacket = 0;
    }

  bool
    MacLow::IsNavZero (void) const
    {
      if (m_lastNavStart + m_lastNavDuration < Simulator::Now ())
      {
        return true;
      }
      else
      {
        return false;
      }
    }

  void
    MacLow::SendCtsAfterRts (Mac48Address source, Time duration, WifiMode rtsTxMode, double rtsSnr)
    {
      NS_LOG_FUNCTION (this << source << duration << rtsTxMode << rtsSnr);
      /* send a CTS when you receive a RTS
       * right after SIFS.
       */
      WifiMode ctsTxMode = GetCtsTxModeForRts (source, rtsTxMode);
      WifiMacHeader cts;
      cts.SetType (WIFI_MAC_CTL_CTS);
      cts.SetDsNotFrom ();
      cts.SetDsNotTo ();
      cts.SetNoMoreFragments ();
      cts.SetNoRetry ();
      cts.SetAddr1 (source);
      duration -= GetCtsDuration (source, rtsTxMode);
      duration -= GetSifs ();
      NS_ASSERT (duration >= MicroSeconds (0));
      cts.SetDuration (duration);

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (cts);
      WifiMacTrailer fcs;
      packet->AddTrailer (fcs);

      SnrTag tag;
      tag.Set (rtsSnr);
      packet->AddPacketTag (tag);

      ForwardDown (packet, &cts, ctsTxMode);
    }

  void
    MacLow::SendDataAfterCts (Mac48Address source, Time duration, WifiMode txMode)
    {
      NS_LOG_FUNCTION (this);
      /* send the third step in a
       * RTS/CTS/DATA/ACK hanshake
       */
      NS_ASSERT (m_currentPacket != 0);
      StartDataTxTimers ();

      WifiMode dataTxMode = GetDataTxMode (m_currentPacket, &m_currentHdr);
      Time newDuration = Seconds (0);
      newDuration += GetSifs ();
      newDuration += GetAckDuration (m_currentHdr.GetAddr1 (), dataTxMode);
      Time txDuration = m_phy->CalculateTxDuration (GetSize (m_currentPacket, &m_currentHdr),
          dataTxMode, WIFI_PREAMBLE_LONG);
      duration -= txDuration;
      duration -= GetSifs ();

      duration = std::max (duration, newDuration);
      NS_ASSERT (duration >= MicroSeconds (0));
      m_currentHdr.SetDuration (duration);

      m_currentPacket->AddHeader (m_currentHdr);
      WifiMacTrailer fcs;
      m_currentPacket->AddTrailer (fcs);

      ForwardDown (m_currentPacket, &m_currentHdr, dataTxMode);
      m_currentPacket = 0;
    }

  void
    MacLow::WaitSifsAfterEndTx (void)
    {
      m_listener->StartNext ();
    }

  void
    MacLow::FastAckFailedTimeout (void)
    {
      NS_LOG_FUNCTION (this);
      MacLowTransmissionListener *listener = m_listener;
      m_listener = 0;
      listener->MissedAck ();
      NS_LOG_DEBUG ("fast Ack busy but missed");
    }

  void
    MacLow::SendAckAfterData (Mac48Address source, Time duration, WifiMode dataTxMode, double dataSnr)
    {
      NS_LOG_FUNCTION (this);
      /* send an ACK when you receive
       * a packet after SIFS.
       */
      WifiMode ackTxMode = GetAckTxModeForData (source, dataTxMode);
      WifiMacHeader ack;
      ack.SetType (WIFI_MAC_CTL_ACK);
      ack.SetDsNotFrom ();
      ack.SetDsNotTo ();
      ack.SetNoRetry ();
      ack.SetNoMoreFragments ();
      ack.SetAddr1 (source);
      duration -= GetAckDuration (source, dataTxMode);
      duration -= GetSifs ();
      NS_ASSERT (duration >= MicroSeconds (0));
      ack.SetDuration (duration);

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (ack);
      WifiMacTrailer fcs;
      packet->AddTrailer (fcs);

      SnrTag tag;
      tag.Set (dataSnr);
      packet->AddPacketTag (tag);

      ForwardDown (packet, &ack, ackTxMode);
    }

  bool
    MacLow::StoreMpduIfNeeded (Ptr<Packet> packet, WifiMacHeader hdr)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
      if (it != m_bAckAgreements.end ())
      {
        WifiMacTrailer fcs;
        packet->RemoveTrailer (fcs);
        BufferedPacket bufferedPacket (packet, hdr);

        uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
        uint16_t mappedSeqControl = QosUtilsMapSeqControlToUniqueInteger (hdr.GetSequenceControl (), endSequence);

        BufferedPacketI i = (*it).second.second.begin ();
        for (; i != (*it).second.second.end ()
            && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceControl (), endSequence) < mappedSeqControl; i++)
        {
          ;
        }
        (*it).second.second.insert (i, bufferedPacket);

        //Update block ack cache
        BlockAckCachesI j = m_bAckCaches.find (std::make_pair (hdr.GetAddr2 (), hdr.GetQosTid ()));
        NS_ASSERT (j != m_bAckCaches.end ());
        (*j).second.UpdateWithMpdu (&hdr);

        return true;
      }
      return false;
    }

  void
    MacLow::CreateBlockAckAgreement (const MgtAddBaResponseHeader *respHdr, Mac48Address originator,
        uint16_t startingSeq)
    {
      uint8_t tid = respHdr->GetTid ();
      BlockAckAgreement agreement (originator, tid);
      if (respHdr->IsImmediateBlockAck ())
      {
        agreement.SetImmediateBlockAck ();
      }
      else
      {
        agreement.SetDelayedBlockAck ();
      }
      agreement.SetAmsduSupport (respHdr->IsAmsduSupported ());
      agreement.SetBufferSize (respHdr->GetBufferSize () + 1);
      agreement.SetTimeout (respHdr->GetTimeout ());
      agreement.SetStartingSequence (startingSeq);

      std::list<BufferedPacket> buffer (0);
      AgreementKey key (originator, respHdr->GetTid ());
      AgreementValue value (agreement, buffer);
      m_bAckAgreements.insert (std::make_pair (key, value));

      BlockAckCache cache;
      cache.Init (startingSeq, respHdr->GetBufferSize () + 1);
      m_bAckCaches.insert (std::make_pair (key, cache));

      if (respHdr->GetTimeout () != 0)
      {
        AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, respHdr->GetTid ()));
        Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());

        AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());

        it->second.first.m_inactivityEvent = Simulator::Schedule (timeout,
            &MacLowBlockAckEventListener::BlockAckInactivityTimeout,
            m_edcaListeners[ac],
            originator, tid);
      }
    }

  void
    MacLow::DestroyBlockAckAgreement (Mac48Address originator, uint8_t tid)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
      {
        RxCompleteBufferedPacketsWithSmallerSequence (it->second.first.GetStartingSequence (), originator, tid);
        RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
        m_bAckAgreements.erase (it);

        BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
        NS_ASSERT (i != m_bAckCaches.end ());
        m_bAckCaches.erase (i);
      }
    }

  void
    MacLow::RxCompleteBufferedPacketsWithSmallerSequence (uint16_t seq, Mac48Address originator, uint8_t tid)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
      {
        uint16_t endSequence = ((*it).second.first.GetStartingSequence () + 2047) % 4096;
        uint16_t mappedStart = QosUtilsMapSeqControlToUniqueInteger (seq, endSequence);
        uint16_t guard = (*it).second.second.begin ()->second.GetSequenceControl () & 0xfff0;
        BufferedPacketI last = (*it).second.second.begin ();

        BufferedPacketI i = (*it).second.second.begin ();
        for (; i != (*it).second.second.end ()
            && QosUtilsMapSeqControlToUniqueInteger ((*i).second.GetSequenceNumber (), endSequence) < mappedStart;)
        {
          if (guard == (*i).second.GetSequenceControl ())
          {
            if (!(*i).second.IsMoreFragments ())
            {
              while (last != i)
              {
                m_rxCallback ((*last).first, &(*last).second);
                last++;
              }
              m_rxCallback ((*last).first, &(*last).second);
              last++;
              /* go to next packet */
              while (i != (*it).second.second.end () && ((guard >> 4) & 0x0fff) == (*i).second.GetSequenceNumber ())
              {
                i++;
              }
              if (i != (*it).second.second.end ())
              {
                guard = (*i).second.GetSequenceControl () & 0xfff0;
                last = i;
              }
            }
            else
            {
              guard++;
            }
          }
          else
          {
            /* go to next packet */
            while (i != (*it).second.second.end () && ((guard >> 4) & 0x0fff) == (*i).second.GetSequenceNumber ())
            {
              i++;
            }
            if (i != (*it).second.second.end ())
            {
              guard = (*i).second.GetSequenceControl () & 0xfff0;
              last = i;
            }
          }
        }
        (*it).second.second.erase ((*it).second.second.begin (), i);
      }
    }

  void
    MacLow::RxCompleteBufferedPacketsUntilFirstLost (Mac48Address originator, uint8_t tid)
    {
      AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
      if (it != m_bAckAgreements.end ())
      {
        uint16_t startingSeqCtrl = ((*it).second.first.GetStartingSequence () << 4) & 0xfff0;
        uint16_t guard = startingSeqCtrl;

        BufferedPacketI lastComplete = (*it).second.second.begin ();
        BufferedPacketI i = (*it).second.second.begin ();
        for (; i != (*it).second.second.end () && guard == (*i).second.GetSequenceControl (); i++)
        {
          if (!(*i).second.IsMoreFragments ())
          {
            while (lastComplete != i)
            {
              m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
              lastComplete++;
            }
            m_rxCallback ((*lastComplete).first, &(*lastComplete).second);
            lastComplete++;
          }
          guard = (*i).second.IsMoreFragments () ? (guard + 1) : ((guard + 16) & 0xfff0);
        }
        (*it).second.first.SetStartingSequence ((guard >> 4) & 0x0fff);
        /* All packets already forwarded to WifiMac must be removed from buffer:
           [begin (), lastComplete) */
        (*it).second.second.erase ((*it).second.second.begin (), lastComplete);
      }
    }

  void
    MacLow::SendBlockAckResponse (const CtrlBAckResponseHeader* blockAck, Mac48Address originator, bool immediate,
        Time duration, WifiMode blockAckReqTxMode)
    {
      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (*blockAck);

      WifiMacHeader hdr;
      hdr.SetType (WIFI_MAC_CTL_BACKRESP);
      hdr.SetAddr1 (originator);
      hdr.SetAddr2 (GetAddress ());
      hdr.SetDsNotFrom ();
      hdr.SetDsNotTo ();
      hdr.SetNoRetry ();
      hdr.SetNoMoreFragments ();

      m_currentPacket = packet;
      m_currentHdr = hdr;
      if (immediate)
      {
        m_txParams.DisableAck ();
        duration -= GetSifs ();
        if (blockAck->IsBasic ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxMode, BASIC_BLOCK_ACK);
        }
        else if (blockAck->IsCompressed ())
        {
          duration -= GetBlockAckDuration (originator, blockAckReqTxMode, COMPRESSED_BLOCK_ACK);
        }
        else if (blockAck->IsMultiTid ())
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
        }
      }
      else
      {
        m_txParams.EnableAck ();
        duration += GetSifs ();
        duration += GetAckDuration (originator, blockAckReqTxMode);
      }
      m_txParams.DisableNextData ();

      StartDataTxTimers ();

      NS_ASSERT (duration >= MicroSeconds (0));
      hdr.SetDuration (duration);
      //here should be present a control about immediate or delayed block ack
      //for now we assume immediate
      packet->AddHeader (hdr);
      WifiMacTrailer fcs;
      packet->AddTrailer (fcs);
      ForwardDown (packet, &hdr, blockAckReqTxMode);
      m_currentPacket = 0;
    }

  void
    MacLow::SendBlockAckAfterBlockAckRequest (const CtrlBAckRequestHeader reqHdr, Mac48Address originator,
        Time duration, WifiMode blockAckReqTxMode)
    {
      NS_LOG_FUNCTION (this);
      CtrlBAckResponseHeader blockAck;
      uint8_t tid;
      bool immediate = false;
      if (!reqHdr.IsMultiTid ())
      {
        tid = reqHdr.GetTidInfo ();
        AgreementsI it = m_bAckAgreements.find (std::make_pair (originator, tid));
        if (it != m_bAckAgreements.end ())
        {
          blockAck.SetStartingSequence (reqHdr.GetStartingSequence ());
          blockAck.SetTidInfo (tid);
          immediate = (*it).second.first.IsImmediateBlockAck ();
          if (reqHdr.IsBasic ())
          {
            blockAck.SetType (BASIC_BLOCK_ACK);
          }
          else if (reqHdr.IsCompressed ())
          {
            blockAck.SetType (COMPRESSED_BLOCK_ACK);
          }
          BlockAckCachesI i = m_bAckCaches.find (std::make_pair (originator, tid));
          NS_ASSERT (i != m_bAckCaches.end ());
          (*i).second.FillBlockAckBitmap (&blockAck);

          /* All packets with smaller sequence than starting sequence control must be passed up to Wifimac
           * See 9.10.3 in IEEE8022.11e standard.
           */
          RxCompleteBufferedPacketsWithSmallerSequence (reqHdr.GetStartingSequence (), originator, tid);
          RxCompleteBufferedPacketsUntilFirstLost (originator, tid);
        }
        else
        {
          NS_LOG_DEBUG ("there's not a valid block ack agreement with " << originator);
        }
      }
      else
      {
        NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
      }

      SendBlockAckResponse (&blockAck, originator, immediate, duration, blockAckReqTxMode);
    }

  void
    MacLow::ResetBlockAckInactivityTimerIfNeeded (BlockAckAgreement &agreement)
    {
      if (agreement.GetTimeout () != 0)
      {
        NS_ASSERT (agreement.m_inactivityEvent.IsRunning ());
        agreement.m_inactivityEvent.Cancel ();
        Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());

        AcIndex ac = QosUtilsMapTidToAc (agreement.GetTid ());
        //std::map<AcIndex, MacLowTransmissionListener*>::iterator it = m_edcaListeners.find (ac);
        //NS_ASSERT (it != m_edcaListeners.end ());

        agreement.m_inactivityEvent = Simulator::Schedule (timeout,
            &MacLowBlockAckEventListener::BlockAckInactivityTimeout,
            m_edcaListeners[ac],
            agreement.GetPeer (),
            agreement.GetTid ());
      }
    }

  void
    MacLow::RegisterBlockAckListenerForAc (enum AcIndex ac, MacLowBlockAckEventListener *listener)
    {
      m_edcaListeners.insert (std::make_pair (ac, listener));
    }

  int64_t MacLow::GetCurrentSlot ()
  {
    m_currentSlot = Simulator::Now ().GetNanoSeconds () / (SLOT_LENGTH * 1000);
    return m_currentSlot;
  }
  void MacLow::SetPosition (double x, double y)
  {
    //std::cout<<" set position: "<<" x: "<< x<< " y: "<< y << std::endl;
    m_positionY = y;
    m_positionX = x;
    m_signalMap.SetXY (x, y);
  }

  void MacLow::SetAngle (double angle)
  {
    //std::cout<<" set angle: "<< angle << std::endl;
    m_angle = angle;
  }

  void MacLow::SetEdge (std::string edge)
  {
    //std::cout<<" edge: "<< edge <<" length: "<< edge.size ()<< std::endl;
    m_edge = edge;
  }

  void MacLow::GetOwnSlotsInFrame (uint16_t &begin, uint16_t &end, DirectionDistribution directions)
  {
    double preSum=0;
    for (uint32_t i = 0; i < directions.selfSector; ++ i)
    {
      //std::cout<<" ratio["<<i<<"]: "<< directions.ratio[i] <<" self: "<< directions.selfSector << std::endl;
      preSum += directions.ratio[i];
    }
    if ( preSum != 0)
    {
      begin = (uint32_t) (preSum * FRAME_LENGTH + 1);
      //std::cout<<" begin: "<<begin <<" presum: "<< preSum<< std::endl;
    }
    else 
    {
      begin = 0;
    }
    //std::cout<<" directions.ratio[directions.selfSector]: "<< directions.ratio[directions.selfSector] << std::endl;
    end = (uint32_t) ((preSum + directions.ratio[directions.selfSector] ) * FRAME_LENGTH);
    //std::cout<<" directions.selfSector: "<< directions.selfSector <<" begin: "<< begin <<" end: "<< end<< std::endl;
  }

  void MacLow::SetStartTxCallback (StartTxCallback callback)
  {
    m_startTxCallback = callback;
  }
  void MacLow::SetQueueEmptyCallback (BooleanCallback callback)
  {
    m_queueEmptyCallback = callback;
  }
  void MacLow::SetListenerCallback (VoidCallback callback)
  {
    m_setListenerCallback = callback;
  }
  void MacLow::SetDcaTxopPacketCallback (SetPacketCallback callback)
  {
    m_setPacketCallback = callback;
  }

  void MacLow::SetMacLowTransmissionListener (MacLowTransmissionListener *listener)
  {
    m_listener = listener;
  }

  int64_t MacLow::CalculatePriority (uint16_t nodeId, int64_t slot)
  {
    //int64_t slot = GetCurrentSlot ();
    int64_t seed = nodeId * 10000 + slot;
    srand (seed);
    int64_t priority = rand () * 10000 + nodeId;
    return abs (priority);
  }

  bool MacLow::IsSelfMaximum (std::vector<uint16_t> conflictSet, int64_t slot)
  {
    int64_t selfPriority = CalculatePriority (m_self.GetNodeId (), slot);
    int64_t maxPriority = selfPriority;
    for (std::vector<uint16_t>::iterator it = conflictSet.begin (); it != conflictSet.end (); ++ it)
    {
      if ( *it == m_self.GetNodeId ())
        continue;
      int64_t temp = CalculatePriority (*it, slot);
      if ( temp > maxPriority)
        maxPriority = temp;
    }
    //std::cout<<" selfpriority: "<< selfPriority <<" maxpriority: "<< maxPriority<<" m_currentSlot: "<< m_currentSlot << std::endl;
    if (maxPriority == selfPriority)
      return true;
    else
      return false;
  }

  void MacLow::CalculateSchedule ()
  {
    //std::cout<<" m_neighborSignalMaps.size (): "<<m_neighborSignalMaps.size () << std::endl;
    //std::cout<<" m_nodesSendingStatus.size (): "<< m_nodesSendingStatus.size () << std::endl;
    //std::cout<<" trying to calculate a schedule "<< std::endl;
    //==================Update Node Status========================================
    NodeStatus status;
    status.nodeId = m_self.GetNodeId ();
    status.begin = m_begin;
    status.end = m_end;
    status.angle = m_angle;
    status.x = m_positionX;
    status.y = m_positionY;
    Simulator::UpdateNodeStatus (m_self.GetNodeId (), status);
    NodeSendingStatus sendingStatus;
    sendingStatus.nodeId = m_self.GetNodeId ();
    sendingStatus.sendingSlot = m_nextSendingSlot;
    //If m_nextSendingSlot is always the same, the add method will simply locate the record and return.
    //If the current slot equals to m_nextSendingSlot, in Simulator, the value would still be the current slot. 
    //This is convenient for us to check if a receiver should be in the data plane while it is not.
    //std::cout<<m_self.GetNodeId () <<" "<< Simulator::Now ()<<" Simulator, add sending status: "<< " nodeid: "<< sendingStatus.nodeId <<" slot: "<< sendingStatus.sendingSlot << std::endl;
    Simulator::AddSendingNode (sendingStatus);

    //=====================First Check If I Can Send Packets===========================
    std::vector<uint16_t> unitedExclusionRegion;

    if ( m_nextSendingSlot <= GetCurrentSlot ())
    {
      CollectConflictingNodes (unitedExclusionRegion);
    }
    bool selfMax = false;
    if ( m_nextSendingSlot == 0 || m_nextSendingSlot < GetCurrentSlot ())  // have not sent out any packets yet.
    {
      selfMax = IsSelfMaximum (unitedExclusionRegion, GetCurrentSlot ());
    }
    if ( selfMax == true || m_nextSendingSlot == GetCurrentSlot ())
    {
      // Need next sending slot
      m_nextSendingSlot = FindNextSendingSlot (unitedExclusionRegion);
      //std::cout<<" trying to send. queueempty: "<< m_queueEmptyCallback () <<" stateidle: "<< m_phy->IsStateIdle ()<<" ersize: "<< unitedExclusionRegion.size ()<<" signalMap.size: " << m_signalMap.GetSignalMap ().size () << std::endl;
      if ( m_queueEmptyCallback () != true && m_phy->IsStateIdle ())
      {
        m_setDequeueCallback ();
        SendDataPacket ();
        //std::cout<<m_self.GetNodeId () << " is sending packet according to schedule "<< Simulator::Now () << std::endl;
      }
      return;
    }
    //=====================Then Check If I Can Receive Packets=========================
    if ( ReceiveInCurrentSlot () == true)
    {
      //std::cout<<m_self.GetNodeId () << " will be a receiver in the current slot" << std::endl;
      return;
      // Stay in Data Channel.
    }
    //Schedule Control Channel Logic. *************************************************

    Simulator::IfSelfShouldBeReceiver (m_self.GetNodeId (), GetCurrentSlot ());
    if (m_phy->IsStateIdle ())
    {
      //std::cout<<m_self.GetNodeId () <<" will schedule control packet in control channel "<<Simulator::Now ()<< std::endl;
      SetChannelNumber (CONTROL_CHANNEL);
      Simulator::Schedule (m_phy->GetObject<YansWifiPhy> ()->GetSwitchingDelay (),  
          &MacLow::ScheduleControlSignalTransmission, this);
      int64_t scheduleDelay = 1249; // Need To Test How Many MicroSeconds It Will Take
       m_setChannelEvent = Simulator::Schedule (MicroSeconds (scheduleDelay), &MacLow::SetChannelNumber, this, DATA_CHANNEL);
    }
  }

  void MacLow::SetChannelNumber (uint32_t channelNumber)
  {
    if ( m_phy != NULL && m_self.GetNodeId () != 0)
    {
      //std::cout<<" m_self: "<< m_self.GetNodeId ()<<" " << m_self <<" m_phy->GetObject: "<< m_phy->GetObject<YansWifiPhy> () << std::endl;
      m_phy->GetObject<YansWifiPhy> ()->SetChannelNumber (channelNumber);
    }
  }

  bool MacLow::IsNeighborSignalMapExisted (uint16_t neighborId)
  {
    for (std::vector<NeighborSignalMap>::iterator it = m_neighborSignalMaps.begin ();
        it != m_neighborSignalMaps.end (); ++ it)
    {
      if (it->neighborId == neighborId)
        return true;
    }
    return false;
  }

  //call  IsNeighborSignalMapExisted first
  void MacLow::CreateNeighborSignalMapRecord (uint16_t neighborId)
  {
    NeighborSignalMap neighborSignalMap;
    neighborSignalMap.neighborId = neighborId;
    m_neighborSignalMaps.push_back (neighborSignalMap);
  }


  void MacLow::UpdateNeighborSignalMapRecord (SignalMapItem item)
  {
    for (std::vector<NeighborSignalMap>::iterator it = m_neighborSignalMaps.begin ();
        it != m_neighborSignalMaps.end (); ++ it)
    {
      if ( it->neighborId == item.to)
      {
        it->signalMap.AddOrUpdate (item);
        it->signalMap.SortAccordingToAttenuation ();
        break;
      }
    }
  }

  SignalMap MacLow::GetSignalMapLocalCopy (uint16_t neighborId)
  {
    for (std::vector<NeighborSignalMap>::iterator it = m_neighborSignalMaps.begin ();
        it != m_neighborSignalMaps.end (); ++ it)
    {
      if (it->neighborId == neighborId)
      {
        return it->signalMap;
      }
    }
    SignalMap s;
    return s;
  }

  void MacLow::CollectConflictingNodes (std::vector<uint16_t> &vec)
  {
    std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap (m_self.GetNodeId ());
    std::vector<uint16_t> receivers;
    for (std::vector<SignalMapItem>::iterator it = signalMap.begin (); it != signalMap.end (); ++ it)
    {
      double rxPower = DEFAULT_POWER + TX_GAIN - it->attenuation;
      if ( rxPower >= LINK_SELECTION_THRESHOLD)
      {
        receivers.push_back (it->from); // since its deterministic channel, we can do this
      }  
    }
    //std::cout<<" receivers vector size: "<< receivers.size ()<< std::endl;
    // neighbors of recievers should be considered. not neighbors of the sender
    for (std::vector<uint16_t>::iterator it = receivers.begin (); it != receivers.end (); ++ it)
    {
      std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap (*it);
      for (std::vector<SignalMapItem>::iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
      {
        if ( _it->from == m_self.GetNodeId () )
          continue;
        bool conflicting = false;
        conflicting = CheckIfTwoNodesConflict (m_self.GetNodeId (), _it->from);
        if ( conflicting == true)
        {
          if ( find (vec.begin (), vec.end (), _it->from) == vec.end ())
            vec.push_back (_it->from);
        }
        // bi-directional exclusion region
        conflicting = CheckIfTwoNodesConflict (_it->from, m_self.GetNodeId ());
        if ( conflicting == true)
        {
          if ( find (vec.begin (), vec.end (), _it->from) == vec.end ())
            vec.push_back (_it->from);
        }
      }
    }
  }

  std::vector<RoadVehicleItem> MacLow::CalculateLengthForDensityShare (std::vector<RoadVehicleItem> vec, 
      uint32_t &count, uint32_t remainBytes, uint32_t &totalBytes)
  {
    totalBytes = 1; //the first byte is used to write @count.
    count = 0;
    std::vector<RoadVehicleItem> _vec;
    for (std::vector<RoadVehicleItem>::iterator it = vec.begin (); it != vec.end (); ++ it)
    {
      if (it->density != 0 && totalBytes <= remainBytes)
      {
        totalBytes += 8 + 1 + it->edge.edgeId.size (); // 8 is the density (double), 1 is the length of edgeId
        _vec.push_back (*it);
        count ++;
      }
    }
    return _vec;
  }

  void MacLow::GetNodesInExclusionRegion (uint16_t node, double exclusionRegion, std::vector<uint16_t> &vec)
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

  bool MacLow::CheckIfTwoNodesConflict (uint16_t sender, uint16_t neighbor)
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

      double exclusionRegion = m_exclusionRegionHelper.GetExclusionRegion (sender, *it);
      std::vector<uint16_t> nodesInExclusionRegion;
      GetNodesInExclusionRegion ( *it, exclusionRegion, nodesInExclusionRegion); // find out who are in Exclusion Region
      if ( find (nodesInExclusionRegion.begin (), nodesInExclusionRegion.end (), neighbor) != nodesInExclusionRegion.end ())
      {
        return true;
      }
      /*
      for (std::vector<SignalMapItem>::iterator _it = signalMap.begin (); _it != signalMap.end (); ++ _it)
      {
        if ( _it->from == sender && _it->to == *it)
        {
          double exclusionRegion = _it->exclusionRegion; // the link Exclusion region is found.
          // need a method to fetch nodes in exclusion region. 
          std::vector<uint16_t> nodesInExclusionRegion;
          GetNodesInExclusionRegion ( *it, exclusionRegion, nodesInExclusionRegion); // find out who are in Exclusion Region
          if ( find (nodesInExclusionRegion.begin (), nodesInExclusionRegion.end (), neighbor) != nodesInExclusionRegion.end ())
          {
            return true;
          }
        }
      }
      */
    }
  }

  std::vector<uint16_t> MacLow::CollectConflictNeighbors ()
  {
    std::vector<uint16_t> vec;
    std::vector<SignalMapItem> signalMap = Simulator::GetSignalMap (m_self.GetNodeId ()); // get receiver signal map
    for (std::vector<SignalMapItem>::iterator it = signalMap.begin (); it != signalMap.end (); ++ it)
    {
      if ( CheckIfTwoNodesConflict (it->from, it->to) == true)
      {
        vec.push_back (it->from);
      }
    }
    vec.push_back (m_self.GetNodeId ()); // m_self.GetNodeId () == it->to
    return vec;
  }

  int64_t MacLow::FindNextSendingSlot (std::vector<uint16_t> exclusionRegion)
  {
    int64_t slot = GetCurrentSlot () + SLOT_OFFSET_FOR_FUTURE_CALCULATION; // offset is 100 slots
    while (true)
    {
      if (IsSelfMaximum (exclusionRegion, slot) == true)
      {
        return slot;
      }
      slot += 1;
    }
  }

  void MacLow::SenseChannelAndSend ()
  {
    int64_t maxPropagationDelay = 1000;
    if ( m_phy->IsStateIdle ())
    {
      GenerateControlPacket ();
      //std::cout<< m_self.GetNodeId ()<<" control signal, sending "<< std::endl;
      Simulator::Schedule (NanoSeconds (maxPropagationDelay), &MacLow::SendDataPacket, this);
    }
  }

  void MacLow::ScheduleControlSignalTransmission ()
  {
    int64_t seed =GetCurrentSlot ()  * 10000 + m_self.GetNodeId ();
    srand (seed);
    int64_t backoffTime = 50000;
    backoffTime = rand () % backoffTime;
    Simulator::Schedule (NanoSeconds (abs (backoffTime)), &MacLow::SenseChannelAndSend, this);
  }

  void MacLow::UpdateSendingStatus (uint16_t node, int64_t slot)
  {
    for (std::vector<NodeSendingStatus>::iterator it = m_nodesSendingStatus.begin (); it != m_nodesSendingStatus.end (); ++ it)
    {
      if (it->nodeId == node)
      {
        it->sendingSlot = slot;
        return;
      }
    }
    NodeSendingStatus item;
    item.nodeId = node;
    item.sendingSlot = slot;
    m_nodesSendingStatus.push_back (item);
  }

  void MacLow::SetDequeueCallback (VoidCallback callback)
  {
    m_setDequeueCallback = callback;
  }

  void MacLow::GenerateControlPacket ()
  {
    uint8_t * payload = new uint8_t[DEFAULT_PACKET_LENGTH];
    PayloadBuffer buff = PayloadBuffer (payload);
    double txPower = 0;
    double rxPower = 0;
    buff.WriteDouble (txPower); // will fill in exact value at PHY layer before transmission
    buff.WriteDouble (rxPower); // will fill in axact value at PHY layer after reception
    buff.WriteDouble (m_angle);
    buff.WriteDouble (m_positionX);
    buff.WriteDouble (m_positionY);
    Ptr<Packet> pkt = Create<Packet> (payload, DEFAULT_PACKET_LENGTH);
    m_setListenerCallback ();

    WifiMacHeader hdr;
    hdr.SetAddr2 (m_self);
    hdr.SetAddr1 (Mac48Address::GetBroadcast ());
    hdr.SetDsNotTo ();
    hdr.SetDsNotFrom ();
    hdr.SetFragmentNumber (0);
    hdr.SetNoRetry ();
    hdr.SetTypeData ();
    MacLowTransmissionParameters params;
    params.DisableAck ();
    params.DisableRts ();
    params.DisableOverrideDurationId ();
    params.DisableNextData ();
    m_currentPacket = pkt;
    m_currentHdr = hdr;
    m_setPacketCallback (hdr, pkt);
    m_txParams = params;

    //mac->Enqueue (pkt, addr1);
    delete [] payload;
  }

  bool MacLow::ReceiveInCurrentSlot () //Using Signal Map And Nodes Sending Status
  {
    //Assuming Symmetric Channel Attenuation For Now.
    bool returnValue=false;
    std::vector<SignalMapItem> signalMap= Simulator::GetSignalMap (m_self.GetNodeId ());
    std::vector<uint16_t> senders;
    //Find out who are the receivers first.
    for (std::vector<SignalMapItem>::iterator it = signalMap.begin (); it != signalMap.end (); ++ it)
    {
      double rxPower = DEFAULT_POWER + TX_GAIN - it->attenuation;
      //std::cout<<" dist: "<< Simulator::GetDistanceBetweenTwoNodes (m_self.GetNodeId (), it->from)<<" rxpower: "<< rxPower<< " atten: "<< it->attenuation <<" default: "<< DEFAULT_POWER <<" txgain: "<< TX_GAIN<< std::endl;
      if ( rxPower >= LINK_SELECTION_THRESHOLD)
      {
        senders.push_back (it->from);
        //std::cout<<" been viewed as sender "<< std::endl;
      }
    }
    for (std::vector<uint16_t>::iterator it = senders.begin (); it != senders.end (); ++ it)
    {
      //std::cout<<" m_nodesSendingStatus.size: "<<m_nodesSendingStatus.size ()<< std::endl;
      for (std::vector<NodeSendingStatus>::iterator _it = m_nodesSendingStatus.begin (); _it != m_nodesSendingStatus.end (); ++ _it)
      {
        if ( _it->nodeId == *it)
        {
          //std::cout<<" sending slot: "<< _it->sendingSlot <<" current: "<< GetCurrentSlot ()<< std::endl;
          if (_it->sendingSlot == GetCurrentSlot ())
          {
            return true;
          }
        }
      }
    }
    return false;
  }

  void MacLow::SortSendingSlot ()
  {
    sort (m_nodesSendingStatus.begin (), m_nodesSendingStatus.end (), SendSlotCompare);
  }

  std::vector<NodeSendingStatus> MacLow::GetFirstTwoNodeSendingSlot (int64_t currentSlot)
  {
    std::vector<NodeSendingStatus> vec;
    for (std::vector<NodeSendingStatus>::iterator it = m_nodesSendingStatus.begin (); it != m_nodesSendingStatus.end (); ++ it)
    {
      //std::cout<<m_self.GetNodeId ()<<" nodeid: "<< it->nodeId <<" sendingslot: "<< it->sendingSlot << std::endl;
      if ( it->sendingSlot <= currentSlot)
      {
        continue;
      }
      else if ( m_signalMap.DistanceToNeighbor ( it->nodeId, m_positionX, m_positionY) <= MAX_LINK_DISTANCE)       
        //slot condition is met. need to check distance to sender.
      {
        vec.push_back (*it);
        if (vec.size () == 2)
          return vec;
      }
    }
    return vec;
  }

  void MacLow::PrintAddress ()
  {
    std::cout<<" printing node address: "<< m_self.GetNodeId () <<" joins the network this: "<< this << std::endl;
  }
} // namespace ns3
