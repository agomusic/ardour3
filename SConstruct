# -*- python -*-

import os
import sys
import re
import shutil
import glob
import errno
import time
import platform
import string
import commands
from sets import Set
import SCons.Node.FS

SConsignFile()
EnsureSConsVersion(0, 96)

version = '2.0beta7.1'

subst_dict = { }

#
# Command-line options
#

opts = Options('scache.conf')
opts.AddOptions(
    ('ARCH', 'Set architecture-specific compilation flags by hand (all flags as 1 argument)',''),
    BoolOption('AUDIOUNITS', 'Compile with Apple\'s AudioUnit library. (experimental)', 0),
    BoolOption('COREAUDIO', 'Compile with Apple\'s CoreAudio library', 0),
    BoolOption('DEBUG', 'Set to build with debugging information and no optimizations', 0),
    PathOption('DESTDIR', 'Set the intermediate install "prefix"', '/'),
    EnumOption('DIST_TARGET', 'Build target for cross compiling packagers', 'auto', allowed_values=('auto', 'i386', 'i686', 'x86_64', 'powerpc', 'tiger', 'panther', 'none' ), ignorecase=2),
    BoolOption('DMALLOC', 'Compile and link using the dmalloc library', 0),
    BoolOption('EXTRA_WARN', 'Compile with -Wextra, -ansi, and -pedantic.  Might break compilation.  For pedants', 0),
    BoolOption('FFT_ANALYSIS', 'Include FFT analysis window', 0),
    BoolOption('FPU_OPTIMIZATION', 'Build runtime checked assembler code', 1),
    BoolOption('LIBLO', 'Compile with support for liblo library', 1),
    BoolOption('NLS', 'Set to turn on i18n support', 1),
    PathOption('PREFIX', 'Set the install "prefix"', '/usr/local'),
    BoolOption('SURFACES', 'Build support for control surfaces', 0),
    BoolOption('SYSLIBS', 'USE AT YOUR OWN RISK: CANCELS ALL SUPPORT FROM ARDOUR AUTHORS: Use existing system versions of various libraries instead of internal ones', 0),
    BoolOption('VERSIONED', 'Add revision information to ardour/gtk executable name inside the build directory', 0),
    BoolOption('VST', 'Compile with support for VST', 0)
)

#----------------------------------------------------------------------
# a handy helper that provides a way to merge compile/link information
# from multiple different "environments"
#----------------------------------------------------------------------
#
class LibraryInfo(Environment):
    def __init__(self,*args,**kw):
        Environment.__init__ (self,*args,**kw)
    
    def Merge (self,others):
        for other in others:
            self.Append (LIBS = other.get ('LIBS',[]))
            self.Append (LIBPATH = other.get ('LIBPATH', []))
            self.Append (CPPPATH = other.get('CPPPATH', []))
            self.Append (LINKFLAGS = other.get('LINKFLAGS', []))
	self.Replace(LIBPATH = list(Set(self.get('LIBPATH', []))))
	self.Replace(CPPPATH = list(Set(self.get('CPPPATH',[]))))
        #doing LINKFLAGS breaks -framework
        #doing LIBS break link order dependency
    
    def ENV_update(self, src_ENV):
        for k in src_ENV.keys():
            if k in self['ENV'].keys() and k in [ 'PATH', 'LD_LIBRARY_PATH',
                                                  'LIB', 'INCLUDE' ]:
                self['ENV'][k]=SCons.Util.AppendPath(self['ENV'][k], src_ENV[k])
            else:
                self['ENV'][k]=src_ENV[k]

env = LibraryInfo (options = opts,
                   CPPPATH = [ '.' ],
                   VERSION = version,
                   TARBALL='ardour-' + version + '.tar.bz2',
                   DISTFILES = [ ],
                   DISTTREE  = '#ardour-' + version,
                   DISTCHECKDIR = '#ardour-' + version + '/check'
                   )

env.ENV_update(os.environ)

#----------------------------------------------------------------------
# Builders
#----------------------------------------------------------------------

# Handy subst-in-file builder
#

def do_subst_in_file(targetfile, sourcefile, dict):
    """Replace all instances of the keys of dict with their values.
    For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
    then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
    """
    try:
        f = open(sourcefile, 'rb')
        contents = f.read()
        f.close()
    except:
        raise SCons.Errors.UserError, "Can't read source file %s"%sourcefile
    for (k,v) in dict.items():
        contents = re.sub(k, v, contents)
    try:
        f = open(targetfile, 'wb')
        f.write(contents)
        f.close()
    except:
        raise SCons.Errors.UserError, "Can't write target file %s"%targetfile
    return 0 # success

