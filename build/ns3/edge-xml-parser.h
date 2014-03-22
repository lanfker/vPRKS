
#ifndef EDGE_XML_PARSER_H
#define EDGE_XML_PARSER_H

#include <string>
#include <vector>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include "ns3/node-xml-parser.h"

namespace ns3 {
  using namespace std;
  using namespace xercesc;
  typedef struct RoadMapEdge
  {
    string edgeId;
    RoadMapNode from;
    RoadMapNode to;
  }RoadMapEdge;

  class EdgeXmlParser : public DefaultHandler
  {
    public:
      EdgeXmlParser ();
      ~EdgeXmlParser ();
      void startElement (
          const XMLCh* const uri,
          const XMLCh* const localname,
          const XMLCh* const qname,
          const Attributes& attrs
          );
      virtual void fatalError (const SAXParseException& );
      void characters (const XMLCh* const, const unsigned int);

      static void ParseEdgeXmlFile (const string & filename);

      static vector<RoadMapEdge> m_mapEdges;
  };
}


#endif
