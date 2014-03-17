#ifndef NODE_XML_PARSER_H
#define NODE_XML_PARSER_H

//#include <iostream>
#include <string>
#include <vector>
//#include <xercesc/sax/HandlerBase.hpp>
//#include <xercesc/sax/DocumentHandler.hpp>
//#include <xercesc/sax/DTDHandler.hpp>
//#include <xercesc/sax/EntityResolver.hpp>
//#include <xercesc/sax/ErrorHandler.hpp>
//#include <xercesc/sax/SAXParseException.hpp>
//#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
//#include <xercesc/sax2/XMLReaderFactory.hpp>
//#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax2/Attributes.hpp>

namespace ns3 {
  using namespace std;
  using namespace xercesc;
  typedef struct RoadMapNode
  {
    string nodeId;
    double xCoordinate;
    double yCoordinate;
  }RoadMapNode;

  class NodeXmlParser : public DefaultHandler
  {
    public:
      NodeXmlParser ();
      ~NodeXmlParser ();
      void startElement (
          const XMLCh* const uri,
          const XMLCh* const localname,
          const XMLCh* const qname,
          const Attributes& attrs
          );
      virtual void fatalError (const SAXParseException& );
      void characters (const XMLCh* const, const unsigned int);

      static void ParseNodeXmlFile (const string & filename);

      static vector<RoadMapNode> m_mapNodes;
  };
}


#endif
