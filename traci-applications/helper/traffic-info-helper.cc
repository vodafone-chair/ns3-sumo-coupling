#include "traffic-info-helper.h"
#include "ns3/traffic-info.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

TrafficInfoServerHelper::TrafficInfoServerHelper (uint16_t port)
{
  m_factory.SetTypeId (TrafficInfoServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
TrafficInfoServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TrafficInfoServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrafficInfoServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrafficInfoServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TrafficInfoServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TrafficInfoServer> ();
  node->AddApplication (app);

  return app;
}

TrafficInfoClientHelper::TrafficInfoClientHelper (uint16_t port)
{
  m_factory.SetTypeId (TrafficInfoClient::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void 
TrafficInfoClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TrafficInfoClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrafficInfoClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TrafficInfoClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TrafficInfoClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TrafficInfoClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
