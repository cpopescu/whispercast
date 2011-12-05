#!/usr/bin/python
#
# Use this to prepare directories and ini files for a standard whispercast
# installation.
#

import sys
import os
import gflags

######################################################################

FLAGS=gflags.FLAGS

gflags.DEFINE_string('name', '',
                     'Name of the config to create')
gflags.DEFINE_string('home_dir', '%s/whispercast' % os.environ['HOME'] ,
                     'Base whispercast dir')
gflags.DEFINE_list('components', 'whispercast,whisperproc,transslave,transcoder,transmaster',
                   'Setup these components inside the config')
gflags.DEFINE_list('disks', '',
                   'The list of original media disks')
gflags.DEFINE_string('admin_pass', 'test',
                     'The password for admin things')
gflags.DEFINE_list('transslaves', '127.0.0.1',
                   'Comma separated list of ips')
gflags.DEFINE_string('scp_user', os.environ['USER'],
                     'We use this user name to transfer files '
                     'between transmaster and transslaves')

######################################################################
def MkDir(d):
    if not os.access(d, os.F_OK):
        print ' Creating dir: %s' % d
        os.makedirs(d)
    else:
        print ' Already existing dir: %s' % d
    # endif
# enddef

WHISPERCAST_INI ="""
--http_port=8080
--rtmp_port=1935
--base_media_dir=%(home_dir)s/media/%(name)s/
--media_state_dir=%(home_dir)s/var/state/%(name)s/
--media_config_dir=%(home_dir)s/etc/config/%(name)s/
--admin_passwd=%(admin_pass)s
--authorization_realm=whispercast
--rpc_js_form_path=%(home_dir)s/var/www/js/rpc/
--element_libraries_dir=%(home_dir)s/modules/
--http_extensions_to_content_types_file=%(home_dir)s/etc/config/extension2content_type
--media_path=/
--loglevel=3

--rtmp_max_num_connections=3000
--rtmp_connection_seek_processing_delay_ms=500
--rtmp_connection_media_chunk_size_ms=100
--media_aio_buffer_pool_size=1600
--disk_devices=%(disks)s
--media_aio_buffer_size=256

--server_id=%(name)s
--stat_collection_interval_ms=5000
--log_stats_dir=%(home_dir)s/var/stats/%(name)s/
--log_stats_file_base=stats
"""
def ConfigureWhispercast():
    print ' ** Setup whispercast...'
    disks = []
    i = 0
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name,
                 'admin_pass': FLAGS.admin_pass,
                 'disks': ','.join(disks)
                 }

    media_dir = '%(home_dir)s/media/%(name)s/' % conf_map
    MkDir(media_dir)
    for orig in FLAGS.disks:
        orig_dir = '%s/%s' % (orig, FLAGS.name)
        MkDir(orig_dir)
        d = '%s/media/%s/%d' % (FLAGS.home_dir, FLAGS.name, i)
        i += 1
        disks.append(d)
        if not os.access(d, os.F_OK):
            print' Linking: %s -> %s' % (orig_dir, d)
            os.symlink(orig_dir, d)
    #
    # create dirs
    log_dir = '%(home_dir)s/var/log/%(name)s/' % conf_map
    MkDir(log_dir)
    conf_dir = '%(home_dir)s/etc/config/%(name)s/' % conf_map
    MkDir(conf_dir)
    ini_dir = '%(home_dir)s/etc/ini/%(name)s/' % conf_map
    MkDir(ini_dir)
    state_dir = '%(home_dir)s/var/state/%(name)s/' % conf_map
    MkDir(state_dir)
    log_dir = '%(home_dir)s/var/stats/%(name)s/' % conf_map
    MkDir(log_dir)

    # write configs
    open('%s/hosts_aliases.config' % conf_dir, 'w').write('[]')
    open('%s/whispercast.ini' % ini_dir, 'w').write(
            WHISPERCAST_INI % conf_map)

# enddef

######################################################################

WHISPERPROC_INI="""
--base_media_dir=%(home_dir)s/media/%(name)s
--default_buildup_period_min=10
--default_buildup_delay_min=1
--min_buildup_delay_min=1

--thumbnail_processing_dir=%(home_dir)s/media/%(name)s/thumbnails
"""
def ConfigureWhisperproc():
    print ' ** Setup whisperproc...'
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name,
                 'admin_pass': FLAGS.admin_pass,
                 }
    # create dirs
    log_dir = '%(home_dir)s/var/log/%(name)s/' % conf_map
    MkDir(log_dir)
    thumbs_dir = '%(home_dir)s/media/%(name)s/thumbnails' % conf_map
    MkDir(thumbs_dir)
    ini_dir = '%(home_dir)s/etc/ini/%(name)s/' % conf_map
    MkDir(ini_dir)

    # write configs
    open('%s/whisperproc.ini' % ini_dir, 'w').write(
        WHISPERPROC_INI % conf_map)
#

######################################################################

TRANSSLAVE_INI="""
--http_port=8083
--media_download_dir=%(home_dir)s/var/data/%(name)s/transslave/download
--media_original_dir=%(home_dir)s/var/data/%(name)s/transslave/original
--media_output_dir=%(home_dir)s/media/%(name)s/0/files
--media_output_dirs=%(home_dir)s/media/%(name)s/0/files
--media_transcoder_input_dir=%(home_dir)s/var/data/%(name)s/transcoder/input
--media_transcoder_output_dir=%(home_dir)s/var/data/%(name)s/transcoder/output
--media_postprocessor_input_dir=%(home_dir)s/var/data/%(name)s/postprocessor/input
--media_postprocessor_output_dir=%(home_dir)s/var/data/%(name)s/postprocessor/output
--state_dir=%(home_dir)s/var/state/%(name)s
--state_name=transslave
--rpc_js_form_path=%(home_dir)s/media/www/js/rpc
--loglevel=4
"""

