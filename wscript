## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import Options
import os.path
import ns3waf
import sys


def options(opt):
    opt.tool_options('compiler_cc') 
    ns3waf.options(opt)
    opt.add_option('--enable-kernel-stack',
                   help=('Path to the prefix where the kernel wrapper headers are installed'),
                   default=None,
                   dest='kernel_stack', type="string")

def search_file(files):
    for f in files:
        if os.path.isfile (f):
            return f
    return None

def configure(conf):
    ns3waf.check_modules(conf, ['core', 'network', 'internet'], mandatory = True)
    ns3waf.check_modules(conf, ['point-to-point'], mandatory = False)

    conf.check_tool('compiler_cc')

    conf.env.append_value('LINKFLAGS', '-pthread')
    conf.check (lib='dl', mandatory = True)

    vg_h = conf.check(header_name='valgrind/valgrind.h', mandatory=False)
    vg_memcheck_h = conf.check(header_name='valgrind/memcheck.h', mandatory=False)
    if vg_h and vg_memcheck_h:
        conf.env.append_value('CXXDEFINES', 'HAVE_VALGRIND_H')

    conf.start_msg('Searching C library')
    libc = search_file ([
            '/lib64/libc.so.6',
            '/lib/libc.so.6',
            ])
    if libc is None:
        conf.fatal('not found')
    else:
        conf.end_msg(libc, True)
    conf.env['LIBC_FILE'] = libc

    conf.start_msg('Searching pthread library')
    libpthread = search_file ([
            '/lib64/libpthread.so.0',
            '/lib/libpthread.so.0',
            ])
    if libpthread is None:
        conf.fatal('not found')
    else:
        conf.end_msg(libpthread, True)
    conf.env['LIBPTHREAD_FILE'] = libpthread

    if Options.options.kernel_stack is not None and os.path.isdir(Options.options.kernel_stack):
        conf.check(header_name='sim/sim.h',
                   includes=os.path.join(Options.options.kernel_stack, 'include'))
        conf.check()
        conf.env['KERNEL_STACK'] = Options.options.kernel_stack

    ns3waf.print_feature_summary(conf)

def build_netlink(bld):
    module_source = [
        'netlink/netlink-socket.cc',
        'netlink/netlink-socket-address.cc',
        'netlink/netlink-socket-factory.cc',
        'netlink/netlink-attribute.cc',
        'netlink/netlink-message.cc',
        'netlink/netlink-message-route.cc',
        ]
    module_headers = [
        'netlink/netlink-socket-factory.h'
        ]
    if ns3waf.modules_found(bld, ['internet', 'core']):
        uselib = ns3waf.modules_uselib(bld, ['internet', 'core'])
        module = ns3waf.create_module(bld, name='netlink',
                                      source=module_source,
                                      headers=module_headers,
                                      use=uselib)

        if ns3waf.modules_found(bld, ['point-to-point']):
            module_tests = [
                'test/netlink-socket-test.cc',
                ]
            uselib = ns3waf.modules_uselib(bld, ['internet', 
                                                 'point-to-point', 
                                                 'core'])
            module.add_tests(source=module_tests, use = uselib)

def create_dce_program(bld, **kw):
    if os.uname()[4] == 'x86_64':
        mcmodel = ['-mcmodel=large']
    else:
        mcmodel = []
    nofortify = ['-U_FORTIFY_SOURCE']
    #debug_dl = ['-Wl,--dynamic-linker=/usr/lib/debug/ld-linux-x86-64.so.2']
    debug_dl = []
    kw['cxxflags'] = kw.get('cxxflags', []) + ['-fpie'] + mcmodel + nofortify
    kw['cflags'] = kw.get('cflags', []) + ['-fpie'] + mcmodel + nofortify
    kw['linkflags'] = kw.get('linkflags', []) + ['-pie'] + debug_dl
    bld.program(**kw)