def subst_in_file(target, source, env):
    if not env.has_key('SUBST_DICT'):
        raise SCons.Errors.UserError, "SubstInFile requires SUBST_DICT to be set."
    d = dict(env['SUBST_DICT']) # copy it
    for (k,v) in d.items():
        if callable(v):
            d[k] = env.subst(v())
        elif SCons.Util.is_String(v):
            d[k]=env.subst(v)
        else:
            raise SCons.Errors.UserError, "SubstInFile: key %s: %s must be a string or callable"%(k, repr(v))
    for (t,s) in zip(target, source):
        return do_subst_in_file(str(t), str(s), d)

def subst_in_file_string(target, source, env):
    """This is what gets printed on the console."""
    return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
                      for (t,s) in zip(target, source)])

def subst_emitter(target, source, env):
    """Add dependency from substituted SUBST_DICT to target.
    Returns original target, source tuple unchanged.
    """
    d = env['SUBST_DICT'].copy() # copy it
    for (k,v) in d.items():
        if callable(v):
            d[k] = env.subst(v())
        elif SCons.Util.is_String(v):
            d[k]=env.subst(v)
    Depends(target, SCons.Node.Python.Value(d))
    # Depends(target, source) # this doesn't help the install-sapphire-linux.sh problem
    return target, source

subst_action = Action (subst_in_file, subst_in_file_string)
env['BUILDERS']['SubstInFile'] = Builder(action=subst_action, emitter=subst_emitter)

#
# internationalization
#

# po_builder: builder function to copy po files to the parent directory while updating them
#
# first source:  .po file
# second source: .pot file
#

def po_builder(target,source,env):
    os.spawnvp (os.P_WAIT, 'cp', ['cp', str(source[0]), str(target[0])])
    args = [ 'msgmerge',
             '--update',
             str(target[0]),
             str(source[1])
             ]
    print 'Updating ' + str(target[0])
    return os.spawnvp (os.P_WAIT, 'msgmerge', args)

po_bld = Builder (action = po_builder)
env.Append(BUILDERS = {'PoBuild' : po_bld})

# mo_builder: builder function for (binary) message catalogs (.mo)
#
# first source:  .po file
#

def mo_builder(target,source,env):
    args = [ 'msgfmt',
             '-c',
             '-o',
             target[0].get_path(),
             source[0].get_path()
             ]
    return os.spawnvp (os.P_WAIT, 'msgfmt', args)

mo_bld = Builder (action = mo_builder)
env.Append(BUILDERS = {'MoBuild' : mo_bld})

# pot_builder: builder function for message templates (.pot)
#
# source: list of C/C++ etc. files to extract messages from
#

def pot_builder(target,source,env):
    args = [ 'xgettext',
             '--keyword=_',
             '--keyword=N_',
             '--from-code=UTF-8',
             '-o', target[0].get_path(),
             "--default-domain=" + env['PACKAGE'],
             '--copyright-holder="Paul Davis"' ]
    args += [ src.get_path() for src in source ]
    
    return os.spawnvp (os.P_WAIT, 'xgettext', args)

pot_bld = Builder (action = pot_builder)
env.Append(BUILDERS = {'PotBuild' : pot_bld})

#
# utility function, not a builder
#

def i18n (buildenv, sources, installenv):
    domain = buildenv['PACKAGE']
    potfile = buildenv['POTFILE']
    
    installenv.Alias ('potupdate', buildenv.PotBuild (potfile, sources))
    
    p_oze = [ os.path.basename (po) for po in glob.glob ('po/*.po') ]
    languages = [ po.replace ('.po', '') for po in p_oze ]
    
    for po_file in p_oze:
        buildenv.PoBuild(po_file, ['po/'+po_file, potfile])
        mo_file = po_file.replace (".po", ".mo")
        installenv.Alias ('install', buildenv.MoBuild (mo_file, po_file))
    
    for lang in languages:
        modir = (os.path.join (install_prefix, 'share/locale/' + lang + '/LC_MESSAGES/'))
        moname = domain + '.mo'
        installenv.Alias('install', installenv.InstallAs (os.path.join (modir, moname), lang + '.mo'))

#
# A generic builder for version.cc files
#
# note: requires that DOMAIN, MAJOR, MINOR, MICRO are set in the construction environment
# note: assumes one source files, the header that declares the version variables
#

