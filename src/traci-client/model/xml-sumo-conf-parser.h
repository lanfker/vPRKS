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
 *
 * @file xml-sumo-conf-parser.h
 * @date Apr 19, 2010
 *
 * @author Yoann Pigné
 */

#ifndef XMLVEHICLELISTPARSER_H_
#define XMLVEHICLELISTPARSER_H_

#include<iostream>
#include<vector>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/DocumentHandler.hpp>
#include <xercesc/sax/DTDHandler.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include<xercesc/sax2/SAX2XMLReader.hpp>
#include<xercesc/sax2/DefaultHandler.hpp>
#include<xercesc/sax2/XMLReaderFactory.hpp>
#include<xercesc/sax/AttributeList.hpp>

#include<xercesc/sax2/Attributes.hpp>
namespace ns3
{

  using namespace std;
  using namespace xercesc;

  //class AttributeList;

  class XMLSumoConfParser : public DefaultHandler
  {

    public:
      XMLSumoConfParser();
      virtual void startElement
        (
         const   XMLCh* const    uri,
         const   XMLCh* const    localname,
         const   XMLCh* const    qname,
         const   Attributes&     attrs
        );
      void
        fatalError(const SAXParseException&);



      static void parseConfiguration(const string & filename);



      std::string net_file_name;
      static int32_t port;
      //double * boundaries;

      bool is_location;
      bool is_net_file_name;
      bool is_port;
  };

}
#endif /* XMLVEHICLELISTPARSER_H_ */