def new_test(bld,name,uselib):
    obj = create_dce_program(bld, target='bin/' + name, source = ['test/' + name + '.cc'],
                             use = uselib + ['lib/test'])


def build_dce_tests(bld):
    test = bld.shlib(source=['test/test-macros.cc'], target='lib/test',
                     linkflags=['-Wl,-soname=libtest.so'])

    tests = [['test-empty', []],
             ['test-sleep', []],
             ['test-nanosleep', []],
             ['test-pthread', ['PTHREAD']],
             ['test-mutex', ['PTHREAD']],
             ['test-once', ['PTHREAD']],
             ['test-pthread-key', ['PTHREAD']],
             ['test-sem', ['PTHREAD']],
             ['test-malloc', []],
             ['test-malloc-2', []],
             ['test-fd-simple', []],
             ['test-strerror', []],
             ['test-stdio', []],
             ['test-string', []],
             ['test-netdb', []],
             ['test-env', []],
             ['test-cond', ['PTHREAD']],
             ['test-timer-fd', []],
             ['test-stdlib', []],
             ['test-select', []],
             ['test-random', []],
             ['test-ioctl', []],
             ['test-fork', []],
             ]
    for name,uselib in tests:
        new_test(bld, name, uselib)

def build_example(bld, needed, **kw):
    external = [i for i in needed if not i == 'dce' ]
    if not ns3waf.modules_found(bld, external):
        return
    kw['use'] = kw.get('use', []) + ns3waf.modules_uselib(bld, needed)
    bld.program(**kw)
    

def build_dce_examples(bld):
    create_dce_program(bld, target = 'bin/udp-server', 
                       source = ['example/udp-server.cc'])
    create_dce_program(bld, target = 'bin/udp-client', 
                       source = ['example/udp-client.cc'])
    create_dce_program(bld, target = 'bin/tcp-server', 
                       source = ['example/tcp-server.cc'])
    create_dce_program(bld, target = 'bin/tcp-client', 
                       source = ['example/tcp-client.cc'])
    create_dce_program(bld, target = 'bin/tcp-loopback', 
                       source = ['example/tcp-loopback.cc'])
    create_dce_program(bld, target = 'bin/udp-perf', 
                       source = ['example/udp-perf.cc'],
                       lib='m')

    build_example(bld, ['core', 'internet', 'dce'], 
                  target='bin/dce-tcp-simple',
                  source=['example/dce-tcp-simple.cc'])

    build_example(bld, ['core', 'internet', 'dce'], 
                  target='bin/dce-udp-simple',
                  source=['example/dce-udp-simple.cc'])

    build_example(bld, ['core', 'internet', 'dce', 'point-to-point'], 
                  target='bin/dce-udp-perf',
                  source=['example/dce-udp-perf.cc'])


    if bld.env['KERNEL_STACK']:
        build_example(bld, ['core', 'network', 'dce'], 
                      target='bin/dce-linux-simple',
                      source=['example/dce-linux-simple.cc'])

        build_example(bld, ['core', 'network', 'dce', 'wifi', 'point-to-point', 'csma', 'mobility'],
                      target='bin/dce-linux',
                      source=['example/dce-linux.cc'])


