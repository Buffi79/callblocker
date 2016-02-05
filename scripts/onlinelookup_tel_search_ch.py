#!/usr/bin/env python

# callblocker - blocking unwanted calls from your home phone
# Copyright (C) 2015-2015 Patrick Ammann <pammann@gmx.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

from __future__ import print_function
import os, sys, argparse, re
import urllib, urllib2
from BeautifulSoup import BeautifulSoup
import json


g_debug = False

def error(*objs):
  print("ERROR: ", *objs, file=sys.stderr)
  sys.exit(-1)

def debug(*objs):
  if g_debug: print("DEBUG: ", *objs, file=sys.stdout)
  return

def fetch_url(url):
  debug("fetch_url: '" + str(url)+"'")
  data = urllib2.urlopen(url, timeout = 5)
  return data.read()


def getStringElement(output, element):
  if output.find(element) < 0:
    return ""
  idxNameStart = output.find(element) + len (element) + 1
  idxNameEnd = output.index("/"+element, idxNameStart) -1
  item = output[idxNameStart:idxNameEnd]
  return str(item)
  
def doSearch(phonenr, onlyCompany, key):
    suffix = "&lang?de"
    if (onlyCompany):
        suffix  = suffix + "privat=0"
    url = "http://tel.search.ch/api/?was="+phonenr+"&key="+key+"&lang?de"
    #logger.info (url)
    outbin = fetch_url(url)
	
    #logger.info(outbin)
    output = outbin.decode('utf-8')
    return output  

	
def lookup_number(phonenr, key):
  i = 0
  onlyCompany = False
  result = ""
  while i <= 4:
    a = str(phonenr)
    #debug("NR:"+a)
    b = a[0:12-i]
    debug("try Nr:"+b)
    if i > 0:
      onlyCompany = True
      
    output = doSearch(b, onlyCompany, key)
    anzResult = int(getStringElement(output,"openSearch:totalResults"))
    #debug("Res:"+str(anzResult))      
    if anzResult > 0:
      result = result +" "+getStringElement(output, "tel:name")
      result = result +" "+getStringElement(output,"tel:firstname")
      result = result +" "+getStringElement(output,"tel:occupation")
      result = result +" "+getStringElement(output,"tel:zip")
      result = result +" "+getStringElement(output,"tel:city")
      i = 10
      #debug("query"+result)
      return result

    result = "?" + result  
    i+=1
  return result

#
# main
#
def main(argv):
  global g_debug
  parser = argparse.ArgumentParser(description="Online lookup via tel.search.ch")
  parser.add_argument("--number", help="number to be checked", required=True)
  parser.add_argument("--password", help="api key", required=True)
  parser.add_argument('--debug', action='store_true')
  args = parser.parse_args()
  g_debug = args.debug

  # map number to correct URL
  if not args.number.startswith("+41"):
    error("Not a valid Swiss number: " + args.number)

  callerName = lookup_number(args.number, args.password)

  # result in json format, if not found empty field
  result = {
    "name"  : callerName
  }
  j = json.dumps(result, encoding="utf-8", ensure_ascii=False)
  sys.stdout.write(j.encode("utf8"))
  sys.stdout.write("\n") # must be seperate line, to avoid conversion of json into ascii

if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