def version_builder (target, source, env):
    cmd = "svn info "
    cmd += source[0].get_path()
    cmd += " | awk '/^Revision:/ { print $2}'"
    
    rev = commands.getoutput (cmd)
        
    text  = "const char* " + env['DOMAIN'] + "_revision = \"" + rev + "\";\n"
    text += "int " + env['DOMAIN'] + "_major_version = " + str (env['MAJOR']) + ";\n"
    text += "int " + env['DOMAIN'] + "_minor_version = " + str (env['MINOR']) + ";\n"
    text += "int " + env['DOMAIN'] + "_micro_version = " + str (env['MICRO']) + ";\n"
    
    try:
        o = file (target[0].get_path(), 'w')
        o.write (text)
        o.close ()
    except IOError:
        print "Could not open", target[0].get_path(), " for writing\n"
        sys.exit (-1)

    text  = "#ifndef __" + env['DOMAIN'] + "_version_h__\n"
    text += "#define __" + env['DOMAIN'] + "_version_h__\n"
    text += "extern const char* " + env['DOMAIN'] + "_revision;\n"
    text += "extern int " + env['DOMAIN'] + "_major_version;\n"
    text += "extern int " + env['DOMAIN'] + "_minor_version;\n"
    text += "extern int " + env['DOMAIN'] + "_micro_version;\n"
    text += "#endif /* __" + env['DOMAIN'] + "_version_h__ */\n"
    
    try:
        o = file (target[1].get_path(), 'w')
        o.write (text)
        o.close ();
    except IOError:
        print "Could not open", target[1].get_path(), " for writing\n"
        sys.exit (-1)
        
    return None

version_bld = Builder (action = version_builder)
env.Append (BUILDERS = {'VersionBuild' : version_bld})

#
# a builder that makes a hard link from the 'source' executable to a name with
# a "build ID" based on the most recent CVS activity that might be reasonably
# related to version activity. this relies on the idea that the SConscript
# file that builds the executable is updated with new version info and committed
# to the source code repository whenever things change.
#

def versioned_builder(target,source,env):
    w, r = os.popen2( "svn info | awk '/^Revision:/ { print $2}'")
    
    last_revision = r.readline().strip()
    w.close()
    r.close()
    if last_revision == "":
        print "No SVN info found - versioned executable cannot be built"
        return -1
    
    print "The current build ID is " + last_revision
    
    tagged_executable = source[0].get_path() + '-' + last_revision
    
    if os.path.exists (tagged_executable):
        print "Replacing existing executable with the same build tag."
        os.unlink (tagged_executable)
    
    return os.link (source[0].get_path(), tagged_executable)

verbuild = Builder (action = versioned_builder)
env.Append (BUILDERS = {'VersionedExecutable' : verbuild})

#
# source tar file builder
#

def distcopy (target, source, env):
    treedir = str (target[0])
    
    try:
        os.mkdir (treedir)
    except OSError, (errnum, strerror):
        if errnum != errno.EEXIST:
            print 'mkdir ', treedir, ':', strerror
    
    cmd = 'tar cf - '
    #
    # we don't know what characters might be in the file names
    # so quote them all before passing them to the shell
    #
    all_files = ([ str(s) for s in source ])
    cmd += " ".join ([ "'%s'" % quoted for quoted in all_files])
    cmd += ' | (cd ' + treedir + ' && tar xf -)'
    p = os.popen (cmd)
    return p.close ()

def tarballer (target, source, env):
    cmd = 'tar -jcf ' + str (target[0]) +  ' ' + str(source[0]) + "  --exclude '*~'"
    print 'running ', cmd, ' ... '
    p = os.popen (cmd)
    return p.close ()

dist_bld = Builder (action = distcopy,
                    target_factory = SCons.Node.FS.default_fs.Entry,
                    source_factory = SCons.Node.FS.default_fs.Entry,
                    multi = 1)

tarball_bld = Builder (action = tarballer,
                       target_factory = SCons.Node.FS.default_fs.Entry,
                       source_factory = SCons.Node.FS.default_fs.Entry)

env.Append (BUILDERS = {'Distribute' : dist_bld})
env.Append (BUILDERS = {'Tarball' : tarball_bld})

#
# Make sure they know what they are doing
#

if env['VST']:
    sys.stdout.write ("Are you building Ardour for personal use (rather than distributiont to others)? [no]: ")
    answer = sys.stdin.readline ()
    answer = answer.rstrip().strip()
    if answer != "yes" and answer != "y":
        print 'You cannot build Ardour with VST support for distribution to others.\nIt is a violation of several different licenses. Build with VST=false.'
        sys.exit (-1);
    else:
        print "OK, VST support will be enabled"


# ----------------------------------------------------------------------
# Construction environment setup
# ----------------------------------------------------------------------

libraries = { }

libraries['core'] = LibraryInfo (CCFLAGS = '-Ilibs')

#libraries['sndfile'] = LibraryInfo()
#libraries['sndfile'].ParseConfig('pkg-config --cflags --libs sndfile')

libraries['lrdf'] = LibraryInfo()
libraries['lrdf'].ParseConfig('pkg-config --cflags --libs lrdf')

libraries['raptor'] = LibraryInfo()
libraries['raptor'].ParseConfig('pkg-config --cflags --libs raptor')

libraries['samplerate'] = LibraryInfo()
libraries['samplerate'].ParseConfig('pkg-config --cflags --libs samplerate')

