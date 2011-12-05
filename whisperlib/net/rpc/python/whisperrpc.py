#!/usr/bin/python
#
# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import json
import httplib
import time
import datetime
import base64

def JsonEncode(p):
    if ( isinstance(p, datetime.datetime) ):
        return json.dumps(time.mktime(p.timetuple()))
    if ( isinstance(p, list) ):
        return '[ %s ]' % ','.join([JsonEncode(x) for x in p])
    try:
        return json.dumps(p)
    except ValueError:
        return p.JsonEncode()
    except TypeError:
        return p.JsonEncode()
    # endtry
# enddef

class RpcError(Exception):
    def __init__(self, reason):
        Exception('RPC Error', reason)

class RpcConnection:
    STATE_DISCONNECTED = 0
    STATE_CONNECTED = 1

    RPC_CALL = 0
    RPC_REPLY = 1

    # RPC reply codes
    RPC_SUCCESS         = 0
    RPC_SERVICE_UNAVAIL = 1
    RPC_PROC_UNAVAIL    = 2
    RPC_GARBAGE_ARGS    = 3
    RPC_SYSTEM_ERR      = 4
    RPC_CONN_CLOSED     = 5

    def __init__(self, host, port=None, strict=False, timeout=10,
                 user=None, passwd=None, retries=3):
        self.__host = host
        self.__host = port
        self.__strict = strict
        self.__timeout = timeout
        self.__user = user
        self.__passwd = passwd
        self.__retries = retries
        self.__conn = httplib.HTTPConnection(host, port, strict, timeout)
        self.__state = RpcConnection.STATE_DISCONNECTED
        self.__next_xid = 0
    # enddef

    def Request(self, url_base, service, method, params):
        # prepare the request
        retry = 0
        self.__next_xid += 1
        if params == '{}':
            p = ''
        else:
            p = params
        # endif

        # Holly crap - the order is important..
        json_request = '{"header" : { "xid" : %s , "msgType" : %s} , "cbody" : { "service" : "%s" , "method" : "%s" , "params" : [%s]} }' % (
            self.__next_xid,
            RpcConnection.RPC_CALL,
            service,
            method,
            p)
        response = None
        while response is None and retry < self.__retries:
            try:
                if self.__state == RpcConnection.STATE_DISCONNECTED:
                    self.__conn.connect()
                    self.__state = RpcConnection.STATE_CONNECTED
                headers = { 'Rpc_Codec_Id': 2,
                            'Keep-Alive' :  300 }
                if self.__user is not None:
                    if self.__passwd is None:
                        auth = '%s:' % (self.__user)
                    else:
                        auth = '%s:%s' % (self.__user, self.__passwd)
                    headers['Authorization'] = \
                        'Basic %s' % base64.b64encode(auth)
                # endif
                req = self.__conn.request('POST', url_base, json_request, headers)
                answer = self.__conn.getresponse()
                response = answer.read()
            except httplib.HTTPException, e:
                retry += 1
                if retry >= self.__retries:
                    raise e
                self.__conn.close()
                self.__conn = httplib.HTTPConnection(host, port, strict, timeout)
                self.__state = RpcConnection.STATE_DISCONNECTED
            # endtry
        # endwhile
        if response is None:
            raise RpcError('Invalid response from server')
        # endif
        try:
            parsed_response = json.loads(response)
        except:
            raise RpcError('Invalid JSON response from server')
        # endtry

        if not isinstance(parsed_response, dict):
            raise RpcError('Non dict (i.e. garbage) response from server')
        # endif
        if not parsed_response.has_key('rbody'):
            raise RpcError('No rbody found in server response (i.e. garbage)')
        # endif
        rbody = parsed_response['rbody']
        if  not isinstance(rbody, dict):
            raise RpcError('Wrong rbody found in server response (i.e. garbage)')
        # endif
        if not rbody.has_key('replyStatus') and not rbody.has_key('result'):
            raise RpcError('Wrong keys in rbody from server response (i.e. garbage)')
        # endif
        replyStatus = rbody['replyStatus']
        result = rbody['result']
        if not isinstance(replyStatus, int):
            raise RpcError('Strange reply status type receive from server: %s' % type(replyStatus))
        # endif
        if replyStatus != 0:
            raise RpcError('Server reported error: %s' % replyStatus)
        # endif

        ##
        ## Finally a good result :)
        ##
        return result
    # enddef
# endclass
