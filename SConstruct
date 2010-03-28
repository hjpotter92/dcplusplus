# vim: set filetype: py

EnsureSConsVersion(0, 98, 5)

import os,sys
from build_util import Dev, gen_po_name

gcc_flags = {
	'common': ['-g', '-Wall', '-Wextra', '-Wno-unused-parameter', '-Wno-missing-field-initializers', '-Wno-address', '-fexceptions', '-mthreads'],
	'debug': [], 
	'release' : ['-O2', '-mwindows']
}

gcc_xxflags = {
	'common' : [],
	'debug' : [],
	'release' : ['-fno-enforce-eh-specs']
}

msvc_flags = {
	# 4100: unreferenced formal parameter
	# 4121: alignment of member sensitive to packing
	# 4127: conditional expression is constant
	# 4189: var init'd, unused
	# 4290: exception spec ignored
	# 4510: no default constructor
	# 4512: assn not generated
	# 4610: no default constructor
	# 4800: converting from BOOL to bool
	# 4996: fn unsafe, use fn_s
	'common' : ['/W4', '/EHsc', '/Zi', '/GR', '/wd4100', '/wd4121', '/wd4127', '/wd4189', '/wd4290', '/wd4510', '/wd4512', '/wd4610', '/wd4800', '/wd4996'],
	'debug' : ['/MDd'],
	'release' : ['/MD', '/O2']
}

msvc_xxflags = {
	'common' : [],
	'debug' : [],
	'release' : []
}

gcc_link_flags = {
	'common' : ['-g', '-Wl,--no-undefined', '-time', '-mthreads'],
	'debug' : [],
	'release' : ['-mwindows']
}

msvc_link_flags = {
	'common' : ['/DEBUG', '/FIXED:NO', '/INCREMENTAL:NO', '/SUBSYSTEM:WINDOWS', '/MANIFESTUAC:NO'],
	'debug' : [],
	'release' : []
}

msvc_defs = {
	'common' : ['_REENTRANT'],
	'debug' : ['_DEBUG', '_HAS_ITERATOR_DEBUGGING=0', '_SECURE_SCL=0'],
	'release' : ['NDEBUG']
}

gcc_defs = {
	'common' : ['_REENTRANT'],
	'debug' : ['_DEBUG'],
	'release' : ['NDEBUG']
}

# --- cut ---

# defEnv will hold a temporary Environment used to get options that can't be set after the actual
# Environment has been created
defEnv = Environment(ENV = os.environ)
opts = Variables('custom.py', ARGUMENTS)
opts.AddVariables(
	EnumVariable('tools', 'Toolset to compile with, default = platform default (msvc under windows)', 'mingw', ['mingw', 'default']),
	EnumVariable('mode', 'Compile mode', 'debug', ['debug', 'release']),
	BoolVariable('nativestl', 'Use native STL instead of STLPort', 'yes'),
	BoolVariable('gch', 'Use GCH when compiling GUI (disable if you have linking problems with mingw)', 'yes'),
	BoolVariable('verbose', 'Show verbose command lines', 'no'),
	BoolVariable('savetemps', 'Save intermediate compilation files (assembly output)', 'no'),
	BoolVariable('unicode', 'Build a Unicode version which fully supports international characters', 'yes'),
	BoolVariable('i18n', 'Rebuild i18n files in debug build', 'no'),
	BoolVariable('help', 'Build help files (requires i18n=1 or mode=release)', 'yes'),
	BoolVariable('test', 'Build test suite', 'no'),
	BoolVariable('webhelp', 'Build help files for the web (requires help=1)', 'no'),
	('prefix', 'Prefix to use when cross compiling', ''),
	EnumVariable('arch', 'Target architecture', 'x86', ['x86', 'x64', 'ia64'])
)

opts.Update(defEnv)
Help(opts.GenerateHelpText(defEnv))

# workaround for SCons 1.2 which hard-codes possible archs (only allows 'x86' and 'amd64'...)
# TODO remove when SCons knows about all available archs
TARGET_ARCH = defEnv['arch']
if TARGET_ARCH == 'x64':
	TARGET_ARCH = 'amd64'

env = Environment(ENV = os.environ, tools = [defEnv['tools']], options = opts, TARGET_ARCH = TARGET_ARCH, MSVS_ARCH = TARGET_ARCH)

if 'mingw' not in env['TOOLS'] and 'gcc' in env['TOOLS']:
	print "Non-mingw gcc builds not supported"
	Exit(1)

mode = env['mode']
if mode not in gcc_flags:
	print "Unknown mode, exiting"
	Exit(1)

# filter out boost from dependencies to get a speedier rebuild scan
# this means that if boost changes, scons -c needs to be run
# delete .sconsign.dblite to see the effects of this if you're upgrading
def filterBoost(x):
	return [y for y in x if str(y).find('boost') == -1]