if env['FFT_ANALYSIS']:
	libraries['fftw3f'] = LibraryInfo()
	libraries['fftw3f'].ParseConfig('pkg-config --cflags --libs fftw3f')

libraries['jack'] = LibraryInfo()
libraries['jack'].ParseConfig('pkg-config --cflags --libs jack')

libraries['xml'] = LibraryInfo()
libraries['xml'].ParseConfig('pkg-config --cflags --libs libxml-2.0')

libraries['xslt'] = LibraryInfo()
libraries['xslt'].ParseConfig('pkg-config --cflags --libs libxslt')

libraries['glib2'] = LibraryInfo()
libraries['glib2'].ParseConfig ('pkg-config --cflags --libs glib-2.0')
libraries['glib2'].ParseConfig ('pkg-config --cflags --libs gobject-2.0')
libraries['glib2'].ParseConfig ('pkg-config --cflags --libs gmodule-2.0')
libraries['glib2'].ParseConfig ('pkg-config --cflags --libs gthread-2.0')

libraries['gtk2'] = LibraryInfo()
libraries['gtk2'].ParseConfig ('pkg-config --cflags --libs gtk+-2.0')

libraries['pango'] = LibraryInfo()
libraries['pango'].ParseConfig ('pkg-config --cflags --libs pango')

libraries['libgnomecanvas2'] = LibraryInfo()
libraries['libgnomecanvas2'].ParseConfig ('pkg-config --cflags --libs libgnomecanvas-2.0')

#libraries['flowcanvas'] = LibraryInfo(LIBS='flowcanvas', LIBPATH='#/libs/flowcanvas', CPPPATH='#libs/flowcanvas')

# The Ardour Control Protocol Library

libraries['ardour_cp'] = LibraryInfo (LIBS='ardour_cp', LIBPATH='#libs/surfaces/control_protocol',
                                      CPPPATH='#libs/surfaces/control_protocol')

# The Ardour backend/engine

libraries['ardour'] = LibraryInfo (LIBS='ardour', LIBPATH='#libs/ardour', CPPPATH='#libs/ardour')
libraries['midi++2'] = LibraryInfo (LIBS='midi++', LIBPATH='#libs/midi++2', CPPPATH='#libs/midi++2')
libraries['pbd']    = LibraryInfo (LIBS='pbd', LIBPATH='#libs/pbd', CPPPATH='#libs/pbd')
libraries['gtkmm2ext'] = LibraryInfo (LIBS='gtkmm2ext', LIBPATH='#libs/gtkmm2ext', CPPPATH='#libs/gtkmm2ext')

#
# Check for libusb

libraries['usb'] = LibraryInfo ()

conf = Configure (libraries['usb'])
if conf.CheckLib ('usb', 'usb_interrupt_write'):
    have_libusb = True
else:
    have_libusb = False

libraries['usb'] = conf.Finish ()

#
# Check for FLAC

libraries['flac'] = LibraryInfo ()

conf = Configure (libraries['flac'])
conf.CheckLib ('FLAC', 'FLAC__stream_decoder_new', language='CXX')
libraries['flac'] = conf.Finish ()

# or if that fails...
#libraries['flac']    = LibraryInfo (LIBS='FLAC')

# boost (we don't link against boost, just use some header files)

libraries['boost'] = LibraryInfo ()
conf = Configure (libraries['boost'])
if conf.CheckHeader ('boost/shared_ptr.hpp', language='CXX') == False:
        print "Boost header files do not appear to be installed."
        sys.exit (1)
    
libraries['boost'] = conf.Finish ()

#
# Check for liblo

if env['LIBLO']:
    libraries['lo'] = LibraryInfo ()
    
    conf = Configure (libraries['lo'])
    if conf.CheckLib ('lo', 'lo_server_new') == False:
        print "liblo does not appear to be installed."
        sys.exit (1)
    
    libraries['lo'] = conf.Finish ()

#
# Check for dmalloc

libraries['dmalloc'] = LibraryInfo ()

#
# look for the threaded version
#

conf = Configure (libraries['dmalloc'])
if conf.CheckLib ('dmallocth', 'dmalloc_shutdown'):
    have_libdmalloc = True
else:
    have_libdmalloc = False

libraries['dmalloc'] = conf.Finish ()

#
# Audio/MIDI library (needed for MIDI, since audio is all handled via JACK)
#

conf = Configure(env)

if conf.CheckCHeader('alsa/asoundlib.h'):
    libraries['sysmidi'] = LibraryInfo (LIBS='asound')
    env['SYSMIDI'] = 'ALSA Sequencer'
    subst_dict['%MIDITAG%'] = "seq"
    subst_dict['%MIDITYPE%'] = "alsa/sequencer"
