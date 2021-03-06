#!/usr/bin/python

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

# dojo json exchange format see:
# http://dojotoolkit.org/reference-guide/1.10/dojo/data/ItemFileReadStore.html#input-data-format

import os, sys, json, re
import urlparse
from datetime import datetime

import settings
import journal


def application(environ, start_response):
  #print >> sys.stderr, 'environ="%s"\n' % environ
  path = environ.get('PATH_INFO', '')

  params = dict(urlparse.parse_qsl(environ.get('QUERY_STRING', '')))
  #print >> sys.stderr, 'params="%s"\n' % params
  
  if path == "/phones":
    return settings.handle_phones(environ, start_response, params)
  if path == "/online_credentials":
    return settings.handle_online_credentials(environ, start_response, params)
  if path == "/get_list":
    return settings.handle_get_list(environ, start_response, params)
  if path == "/get_lists":
    return settings.handle_get_lists(environ, start_response, params)
 
  if path == "/callerlog":
    return journal.handle_callerlog(environ, start_response, params)
  if path == "/journal":
    return journal.handle_journal(environ, start_response, params)
 
  # return error
  start_response('404 NOT FOUND', [('Content-Type', 'text/plain')])
  return ['Not Found']


if __name__ == '__main__':
  from flup.server.fcgi import WSGIServer 
  WSGIServer(application).run()