SourceFileScanner.function['.c'].recurse_nodes = filterBoost
SourceFileScanner.function['.cpp'].recurse_nodes = filterBoost
SourceFileScanner.function['.h'].recurse_nodes = filterBoost
SourceFileScanner.function['.hpp'].recurse_nodes = filterBoost

dev = Dev(mode, env['tools'], env)
dev.prepare()

env.SConsignFile()

env.Append(CPPPATH = ["#/boost/boost/tr1/tr1/", "#/boost/", "#/intl/"])

if env['nativestl']:
	if 'gcc' in env['TOOLS']:
		env.Append(CPPDEFINES = ['BOOST_HAS_GCC_TR1'])
	# boost detects MSVC's tr1 automagically
else:
	env.Append(CPPPATH = ['#/stlport/stlport/'])
	env.Append(LIBPATH = ['#/stlport/lib/'])
	env.Append(CPPDEFINES = ['HAVE_STLPORT', '_STLP_USE_STATIC_LIB=1'])
	if mode == 'debug':
		env.Append(LIBS = ['stlportg.5.1'])
	else:
		env.Append(LIBS = ['stlport.5.1'])

	# assume STLPort has tr1 containers
	env.Append(CPPDEFINES = ['BOOST_HAS_TR1'])

if 'gcc' in env['TOOLS']:
	if env['savetemps']:
		env.Append(CCFLAGS = ['-save-temps', '-fverbose-asm'])
	else:
		env.Append(CCFLAGS = ['-pipe'])

if env['unicode']:
	env.Append(CPPDEFINES = ['UNICODE', '_UNICODE'])

if env['CC'] == 'cl': # MSVC
	flags = msvc_flags
	xxflags = msvc_xxflags
	link_flags = msvc_link_flags
	defs = msvc_defs

	env.Append(LIBS = ['User32', 'shell32', 'Advapi32'])

else:
	flags = gcc_flags
	xxflags = gcc_xxflags
	link_flags = gcc_link_flags
	defs = gcc_defs

	env.Tool("gch", toolpath=".")

env.Append(CPPDEFINES = defs[mode])
env.Append(CPPDEFINES = defs['common'])

env.Append(CCFLAGS = flags[mode])
env.Append(CCFLAGS = flags['common'])

env.Append(LINKFLAGS = link_flags[mode])
env.Append(LINKFLAGS = link_flags['common'])

env.SourceCode('.', None)

import SCons.Scanner
SWIGScanner = SCons.Scanner.ClassicCPP(
	"SWIGScan",
	".i",
	"CPPPATH",
	'^[ \t]*[%,#][ \t]*(?:include|import)[ \t]*(<|")([^>"]+)(>|")'
)
env.Append(SCANNERS=[SWIGScanner])

#
# internationalization (ardour.org provided the initial idea)
#

po_args = ['msgmerge', '-q', '--update', '--backup=none', '$TARGET', '$SOURCE']
po_bld = Builder (action = Action([po_args], 'Updating translation $TARGET from $SOURCES'))
env.Append(BUILDERS = {'PoBuild' : po_bld})

mo_args = ['msgfmt', '-c', '-o', '$TARGET', '$SOURCE']
mo_bld = Builder (action = [Action([mo_args], 'Compiling message catalog $TARGET from $SOURCES'),
	Action(lambda target, source, env: gen_po_name(source[0], env), 'Generating $NAME_FILE')])
env.Append(BUILDERS = {'MoBuild' : mo_bld})

pot_args = ['xgettext', '--from-code=UTF-8', '--foreign-user', '--package-name=$PACKAGE',
		'--copyright-holder=Jacek Sieka', '--msgid-bugs-address=dcplusplus-devel@lists.sourceforge.net',
		'--no-wrap', '--keyword=_', '--keyword=T_', '--keyword=TF_', '--keyword=TFN_:1,2',
		'--keyword=F_', '--keyword=gettext_noop', '--keyword=N_', '--keyword=CT_', '--boost', '-s',
		'--output=$TARGET', '$SOURCES']

pot_bld = Builder (action = Action([pot_args], 'Extracting messages to $TARGET from $SOURCES'))
env.Append(BUILDERS = {'PotBuild' : pot_bld})

dev.zlib = dev.build('zlib/')
dev.bzip2 = dev.build('bzip2/')
dev.intl = dev.build('intl/')
dev.dwt = dev.build('dwt/')
dev.client = dev.build('dcpp/')
dev.help = dev.build('help/')
if env['test']:
	dev.test = dev.build('test/')
else:
	dev.win32 = dev.build('win32/')
	