elif conf.CheckCHeader('/System/Library/Frameworks/CoreMIDI.framework/Headers/CoreMIDI.h'):
    # this line is needed because scons can't handle -framework in ParseConfig() yet.
    libraries['sysmidi'] = LibraryInfo (LINKFLAGS= '-framework CoreMIDI -framework CoreFoundation -framework CoreAudio -framework CoreServices -framework AudioUnit -framework AudioToolbox -bind_at_load')
    env['SYSMIDI'] = 'CoreMIDI'
    subst_dict['%MIDITAG%'] = "ardour"
    subst_dict['%MIDITYPE%'] = "coremidi"
else:
    print "It appears you don't have the required MIDI libraries installed. For Linux this means you are missing the development package for ALSA libraries."
    sys.exit (1)

env = conf.Finish()

if env['SYSLIBS']:
    
    libraries['sigc2'] = LibraryInfo()
    libraries['sigc2'].ParseConfig('pkg-config --cflags --libs sigc++-2.0')
    libraries['glibmm2'] = LibraryInfo()
    libraries['glibmm2'].ParseConfig('pkg-config --cflags --libs glibmm-2.4')
    libraries['gdkmm2'] = LibraryInfo()
    libraries['gdkmm2'].ParseConfig ('pkg-config --cflags --libs gdkmm-2.4')
    libraries['gtkmm2'] = LibraryInfo()
    libraries['gtkmm2'].ParseConfig ('pkg-config --cflags --libs gtkmm-2.4')
    libraries['atkmm'] = LibraryInfo()
    libraries['atkmm'].ParseConfig ('pkg-config --cflags --libs atkmm-1.6')
    libraries['pangomm'] = LibraryInfo()
    libraries['pangomm'].ParseConfig ('pkg-config --cflags --libs pangomm-1.4')
    libraries['libgnomecanvasmm'] = LibraryInfo()
    libraries['libgnomecanvasmm'].ParseConfig ('pkg-config --cflags --libs libgnomecanvasmm-2.6')

#
# cannot use system one for the time being
#
    
    libraries['sndfile-ardour'] = LibraryInfo(LIBS='libsndfile-ardour',
                                    LIBPATH='#libs/libsndfile',
                                    CPPPATH=['#libs/libsndfile', '#libs/libsndfile/src'])

#    libraries['libglademm'] = LibraryInfo()
#    libraries['libglademm'].ParseConfig ('pkg-config --cflags --libs libglademm-2.4')

#    libraries['flowcanvas'] = LibraryInfo(LIBS='flowcanvas', LIBPATH='#/libs/flowcanvas', CPPPATH='#libs/flowcanvas')
    libraries['soundtouch'] = LibraryInfo()
    libraries['soundtouch'].ParseConfig ('pkg-config --cflags --libs soundtouch-1.0')

    libraries['appleutility'] = LibraryInfo(LIBS='libappleutility',
                                            LIBPATH='#libs/appleutility',
                                            CPPPATH='#libs/appleutility')
    
    coredirs = [
        'templates'
    ]
    
    subdirs = [
        'libs/libsndfile',
        'libs/pbd',
        'libs/midi++2',
        'libs/ardour',
    # these are unconditionally included but have
    # tests internally to avoid compilation etc
    # if VST is not set
        'libs/fst',
        'vst',
    # this is unconditionally included but has
    # tests internally to avoid compilation etc
    # if COREAUDIO is not set
        'libs/appleutility'
        ]
    
    gtk_subdirs = [
#        'libs/flowcanvas',
        'libs/gtkmm2ext',
        'gtk2_ardour'
        ]

else:
    libraries['sigc2'] = LibraryInfo(LIBS='sigc++2',
                                    LIBPATH='#libs/sigc++2',
                                    CPPPATH='#libs/sigc++2')
    libraries['glibmm2'] = LibraryInfo(LIBS='glibmm2',
                                    LIBPATH='#libs/glibmm2',
                                    CPPPATH='#libs/glibmm2')
    libraries['pangomm'] = LibraryInfo(LIBS='pangomm',
                                    LIBPATH='#libs/gtkmm2/pango',
                                    CPPPATH='#libs/gtkmm2/pango')
    libraries['atkmm'] = LibraryInfo(LIBS='atkmm',
                                     LIBPATH='#libs/gtkmm2/atk',
                                     CPPPATH='#libs/gtkmm2/atk')
    libraries['gdkmm2'] = LibraryInfo(LIBS='gdkmm2',
                                      LIBPATH='#libs/gtkmm2/gdk',
                                      CPPPATH='#libs/gtkmm2/gdk')
    libraries['gtkmm2'] = LibraryInfo(LIBS='gtkmm2',
                                     LIBPATH="#libs/gtkmm2/gtk",
                                     CPPPATH='#libs/gtkmm2/gtk/')
    libraries['libgnomecanvasmm'] = LibraryInfo(LIBS='libgnomecanvasmm',
                                                LIBPATH='#libs/libgnomecanvasmm',
                                                CPPPATH='#libs/libgnomecanvasmm')
    
    libraries['soundtouch'] = LibraryInfo(LIBS='soundtouch',
                                          LIBPATH='#libs/soundtouch',
                                          CPPPATH=['#libs', '#libs/soundtouch'])
    libraries['sndfile-ardour'] = LibraryInfo(LIBS='libsndfile-ardour',
                                    LIBPATH='#libs/libsndfile',
                                    CPPPATH=['#libs/libsndfile', '#libs/libsndfile/src'])
