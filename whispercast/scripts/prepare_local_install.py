#!/usr/bin/python
#
# Simple script to prepare flags file for a local example of whispercast
#

import sys
import os

FLAGS =  """
--http_port=8880
--rpc_http_port=8881
--rtmp_port=8935
--base_media_dir=%(install_dir)s/media/www/
--media_state_dir=%(install_dir)s/var/state/demo/
--media_config_dir=%(install_dir)s/etc/config/demo/
--admin_passwd=test
--authorization_realm=whispercast
--rpc_js_form_path=%(install_dir)s/media/www/js/rpc/
--element_libraries_dir=%(install_dir)s/modules/
--http_extensions_to_content_types_file=%(install_dir)s/etc/config/extension2content_type
--media_path=/
--loglevel=3
"""

# Possible other flags:
# --rtmp_handshake_library=%(install_dir)s/whispercast/modules/libwhisper_streamlib_rtmp_extra.so
# --alsologtostderr

START_SCRIPT = """#!/bin/sh

echo "=================="
echo ""
echo "Starting whispercast -> Point your browser to http://localhost:8880/"
echo ""
echo "=================="
%(install_dir)s/bin/shell %(install_dir)s/bin/whispercast --flagfile=%(install_dir)s/etc/ini/demo/whispercast.ini
"""

if __name__ == "__main__":
    install_dir = sys.argv[1]
    output_dir =  sys.argv[2]
    open("%s/whispercast.ini" % output_dir, "w").write(
        FLAGS % { "install_dir" : install_dir })
    open("%s/start_whispercast.sh" % output_dir, "w").write(
        START_SCRIPT % { "install_dir" : install_dir })
    os.chmod("%s/start_whispercast.sh" % output_dir, 0755);
