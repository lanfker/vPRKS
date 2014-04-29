
#include "edge-xml-parser.h"
#include "node-xml-parser.h"
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/DocumentHandler.hpp>
#include <xercesc/sax/DTDHandler.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <cmath>

#include "ns3/log.h"

namespace ns3{
  NS_LOG_COMPONENT_DEFINE ("Edge_Xml_Parse");
  using namespace std;
  using namespace xercesc;

  vector<RoadMapEdge> EdgeXmlParser::m_mapEdges;
  EdgeXmlParser::EdgeXmlParser ()
  {
  }

  EdgeXmlParser::~EdgeXmlParser ()
  {
  }

  void EdgeXmlParser::ParseEdgeXmlFile (const string & filename)
  {
    string base = filename.substr(0, filename.find_last_of('/'));

    try {
      XMLPlatformUtils::Initialize ();
    }
    catch (const XMLException& toCatch)
    {
      char* message = XMLString::transcode(toCatch.getMessage ());
      std::cout<<"Error during initializaiton of Xercesc"<< std::endl;
      std::cout<<"Exception message is: \n"<< message << std::endl;
      XMLString::release (&message);
      return;
    }

    SAX2XMLReader* parser = XMLReaderFactory::createXMLReader ();
    parser->setFeature (XMLUni::fgSAX2CoreValidation, false);
    parser->setFeature (XMLUni::fgSAX2CoreNameSpaces, true);

    EdgeXmlParser* defaultHandler = new EdgeXmlParser ();
    parser->setContentHandler (defaultHandler);
    parser->setErrorHandler (defaultHandler);

    try
    {
      parser->parse (filename.c_str ());
    }
    catch (const XMLException& toCatch)
    {
      char* message = XMLString::transcode (toCatch.getMessage ());
      std::cout<<"Exception message is: \n"<<message << std::endl;
      XMLString::release (&message);
      return;
    }
    catch (...)
    {
      std::cout<<"Unexpected Exception"<< std::endl;
      return;
    }
    delete parser;
    delete defaultHandler;
    XMLPlatformUtils::Terminate ();
  }

  void EdgeXmlParser::fatalError (const SAXParseException& exception)
  {
    char* message = XMLString::transcode(exception.getMessage ());
    std::cout<<"Fatal error: "<< message <<" at line: "<< exception.getLineNumber () << std::endl;
    XMLString::release (&message);
  }

  void EdgeXmlParser::startElement (const XMLCh* const uri, const XMLCh* const localname, 
      const XMLCh* const qname, const Attributes& attrs)
  {
    char* name = XMLString::transcode (localname);
    //std::cout<<" name: "<< name << std::endl;
    if (strcmp("edge", name) == 0)
    {
      RoadMapEdge edge;
      XMLCh* attrName = XMLString::transcode("id");
      char* value = XMLString::transcode(attrs.getValue(attrName));
      edge.edgeId = value;
      attrName = XMLString::transcode ("from");
      value = XMLString::transcode (attrs.getValue (attrName));

      for (std::vector<RoadMapNode>::iterator it = NodeXmlParser::m_mapNodes.begin (); 
          it != NodeXmlParser::m_mapNodes.end (); ++ it)
      {
        //std::cout<<" it->nodeId: "<< it->nodeId <<" value: "<< value<< std::endl;
        if ( it->nodeId.compare (value) == 0)
        {
          //std::cout<<" strings are equal "<< std::endl;
          edge.from = *it;
          break;
        }
      }
      attrName = XMLString::transcode ("to");
      value = XMLString::transcode (attrs.getValue (attrName));
      for (std::vector<RoadMapNode>::iterator it = NodeXmlParser::m_mapNodes.begin (); 
          it != NodeXmlParser::m_mapNodes.end (); ++ it)
      {
        if ( it->nodeId.compare (value) == 0)
        {
          edge.to= *it;
          break;
        }
      }
      XMLString::release (&attrName);
      XMLString::release (&value);
      EdgeXmlParser::m_mapEdges.push_back (edge);
    }
    XMLString::release (&name);
  }

  void EdgeXmlParser::CalculateLinearModel (RoadMapEdge edge, double &a, double &b, double &length)
  {
    double x1,x2,y1,y2;
    x1 = edge.from.xCoordinate;
    x2 = edge.to.xCoordinate;
    y1 = edge.from.yCoordinate;
    y2 = edge.to.yCoordinate;

    a = (y1-y2) / (x1-x2);
    b = y1 - a * x1;
    length = sqrt ( pow (x1-x2,2) + pow (y1-y2, 2));
  }
}
