/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <string>
#include <stdlib.h>

#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"

#include "traffic-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TrafficInfoApplication");

NS_OBJECT_ENSURE_REGISTERED (TrafficInfoServer);
NS_OBJECT_ENSURE_REGISTERED (TrafficInfoClient);

TypeId
TrafficInfoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficInfoServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<TrafficInfoServer> ()
    .AddAttribute ("Port", "Port on which we send packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&TrafficInfoServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Interval", 
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&TrafficInfoServer::m_interval),
                   MakeTimeChecker ())
   .AddAttribute ("MaxPackets",
					  "The maximum number of packets the application will send",
					  UintegerValue (100),
					  MakeUintegerAccessor (&TrafficInfoServer::m_count),
					  MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("Velocity", "Velocity value which is sent to client.",
	                UintegerValue (10),
	                MakeUintegerAccessor (&TrafficInfoServer::m_velocity),
	                MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Client",
	               "TraCI client for SUMO",
	               PointerValue (0),
	               MakePointerAccessor (&TrafficInfoServer::m_client),
	               MakePointerChecker<TraciClient> ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&TrafficInfoServer::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

TrafficInfoServer::TrafficInfoServer()
{
  NS_LOG_FUNCTION (this);
  m_sendEvent = EventId ();
  m_port = 0;
  m_socket = 0;
  m_velocity = 0;
  m_count = 1e9;
}

TrafficInfoServer::~TrafficInfoServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}

void
TrafficInfoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
TrafficInfoServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
	  Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
	  Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
	  Ipv4Address ipAddr = iaddr.GetBroadcast();
      InetSocketAddress remote = InetSocketAddress (ipAddr, m_port);
      m_socket->SetAllowBroadcast (true);
      m_socket->Connect (remote);

      ScheduleTransmit (Seconds (.0));
      Simulator::Schedule(Seconds(10.0), &TrafficInfoServer::ChangeSpeed, this);
}}

void 
TrafficInfoServer::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &TrafficInfoServer::Send, this);
}

void
TrafficInfoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  Simulator::Cancel (m_sendEvent);
}

void
TrafficInfoServer::Send ()
{
	NS_LOG_FUNCTION (this << m_socket);

	std::ostringstream msg; msg << std::to_string(m_velocity) << '\0';
	Ptr<Packet> packet = Create<Packet> ((uint8_t*) msg.str().c_str(), msg.str().length());

	Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
	Ipv4Address ipAddr = iaddr.GetLocal();

	m_socket->Send(packet);
	NS_LOG_INFO("Packet sent - "
			<< "[ip:" << ipAddr << "]"
			<< "[tx vel:" << m_velocity << "m/s]"
			);

	ScheduleTransmit (m_interval);
}

void
TrafficInfoServer::ChangeSpeed ()
{
	m_velocity = rand() % 60;
	Simulator::Schedule(Seconds(10.0), &TrafficInfoServer::ChangeSpeed, this);
}

TypeId
TrafficInfoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TrafficInfoClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<TrafficInfoClient> ()
    .AddAttribute ("Port", 
                   "The port on which the client will listen for incoming packets.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TrafficInfoClient::m_port),
                   MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Client",
	               "TraCI client for SUMO",
	               PointerValue (0),
	               MakePointerAccessor (&TrafficInfoClient::m_client),
	               MakePointerChecker<TraciClient> ())
  ;
  return tid;
}

TrafficInfoClient::TrafficInfoClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_port = 0;
  m_client = nullptr;
  last_velocity = -1;
}

TrafficInfoClient::~TrafficInfoClient()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}


void
TrafficInfoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
TrafficInfoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
      Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
      Ipv4Address ipAddr = iaddr.GetLocal();
      InetSocketAddress local = InetSocketAddress (ipAddr, m_port);
      m_socket->Bind (local);
      m_socket->SetRecvCallback (MakeCallback (&TrafficInfoClient::HandleRead, this));

}

void 
TrafficInfoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
TrafficInfoClient::StopApplicationNow ()
{
	NS_LOG_FUNCTION (this);
	StopApplication();
}

void
TrafficInfoClient::HandleRead (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	packet = socket->Recv ();

	uint8_t *buffer = new uint8_t[packet->GetSize ()];
	packet->CopyData(buffer, packet->GetSize ());
	std::string s = std::string((char*)buffer);
	double velocity = (double)std::stoi(s);

	Ptr<Ipv4> ipv4 = this->GetNode()->GetObject<Ipv4> ();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);
	Ipv4Address ipAddr = iaddr.GetLocal();

	NS_LOG_INFO("Packet received - "
			<< "[id:" << m_client->GetVehicleId(this->GetNode()) << "]"
			<< "[ip:" << ipAddr << "]"
			<< "[vel:" << m_client->TraCIAPI::vehicle.getSpeed(m_client->GetVehicleId(this->GetNode())) << "m/s]"
			<< "[rx vel:" << velocity << "m/s]"
			);

	if (velocity != last_velocity)
	{
		//NS_LOG_INFO("Set speed of: " << m_client->GetVehicleId(this->GetNode()) << " [" << ipAddr << "] to " << velocity << "m/s");
		m_client->TraCIAPI::vehicle.setSpeed(m_client->GetVehicleId(this->GetNode()), velocity);
		last_velocity = velocity;
	}

}

} // Namespace ns3
