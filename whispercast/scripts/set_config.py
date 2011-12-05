#!/usr/bin/python

from rpc.whisperrpc import *
from rpc.factory import *
from rpc.standard_library import *

if __name__ == '__main__':
    params = {
        'remote_host' : '127.0.0.1',
        'remote_port' : 8881,
        'admin_pass' : 'test',
        }
    rpc_conn = RpcConnection(
        params.get('remote_host'), port = params.get('remote_port'),
        user = 'admin', passwd = params.get('admin_pass'))

    global_service = RpcService_MediaElementService(
        rpc_conn, "/rpc-config")
    standardlib_service = RpcService_StandardLibraryService(
        rpc_conn, '/rpc-config/standard_library')

    # Deleting old elements and exports:
    global_service.DeleteElementSpec("files")
    global_service.DeleteElementSpec("normalizer")
    global_service.DeleteElementSpec("mp3")
    global_service.DeleteElementSpec("example")
    global_service.DeleteElementSpec("example_flv")
    global_service.DeleteElementSpec("example_mp4")
    global_service.DeleteElementSpec("livestream")
    global_service.DeleteElementSpec("f4v2flv")
    global_service.DeleteElementSpec("normalize")

    global_service.StopExportElement('http', '/')
    global_service.StopExportElement('http', '/mp3')
    global_service.StopExportElement('http', '/example')
    global_service.StopExportElement('rtmp', 'example')
    global_service.StopExportElement('rtmp', 'livecam')
    global_service.StopExportElement('http', '/whispercast/livecam')

    # Create new elements:
    ret = standardlib_service.AddAioFileElementSpec(
        "files", True, False,
        AioFileElementSpec(_media_type_ = 'raw',
                           _home_dir_ = '',
                           _file_pattern_ = '',
                           _default_index_file_ = 'index.html'))
    print "File source added: %s" % ret

    ret = standardlib_service.AddAioFileElementSpec(
        "mp3", True, False,
        AioFileElementSpec(_media_type_ = 'mp3',
                           _home_dir_ = 'mp3',
                           _file_pattern_ = '\\.mp3$'))
    print "File source added: %s" % ret

    ret = standardlib_service.AddAioFileElementSpec(
        "example_flv", True, False,
        AioFileElementSpec(_media_type_ = 'flv',
                           _home_dir_ = 'media',
                           _file_pattern_ = '\\.flv$'))
    print "File source added: %s" % ret

    ret = standardlib_service.AddAioFileElementSpec(
        "example_mp4", True, False,
        AioFileElementSpec(_media_type_ = 'f4v',
                           _home_dir_ = 'media',
                           _file_pattern_ = '4v$'))
    print "File source added: %s" % ret

    ret = standardlib_service.AddLoadBalancingElementSpec(
        "example", True, False,
        LoadBalancingElementSpec(_sub_elements_ = ["example_flv",
                                                   "f4v2flv/example_mp4"]))
    print "File source added: %s" % ret

    ret = standardlib_service.AddRtmpPublishingElementSpec(
        "livestream", True, False,
        RtmpPublishingElementSpec(
            [RtmpPublishingElementDataSpec(_name_ = "live1",
                                           _media_type_ = "flv"),
             ]));
    print "File source added: %s" % ret

    ret = standardlib_service.AddNormalizingElementSpec(
        "normalizer", True, False,
        NormalizingElementSpec())
    print "Normalizer added: %s" % ret

    ret = standardlib_service.AddF4vToFlvConverterElementSpec(
        "f4v2flv", True, False,
        F4vToFlvConverterElementSpec())
    print "F4v2Flv added: %s" % ret


    # Export them
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "files",
                          _protocol_ = "http",
                          _path_ = "/",
                          _enable_buffer_flow_control_ = True))
    print "Export added: %s" % ret
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "example",
                          _protocol_ = "http",
                          _path_ = "/example",
                          _content_type_ = "video/x-flv",
                          _enable_buffer_flow_control_ = True))
    print "Export added: %s" % ret
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "normalizer/example",
                          _protocol_ = "rtmp",
                          _path_ = "example",
                          _enable_buffer_flow_control_ = True))
    print "Export added: %s" % ret
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "normalizer/mp3",
                          _protocol_ = "http",
                          _path_ = "/mp3",
                          _content_type_ = "audio/mpeg",
                          _enable_buffer_flow_control_ = True))
    print "Export added: %s" % ret
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "livestream",
                          _protocol_ = "rtmp",
                          _path_ = "livecam",
                          _content_type_ = "flv",
                          _enable_buffer_flow_control_ = False))
    print "Export added: %s" % ret
    ret = global_service.StartExportElement(
        ElementExportSpec(_media_name_ = "livestream",
                          _protocol_ = "http",
                          _path_ = "/whispercast/livecam",
                          _content_type_ = "flv",
                          _enable_buffer_flow_control_ = False))
    print "Export added: %s" % ret


    # Save config
    ret = global_service.SaveConfig()
    print "Saved : %s" % ret