def build(bld):
    build_netlink(bld)

    if bld.env['KERNEL_STACK']:
        kernel_source = [
            'linux-socket-fd-factory.cc',
            'linux-socket-fd.cc',
            ]
        kernel_headers = [ 'linux-socket-fd-factory.h']
        kernel_includes = [os.path.join(bld.env['KERNEL_STACK'], 'include')]
    else:
        kernel_source = []
        kernel_headers = []
        kernel_includes = []

    module_source = [
        'model/dce-manager.cc',
	'model/dce-application.cc',
        'model/dce.cc',
        'model/dce-signal.cc',
        'model/libc-dce.c',
        'model/utils.cc',
        'model/unix-fd.cc',
        'model/unix-file-fd.cc',
        'model/unix-socket-fd.cc',
        'model/unix-datagram-socket-fd.cc',
        'model/unix-stream-socket-fd.cc',
        'model/unix-timer-fd.cc',
        'model/dce-fd.cc',
        'model/dce-stdio.cc',
        'model/dce-pthread.cc',
        'model/dce-stdlib.cc',
        'model/dce-debug.cc',
        'model/dce-semaphore.cc',
        'model/dce-pthread-mutex.cc',
        'model/dce-cxa.cc',
        'model/dce-netdb.cc',
        'model/dce-string.cc',
        'model/dce-env.cc',
        'model/dce-pthread-cond.cc',
        'model/dce-timerfd.cc',
        'model/dce-stat.cc',
        'model/dce-global-variables.cc',
        'model/cmsg.cc',
        'model/waiter.cc',
        'model/kingsley-alloc.cc',
        'model/dce-alloc.cc',
        'model/fiber-manager.cc',
        'model/ucontext-fiber-manager.cc',
        'model/pthread-fiber-manager.cc',
        'model/task-manager.cc',
        'model/task-scheduler.cc',
        'model/rr-task-scheduler.cc',
        'model/loader-factory.cc',
        'model/elf-dependencies.cc',
        'model/elf-cache.cc',
        'model/cooja-loader-factory.cc',
        'model/copy-loader-factory.cc',
        'model/dlm-loader-factory.cc',
        'model/socket-fd-factory.cc',
        'model/ns3-socket-fd-factory.cc',
        # helper.
        'helper/dce-manager-helper.cc',
        'helper/dce-application-helper.cc',
        ]
    module_headers = [
        'model/dce-manager.h',
        'model/task-scheduler.h',
        'model/task-manager.h',
        'model/socket-fd-factory.h',
        'model/loader-factory.h',
	'model/dce-application.h',
        'helper/dce-manager-helper.h',
        'helper/dce-application-helper.h',
        ]
    module_source = module_source + kernel_source
    module_headers = module_headers + kernel_headers
    uselib = ns3waf.modules_uselib(bld, ['core', 'network', 'internet', 'netlink'])
    module = ns3waf.create_module(bld, name='dce',
                                  source=module_source,
                                  headers=module_headers,
                                  use=uselib,
                                  includes=kernel_includes,
                                  lib=['dl'])
    module_tests = [
        'test/dce-manager-test.cc',
        ]
    module.add_tests(source=module_tests, use = uselib)
    build_dce_tests(bld)
    build_dce_examples(bld)

    bld.add_group('dce_version_files')

    bld(source=['model/libc-ns3.version'],
            target='model/libc.version',
            rule='readversiondef ' + bld.env['LIBC_FILE'] + ' |' \
                'cat ${SRC[0].abspath()} - > ${TGT}')

    bld(source=['model/libpthread-ns3.version'],
            target='model/libpthread.version',
            rule='readversiondef ' + bld.env['LIBPTHREAD_FILE'] + ' |' \
                'cat ${SRC[0].abspath()} - > ${TGT}')

    bld.add_group('dce_use_version_files')

    # The very small libc used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c', 'model/libc-global-variables.c'],
              target='lib/c-ns3', cflags=['-g'],
              defines=['LIBSETUP=libc_setup'],
              linkflags=['-nostdlib', 
                         '-Wl,--version-script=' + os.path.join('model', 'libc.version'),
                         '-Wl,-soname=libc.so.6'])

    # The very small libpthread used to replace the glibc
    # and forward to the dce_* code
    bld.shlib(source = ['model/libc.c'],
              target='lib/pthread-ns3', cflags=['-g'],
              defines=['LIBSETUP=libpthread_setup'],
              linkflags=['-nostdlib', '-lc',
                         '-Wl,--version-script=' + os.path.join('model', 'libpthread.version'),
                         '-Wl,-soname=libpthread.so.0'])
