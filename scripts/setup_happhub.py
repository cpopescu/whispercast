#!/usr/bin/python

import os
import os.path
import sys
import argparse
import shutil

def createDir(path):
  print "Create dir:", path
  if not os.path.exists(path):
    os.makedirs(path)

def removeDir(path):
  print "Remove dir:", path
  if os.path.isdir(path):
    shutil.rmtree(path)

def removeFile(path):
  print "Remove file:", path
  if os.path.exists(path):
    os.remove(path)

def paramLine(pname, pvalue):
  return "--" + pname + "=" + str(pvalue) + "\n"

def writeParam(f, config, p):
  f.write(paramLine(p['name'], config.get("whispercast", p['name'])))

def clear(ws_home, service_name):
  removeDir(os.path.join(ws_home, "etc", "ini", service_name))
  removeDir(os.path.join(ws_home, "etc", "config", service_name))
  removeDir(os.path.join(ws_home, "var", "media", service_name))
  removeDir(os.path.join(ws_home, "var", "state", service_name))
  removeDir(os.path.join(ws_home, "var", "stats", service_name))
  removeDir(os.path.join(ws_home, "var", "log", service_name))
  removeDir(os.path.join(ws_home, "var", "media", service_name))
  removeFile(os.path.join(ws_home, "etc", "services", service_name + ".ini"))

def main():
  parser = argparse.ArgumentParser(description='Creates configuration dirs & files for the specified service.',
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter) # to print the default values
  parser.add_argument('--ws_home', metavar='directory',
                      help='The directory where whispercast is installed.',
                      default='~/whispercast')
  parser.add_argument('--service',
                      help='The service name to be installed/cleared. e.g. happhub, radiolynx',
                      required=True)
  parser.add_argument('--clear',
                      help='Remove service config/state/stats/logs, without media',
                      action='store_true')
  parser.add_argument('--http_port', metavar='port', type=int,
                      help='HTTP port',
                      default=8080)
  parser.add_argument('--rpc_http_port', metavar='port', type=int,
                      help='RPC through HTTP port',
                      default=8180)
  parser.add_argument('--rtmp_port', metavar='port', type=int,
                      help='RTMP port',
                      default=1935)
  parser.add_argument('--authorization_realm',
                      help="Whispercast authorization realm",
                      default='whispercast')
  parser.add_argument('--admin_passwd', metavar='pass',
                      help="RPC throught HTTP password",
                      required=False)
  parser.add_argument('--rtmp_max_num_connections', metavar='N', type=int,
                      help='The maximum number of connections on RTMP',
                      default=4000)
  parser.add_argument('-f', '--force',
                      help='No questions',
                      action='store_true')

  args = parser.parse_args()
  print "Args:"
  for k,v in vars(args).iteritems():
    print " ",k,":",v
  print ""

  ws_home = os.path.expanduser(args.ws_home)
  
  #print "# Ignoring ws_home:", ws_home
  #ws_home = "/tmp/whispercast"
  #print "# Using new ws_home:", ws_home
  #print ""

  # check ws_home sanity
  ws_exe = os.path.join(ws_home, "bin", "whispercast")
  if not os.path.exists(ws_exe):
    print "Whispercast executable not found:", ws_exe
    print "Probably wrong ws_home parameter:", ws_home
    return 1

  service_name = args.service

  if args.clear:
    print "Deleting "+service_name+" config/stats/state/logs.. from", ws_home
    if 'y' == raw_input("Continue?(y/n)"): clear(ws_home, service_name)
    else: print "Abort"
    return 0

  if args.admin_passwd is None:
    print "Error: Missing required parameter: --admin_passwd"
    return 1

  whispercast_ini = os.path.join(ws_home, "etc", "ini", service_name, "whispercast.ini")
  if not args.force and os.path.exists(whispercast_ini):
    print "File: " + whispercast_ini + " already exists. Stopping"
    return 1

  print "### Creating directories.."
  createDir(os.path.join(ws_home, "etc", "ini", service_name))
  createDir(os.path.join(ws_home, "etc", "config", service_name))
  createDir(os.path.join(ws_home, "var", "media", service_name))
  createDir(os.path.join(ws_home, "var", "state", service_name))
  createDir(os.path.join(ws_home, "var", "stats", service_name))
  createDir(os.path.join(ws_home, "var", "log", service_name))
  createDir(os.path.join(ws_home, "var", "media", service_name))

  print "Create file:", whispercast_ini
  with open(whispercast_ini, "w") as f:
    f.write(paramLine("rpc_http_port", args.rpc_http_port))
    f.write(paramLine("http_port", args.http_port))
    f.write(paramLine("rtmp_port", args.rtmp_port))
    f.write(paramLine("base_media_dir", os.path.join(ws_home, "var", "media", service_name)))
    f.write(paramLine("media_state_dir", os.path.join(ws_home, "var", "state", service_name)))
    f.write(paramLine("media_config_dir", os.path.join(ws_home, "etc", "config", service_name)))
    f.write(paramLine("admin_passwd", args.admin_passwd))
    f.write(paramLine("authorization_realm", args.authorization_realm))
    f.write(paramLine("rpc_js_form_path", os.path.join(ws_home, "var", "www", "js", "rpc")))
    f.write(paramLine("element_libraries_dir", os.path.join(ws_home, "modules")))
    f.write(paramLine("rtmp_max_num_connections", args.rtmp_max_num_connections))
    f.write(paramLine("rtmp_connection_seek_processing_delay_ms", "500"))
    f.write(paramLine("rtmp_connection_media_chunk_size_ms", "0"))
    f.write(paramLine("media_aio_buffer_pool_size", "800"))
    f.write(paramLine("media_aio_buffer_size", "256"))
    f.write(paramLine("served_id", service_name))
    f.write(paramLine("stat_collection_interval_ms", "5000"))
    f.write(paramLine("log_stats_dir", os.path.join(ws_home, "var", "stats", service_name)))
    f.write(paramLine("log_stats_file_base", "whispercast"))
    f.write(paramLine("rtmp_connection_send_buffer_size", "65536"))
    f.write(paramLine("enable_libav_mts", "true"))

  whispercast_config = os.path.join(ws_home, "etc", "config", service_name, "whispercast.config")
  print "Create file:", whispercast_config
  with open(whispercast_config, "w") as f:
    f.write("{}[][]\n")

  happhub_ini = os.path.join(ws_home, "etc", "services", service_name+".ini")
  createDir(os.path.join(ws_home, "etc", "services"))
  print "Create service file:", happhub_ini
  with open(happhub_ini, "w") as f:
    f.write("whispercast\n")

  print "Success"
  return 0

if __name__ == "__main__":
  sys.exit(main())