#    libraries['libglademm'] = LibraryInfo(LIBS='libglademm',
#                                          LIBPATH='#libs/libglademm',
#                                          CPPPATH='#libs/libglademm')
    libraries['appleutility'] = LibraryInfo(LIBS='libappleutility',
                                            LIBPATH='#libs/appleutility',
                                            CPPPATH='#libs/appleutility')

    coredirs = [
        'libs/soundtouch',
        'templates'
    ]
    
    subdirs = [
        'libs/sigc++2',
        'libs/libsndfile',
        'libs/pbd',
        'libs/midi++2',
        'libs/ardour',
    # these are unconditionally included but have
    # tests internally to avoid compilation etc
    # if VST is not set
        'libs/fst',
        'vst',
    # this is unconditionally included but has
    # tests internally to avoid compilation etc
    # if COREAUDIO is not set
        'libs/appleutility'
        ]
    
    gtk_subdirs = [
	'libs/glibmm2',
	'libs/gtkmm2/pango',
	'libs/gtkmm2/atk',
	'libs/gtkmm2/gdk',
	'libs/gtkmm2/gtk',
	'libs/libgnomecanvasmm',
#	'libs/flowcanvas',
    'libs/gtkmm2ext',
    'gtk2_ardour'
        ]

#
# always build the LGPL control protocol lib, since we link against it ourselves
# ditto for generic MIDI
#

surface_subdirs = [ 'libs/surfaces/control_protocol', 'libs/surfaces/generic_midi' ]

if env['SURFACES']:
    if have_libusb:
        surface_subdirs += [ 'libs/surfaces/tranzport' ]
    if os.access ('libs/surfaces/sony9pin', os.F_OK):
        surface_subdirs += [ 'libs/surfaces/sony9pin' ]

opts.Save('scache.conf', env)
Help(opts.GenerateHelpText(env))

if os.environ.has_key('PATH'):
    env.Append(PATH = os.environ['PATH'])

if os.environ.has_key('PKG_CONFIG_PATH'):
    env.Append(PKG_CONFIG_PATH = os.environ['PKG_CONFIG_PATH'])

if os.environ.has_key('CC'):
    env['CC'] = os.environ['CC']

if os.environ.has_key('CXX'):
    env['CXX'] = os.environ['CXX']

if os.environ.has_key('DISTCC_HOSTS'):
    env['ENV']['DISTCC_HOSTS'] = os.environ['DISTCC_HOSTS']
    env['ENV']['HOME'] = os.environ['HOME']

final_prefix = '$PREFIX'

if env['DESTDIR'] :
    install_prefix = '$DESTDIR/$PREFIX'
else:
    install_prefix = env['PREFIX']

subst_dict['%INSTALL_PREFIX%'] = install_prefix;
subst_dict['%FINAL_PREFIX%'] = final_prefix;
subst_dict['%PREFIX%'] = final_prefix;

if env['PREFIX'] == '/usr':
    final_config_prefix = '/etc'
else:
    final_config_prefix = env['PREFIX'] + '/etc'

config_prefix = '$DESTDIR' + final_config_prefix

# SCons should really do this for us

conf = Configure (env)

have_cxx = conf.TryAction (Action (env['CXX'] + ' --version'))
if have_cxx[0] != 1:
    print "This system has no functional C++ compiler. You cannot build Ardour from source without one."
    sys.exit (1)
else:
    print "Congratulations, you have a functioning C++ compiler."

env = conf.Finish()

#
# Compiler flags and other system-dependent stuff
#

opt_flags = []
debug_flags = [ '-g' ]

# guess at the platform, used to define compiler flags

config_guess = os.popen("tools/config.guess").read()[:-1]

config_cpu = 0
config_arch = 1
config_kernel = 2
config_os = 3
config = config_guess.split ("-")

print "system triple: " + config_guess

