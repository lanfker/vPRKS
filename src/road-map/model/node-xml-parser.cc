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

#include "ns3/log.h"

namespace ns3{
  NS_LOG_COMPONENT_DEFINE ("Node_Xml_Parse");
  using namespace std;
  using namespace xercesc;

  vector<RoadMapNode> NodeXmlParser::m_mapNodes;
  NodeXmlParser::NodeXmlParser ()
  {
  }

  NodeXmlParser::~NodeXmlParser ()
  {
  }

  void NodeXmlParser::ParseNodeXmlFile (const string & filename)
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

    NodeXmlParser* defaultHandler = new NodeXmlParser ();
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

  void NodeXmlParser::fatalError (const SAXParseException& exception)
  {
    char* message = XMLString::transcode(exception.getMessage ());
    std::cout<<"Fatal error: "<< message <<" at line: "<< exception.getLineNumber () << std::endl;
    XMLString::release (&message);
  }

  void NodeXmlParser::startElement (const XMLCh* const uri, const XMLCh* const localname, 
      const XMLCh* const qname, const Attributes& attrs)
  {
    char* name = XMLString::transcode (localname);
    //std::cout<<" name is: "<< name << std::endl;
    if (strcmp("node", name) == 0)
    {
      RoadMapNode node;
      //std::cout<<" name is!: "<< name << std::endl;
      XMLCh* attrName = XMLString::transcode("id");
      char* value = XMLString::transcode(attrs.getValue(attrName));
      node.nodeId = value;
      //std::cout<<" id is: "<< value << std::endl;
      attrName = XMLString::transcode ("x");
      value = XMLString::transcode (attrs.getValue (attrName));
      node.xCoordinate = atof (value);
      attrName = XMLString::transcode ("y");
      value = XMLString::transcode (attrs.getValue (attrName));
      node.yCoordinate = atof (value);
      XMLString::release (&attrName);
      XMLString::release (&value);
      NodeXmlParser::m_mapNodes.push_back (node);
      //std::cout<<"node size: "<< NodeXmlParser::m_mapNodes.size () << std::endl;
    }
    XMLString::release (&name);
  }
}
