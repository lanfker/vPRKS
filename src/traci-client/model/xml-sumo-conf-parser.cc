/**
 *
 *
 * Copyright (c) 2010-2011 University of Luxembourg
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
 * @file XMLSumoConfParser.cpp
 * @date Apr 19, 2010
 *
 * @author Yoann Pigné <yoann@pigne.org>
 */
#include "xml-sumo-conf-parser.h"
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
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iterator>
#include "ns3/log.h"

//#include<stl_vector.h>
namespace ns3{

  NS_LOG_COMPONENT_DEFINE ("XML_SUMO_CONF_PARSER");
  using namespace std;

  using namespace xercesc;

  XMLSumoConfParser::XMLSumoConfParser()
  {
    is_net_file_name = false;
    is_location = false;
    is_port=false;
  }
  int32_t XMLSumoConfParser::port = 0;

  void
    XMLSumoConfParser::startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname,
        const Attributes& attrs)
    {

      char* name = XMLString::transcode(localname);
      //cerr << "I saw element: " << name << endl;
      //string name(message);
      if (strcmp (name, "net-file") == 0)
      {

        XMLCh* q = XMLString::transcode("value");
        char* value = XMLString::transcode(attrs.getValue(q));
        if(value!=NULL){
          net_file_name = string(value);
          std::cout<<" netfile: "<< net_file_name<<std::endl;
          XMLString::release(&value);
          is_net_file_name = false;
        }
        else{
          is_net_file_name = true;
        }
      }

      if(strcmp (name, "remote-port") == 0)
      {
        is_port=true;
        XMLCh* q = XMLString::transcode("value");
        char* value = XMLString::transcode(attrs.getValue(q));
        if (value != NULL)
        {
          std::cout<<" port: "<< value <<std::endl;
          XMLSumoConfParser::port = atoi (value); // port found
        }
      }
      /*
      if (strcmp (name, "location") == 0)
      {
        XMLCh* q = XMLString::transcode("convBoundary");
        char* b = XMLString::transcode(attrs.getValue(q));

        char * pch;
        pch = strtok(b, ",");
        pch = strtok(NULL, ",");
        //boundaries[0] = atof(strtok(NULL, ","));
        //boundaries[1] = atof(strtok(NULL, ","));

        XMLString::release(&b);
        XMLString::release(&q);
      }
      */
      XMLString::release(&name);
    }

  void
    XMLSumoConfParser::fatalError(const SAXParseException& exception)
    {
      char* message = XMLString::transcode(exception.getMessage());
      cout << "Fatal Error: " << message << " at line: " << exception.getLineNumber() << endl;
      XMLString::release(&message);
    }


  void
    XMLSumoConfParser::parseConfiguration(const string & filename)
    {

      // suppose input is scratch/park05.sumocfg
      string base = filename.substr(0, filename.find_last_of('/'));
      //cout << "basename:" << base << endl; // basename is scratch
      try
      {
        XMLPlatformUtils::Initialize();
      }
      catch (const XMLException& toCatch)
      {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Error during initialization! :\n";
        cout << "Exception message is: \n" << message << "\n";
        XMLString::release(&message);
        return;
      }
      //cout<<"port: "<<*p<<endl;

      SAX2XMLReader* parser = XMLReaderFactory::createXMLReader();
      parser->setFeature(XMLUni::fgSAX2CoreValidation, false);
      parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, true); // optional

      XMLSumoConfParser* defaultHandler = new XMLSumoConfParser();
      //p = defaultHandler->port;
      //defaultHandler->boundaries = bound;
      parser->setContentHandler(defaultHandler);
      parser->setErrorHandler(defaultHandler);

      try
      {
        cout << "filename:" << filename.c_str ()<< endl;  // sumocfg file
        parser->parse(filename.c_str());


        parser->parse((base + "/" + defaultHandler->net_file_name).c_str());
        //cout << "filename2:" << (base + "/" + defaultHandler->net_file_name).c_str()<< endl;  // net.xml file
      }
      catch (const XMLException& toCatch)
      {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n" << message << "\n";
        XMLString::release(&message);
        return;
      }
      catch (const SAXParseException& toCatch)
      {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n" << message << "\n";
        XMLString::release(&message);
        return;
      }
      catch (const SAXNotRecognizedException toCatch)
      {
        char* message = XMLString::transcode(toCatch.getMessage());
        cout << "Exception message is: \n" << message << "\n";
        XMLString::release(&message);
        return;
      }
      catch (...)
      {
        cout << "Unexpected Exception ???\n";
        return;
      }

      delete parser;
      delete defaultHandler;
      XMLPlatformUtils::Terminate ();

    }
}