# Autodetect
if env['DIST_TARGET'] == 'auto':
    if config[config_arch] == 'apple':
        # The [.] matches to the dot after the major version, "." would match any character
        if re.search ("darwin[0-7][.]", config[config_kernel]) != None:
            env['DIST_TARGET'] = 'panther'
        else:
            env['DIST_TARGET'] = 'tiger'
    else:
        if re.search ("x86_64", config[config_cpu]) != None:
            env['DIST_TARGET'] = 'x86_64'
        elif re.search("i[0-5]86", config[config_cpu]) != None:
            env['DIST_TARGET'] = 'i386'
        elif re.search("powerpc", config[config_cpu]) != None:
            env['DIST_TARGET'] = 'powerpc'
        else:
            env['DIST_TARGET'] = 'i686'
    print "\n*******************************"
    print "detected DIST_TARGET = " + env['DIST_TARGET']
    print "*******************************\n"


if config[config_cpu] == 'powerpc' and env['DIST_TARGET'] != 'none':
    #
    # Apple/PowerPC optimization options
    #
    # -mcpu=7450 does not reliably work with gcc 3.*
    #
    if env['DIST_TARGET'] == 'panther' or env['DIST_TARGET'] == 'tiger':
        if config[config_arch] == 'apple':
            opt_flags.extend ([ "-mcpu=7450", "-faltivec"])
        else:
            opt_flags.extend ([ "-mcpu=7400", "-maltivec", "-mabi=altivec"])
    else:
        opt_flags.extend([ "-mcpu=750", "-mmultiple" ])
    opt_flags.extend (["-mhard-float", "-mpowerpc-gfxopt"])

elif ((re.search ("i[0-9]86", config[config_cpu]) != None) or (re.search ("x86_64", config[config_cpu]) != None)) and env['DIST_TARGET'] != 'none':
    
    build_host_supports_sse = 0
    
    debug_flags.append ("-DARCH_X86")
    opt_flags.append ("-DARCH_X86")
    
    if config[config_kernel] == 'linux' :
        
        if env['DIST_TARGET'] != 'i386':
            
            flag_line = os.popen ("cat /proc/cpuinfo | grep '^flags'").read()[:-1]
            x86_flags = flag_line.split (": ")[1:][0].split (' ')
            
            if "mmx" in x86_flags:
                opt_flags.append ("-mmmx")
            if "sse" in x86_flags:
                build_host_supports_sse = 1
            if "3dnow" in x86_flags:
                opt_flags.append ("-m3dnow")
            
            if config[config_cpu] == "i586":
                opt_flags.append ("-march=i586")
            elif config[config_cpu] == "i686":
                opt_flags.append ("-march=i686")
    
    if ((env['DIST_TARGET'] == 'i686') or (env['DIST_TARGET'] == 'x86_64')) and build_host_supports_sse:
        opt_flags.extend (["-msse", "-mfpmath=sse"])
        debug_flags.extend (["-msse", "-mfpmath=sse"])
# end of processor-specific section

# optimization section
if env['FPU_OPTIMIZATION']:
    if env['DIST_TARGET'] == 'tiger':
        opt_flags.append ("-DBUILD_VECLIB_OPTIMIZATIONS")
        debug_flags.append ("-DBUILD_VECLIB_OPTIMIZATIONS")
        libraries['core'].Append(LINKFLAGS= '-framework Accelerate')
    elif env['DIST_TARGET'] == 'i686' or env['DIST_TARGET'] == 'x86_64':
        opt_flags.append ("-DBUILD_SSE_OPTIMIZATIONS")
        debug_flags.append ("-DBUILD_SSE_OPTIMIZATIONS")
        if env['DIST_TARGET'] == 'x86_64':
            opt_flags.append ("-DUSE_X86_64_ASM")
            debug_flags.append ("-DUSE_X86_64_ASM")
        if build_host_supports_sse != 1:
            print "\nWarning: you are building Ardour with SSE support even though your system does not support these instructions. (This may not be an error, especially if you are a package maintainer)"
# end optimization section

#
# save off guessed arch element in an env
#
env.Append(CONFIG_ARCH=config[config_arch])


#
# ARCH="..." overrides all
#

if env['ARCH'] != '':
    opt_flags = env['ARCH'].split()

#
# prepend boiler plate optimization flags
#

opt_flags[:0] = [
    "-O3",
    "-fomit-frame-pointer",
    "-ffast-math",
    "-fstrength-reduce"
    ]

if env['DEBUG'] == 1:
    env.Append(CCFLAGS=" ".join (debug_flags))
else:
    env.Append(CCFLAGS=" ".join (opt_flags))

#
# warnings flags
#

env.Append(CCFLAGS="-Wall")
env.Append(CXXFLAGS="-Woverloaded-virtual")

if env['EXTRA_WARN']:
    env.Append(CCFLAGS="-Wextra -pedantic")
    env.Append(CXXFLAGS="-ansi")

if env['LIBLO']:
    env.Append(CCFLAGS="-DHAVE_LIBLO")

#
# everybody needs this
#

env.Merge ([ libraries['core'] ])

#
# fix scons nitpickiness on APPLE
#