def ConfigureTransSlave():
    print ' ** Setup transslave...'
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name,
                 'admin_pass': FLAGS.admin_pass,
                 }
    # create dirs
    dirs = ['%(home_dir)s/var/data/%(name)s/transslave/download',
            '%(home_dir)s/var/data/%(name)s/transslave/original',
            '%(home_dir)s/media/%(name)s/0/files',
            '%(home_dir)s/var/data/%(name)s/transcoder/input',
            '%(home_dir)s/var/data/%(name)s/transcoder/output',
            '%(home_dir)s/var/data/%(name)s/postprocessor/input',
            '%(home_dir)s/var/data/%(name)s/postprocessor/output',
            '%(home_dir)s/var/state/%(name)s',
            '%(home_dir)s/var/log/%(name)s',
            ]
    for d in dirs:
        MkDir(d % conf_map)
    # write configs
    open('%s/etc/ini/%s/transslave.ini' % (FLAGS.home_dir,
                                           FLAGS.name), 'w').write(
        TRANSSLAVE_INI % conf_map)

######################################################################

# TODO: add admin pass !!!

TRANSCODER_INI="""
--transcode_input_dir=%(home_dir)s/var/data/%(name)s/transcoder/input
--transcode_output_dir=%(home_dir)s/var/data/%(name)s/transcoder/output
--postprocess_input_dir=%(home_dir)s/var/data/%(name)s/postprocessor/input
--postprocess_output_dir=%(home_dir)s/var/data/%(name)s/postprocessor/output

--format=default.flv:encoder=flv,audio_samplerate=44100,audio_bitrate=131072,audio_encoder=mp3,video_width=512,video_bitrate=500000
--enable_flv_postprocess
--loglevel=3
"""

def ConfigureTransCoder():
    print ' ** Setup transcoder...'
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name,
                 'admin_pass': FLAGS.admin_pass,
                 }
    # create dirs
    dirs = ['%(home_dir)s/var/data/%(name)s/transcoder/input',
            '%(home_dir)s/var/data/%(name)s/transcoder/output',
            '%(home_dir)s/var/data/%(name)s/postprocessor/input',
            '%(home_dir)s/var/data/%(name)s/postprocessor/output',
            ]
    for d in dirs:
        MkDir(d % conf_map)
    open('%s/etc/ini/%s/transcoder.ini' % (FLAGS.home_dir,
                                           FLAGS.name), 'w').write(
        TRANSCODER_INI % conf_map)

######################################################################

# TODO: add admin pass !!!

TRANSMASTER_INI="""
--http_port=8082
--input_dir=%(home_dir)s/var/data/%(name)s/transmaster/input
--output_dir=%(home_dir)s/var/data/%(name)s/transmaster/output
--process_dir=%(home_dir)s/var/data/%(name)s/transmaster/process
--slaves=%(slaves)s
--scp_username=%(scp_user)s
--state_dir=%(home_dir)s/var/state/%(name)s
--state_name=transmaster
--rpc_js_form_path=%(home_dir)s/media/www/js/rpc
--loglevel=4

--gen_file_id_start=100
"""

def ConfigureTransMaster():
    print ' ** Setup transmaster...'
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name,
                 'scp_user': FLAGS.scp_user,
                 'admin_pass': FLAGS.admin_pass,
                 'slaves': ','.join(
            ['http://%s:8083/rpc_slave_manager?codec=json' % x
             for x in FLAGS.transslaves]),
                 }
    dirs = ['%(home_dir)s/var/data/%(name)s/transmaster/input',
            '%(home_dir)s/var/data/%(name)s/transmaster/output',
            '%(home_dir)s/var/data/%(name)s/transmaster/process',
            '%(home_dir)s/var/log/%(name)s',
            '%(home_dir)s/var/state/%(name)s']
    for d in dirs:
        MkDir(d % conf_map)
    # endfor

    open('%s/etc/ini/%s/transmaster.ini' % (FLAGS.home_dir,
                                            FLAGS.name), 'w').write(
            TRANSMASTER_INI % conf_map)
#enddef

######################################################################

def main(argv):
    try:
        argv = FLAGS(argv)  # parse flags
    except gflags.FlagsError, e:
        print '%s\\nUsage: %s ARGS\\n%s' % (e, sys.argv[0], FLAGS)
        sys.exit(1)
    # endtry

    if not FLAGS.name:
        sys.exit('Neet to specify --name')
    # endif

    for s in FLAGS.components:
        if s == 'whispercast':
            ConfigureWhispercast()
        elif s == 'whisperproc':
            ConfigureWhisperproc()
        elif s == 'transslave':
            ConfigureTransSlave()
        elif s == 'transcoder':
            ConfigureTransCoder()
        elif s == 'transmaster':
            ConfigureTransMaster()
        else:
            sys.exit('Unknown component: %s' % s)
    # endfor

    print ' ** Setup services...'
    conf_map = { 'home_dir' : FLAGS.home_dir,
                 'name': FLAGS.name }
    services_dir = '%(home_dir)s/services/%(name)s/' % conf_map
    MkDir(services_dir)
    open('%s/components.ini' % services_dir, 'w').write(
        '\n'.join(FLAGS.components))
    print 'DONE'

if __name__=='__main__':
    main(sys.argv)