if env['DIST_TARGET'] == 'panther' or env['DIST_TARGET'] == 'tiger':
    env.Append(CCFLAGS="-I/opt/local/include", LINKFLAGS="-L/opt/local/lib")

#
# i18n support
#

conf = Configure (env)
if env['NLS']:
    nls_error = 'This system is not configured for internationalized applications.  An english-only version will be built:'
    print 'Checking for internationalization support ...'
    have_gettext = conf.TryAction(Action('xgettext --version'))
    if have_gettext[0] != 1:
        nls_error += ' No xgettext command.'
        env['NLS'] = 0
    else:
        print "Found xgettext"
    
    have_msgmerge = conf.TryAction(Action('msgmerge --version'))
    if have_msgmerge[0] != 1:
        nls_error += ' No msgmerge command.'
        env['NLS'] = 0
    else:
        print "Found msgmerge"
    
    if not conf.CheckCHeader('libintl.h'):
        nls_error += ' No libintl.h.'
        env['NLS'] = 0
        
    if env['NLS'] == 0:
        print nls_error
    else:
        print "International version will be built."
env = conf.Finish()

if env['NLS'] == 1:
    env.Append(CCFLAGS="-DENABLE_NLS")

Export('env install_prefix final_prefix config_prefix final_config_prefix libraries i18n version subst_dict')

#
# the configuration file may be system dependent
#

conf = env.Configure ()

if conf.CheckCHeader('/System/Library/Frameworks/CoreAudio.framework/Versions/A/Headers/CoreAudio.h'):
    subst_dict['%JACK_INPUT%'] = "coreaudio:Built-in Audio:in"
    subst_dict['%JACK_OUTPUT%'] = "coreaudio:Built-in Audio:out"
else:
    subst_dict['%JACK_INPUT%'] = "alsa_pcm:playback_"
    subst_dict['%JACK_OUTPUT%'] = "alsa_pcm:capture_"

# posix_memalign available
if not conf.CheckFunc('posix_memalign'):
    print 'Did not find posix_memalign(), using malloc'
    env.Append(CCFLAGS='-DNO_POSIX_MEMALIGN')


env = conf.Finish()

rcbuild = env.SubstInFile ('ardour.rc','ardour.rc.in', SUBST_DICT = subst_dict)

env.Alias('install', env.Install(os.path.join(config_prefix, 'ardour2'), 'ardour_system.rc'))
env.Alias('install', env.Install(os.path.join(config_prefix, 'ardour2'), 'ardour.rc'))

Default (rcbuild)

# source tarball

Precious (env['DISTTREE'])

#
# note the special "cleanfirst" source name. this triggers removal
# of the existing disttree
#

env.Distribute (env['DISTTREE'],
                [ 'SConstruct',
                  'COPYING', 'PACKAGER_README', 'README',
                  'ardour.rc.in',
                  'ardour_system.rc',
                  'tools/config.guess',
                  'icons/icon/ardour_icon_mac_mask.png',
                  'icons/icon/ardour_icon_mac.png',
                  'icons/icon/ardour_icon_tango_16px_blue.png',
                  'icons/icon/ardour_icon_tango_16px_red.png',
                  'icons/icon/ardour_icon_tango_22px_blue.png',
                  'icons/icon/ardour_icon_tango_22px_red.png',
                  'icons/icon/ardour_icon_tango_32px_blue.png',
                  'icons/icon/ardour_icon_tango_32px_red.png',
                  'icons/icon/ardour_icon_tango_48px_blue.png',
                  'icons/icon/ardour_icon_tango_48px_red.png'
                  ] +
                glob.glob ('DOCUMENTATION/AUTHORS*') +
                glob.glob ('DOCUMENTATION/CONTRIBUTORS*') +
                glob.glob ('DOCUMENTATION/TRANSLATORS*') +
                glob.glob ('DOCUMENTATION/BUILD*') +
                glob.glob ('DOCUMENTATION/FAQ*') +
                glob.glob ('DOCUMENTATION/README*')
                )

srcdist = env.Tarball(env['TARBALL'], env['DISTTREE'])
env.Alias ('srctar', srcdist)

#
# don't leave the distree around
#
env.AddPreAction (env['DISTTREE'], Action ('rm -rf ' + str (File (env['DISTTREE']))))
env.AddPostAction (srcdist, Action ('rm -rf ' + str (File (env['DISTTREE']))))

#
# the subdirs
#

for subdir in coredirs:
    SConscript (subdir + '/SConscript')

for sublistdir in [ subdirs, gtk_subdirs, surface_subdirs ]:
    for subdir in sublistdir:
        SConscript (subdir + '/SConscript')

# cleanup
env.Clean ('scrub', [ 'scache.conf', '.sconf_temp', '.sconsign.dblite', 'config.log'])

