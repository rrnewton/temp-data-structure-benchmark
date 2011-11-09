#!/bin/sh

# cds library build script
# Maxim Khiszinsky 04.01.2009

# The following variables are defined and exported at the end of this script.
#
# LDFLAGS
# CFLAGS
# CXXFLAGS
# CXX
# CC
# BITSTOBUILD

usage()
{
    echo "Build helper script for one of the supported platforms"
    echo "Usage: build.sh \"options\""
    echo "       where options may be any of the following:"
    echo "       -c <C compiler name> Possible values are: gcc"
    echo "       -x <C++ compiler name> (e.g. g++, CC)"
    echo "       -p <Processor architecture> Possible values are:"
    echo "             x86, amd64 (x86_64), sparc, ia64, ppc64"
    echo "       -o <OS family> Possible values are:"
    echo "             linux, sunos (solaris), hpux, aix"
    echo "       -D <define> define"
    echo "       -b <bitsToBuild> (accepts '64', '32')"
    echo "       -l <extra linker options>"
    echo "       -z <extra compiler options>"
    echo "       -j <number of make jobs. default 2>"
    echo "       -h (to get help on the above commands)"
    echo "       --with-boost <path to boost include>"
    echo "       --debug-cxx-options <extra compiler options for debug target>"
    echo "       --debug-ld-options <extra linker options for debug target>"
    echo "       --release-cxx-options <extra compiler options for release target>"
    echo "       --release-ld-options <extra linker options for release target>"
    echo "       --clean clean all before building"
    echo "       --debug-test make unit test in debug mode"
    echo "       --amd64-use-128bit use 128bit (16byte) CAS on amd64"
}

ERROR_EXIT_CODE=1

MAKE=make

# Set up the default values for each parameter
debug=off                # by default debug is off
bitsToBuild=0            # unknown
makejobs=2
cppcompiler=g++
ccompiler=gcc
processor_arch=unknown
OS_FAMILY=unknown

BOOST_INCLUDE_PATH=
makeclean=off

MAKE_DEBUG_TEST=0

cxx_debug_options=
ld_debug_options=

cxx_release_options=
ld_release_options=

cxx_test_release_options=
ls_test_release_options=

cxx_release_optimization_level="-O3 -fno-strict-aliasing"

amd64_cxx_options=

OS_VERSION=
TOOLSET_SUFFIX=

while [ $# -gt 0 ]
   do
   case $1 in
   -c)
        ccompiler=$2
		shift 2
		;;
   -x)
        cppcompiler=$2
		shift 2
		;;
   -o)
		OS_FAMILY=$2
		shift 2
		;;
   -p)
        processor_arch=$2; shift 2
		;;
   -b)
        bitsToBuild=$2
		shift 2
		;;
   -l)
        linkeroptions="$linkeroptions $2"
		shift 2
		;;
   -z)
        compileroptions="$compileroptions $2"
		shift 2
		;;
   -j)
        makejobs=$2
		shift 2
		;;
   -h)
        usage
        exit $ERROR_EXIT_CODE;; 
	
    --clean)
	makeclean=on
	shift
	;;		
    --with-boost)
	BOOST_INCLUDE_PATH=$2
	shift 2
	;;	
    --debug-cxx-options)
	cxx_debug_options=$2
	shift 2
	;;
    --debug-ld-options)
	ld_debug_options=$2
	shift 2
	;;
    --release-cxx-options)
	cxx_release_options=$2
	shift 2
	;;
    --release-ld-options)
	ld_release_options=$2
	shift 2
	;;
    --with-make)
        MAKE=$2
	shift 2
	;;
    --platform-suffix)
        OS_VERSION=$2		
	shift 2
	;;
	--toolset-suffix)
		TOOLSET_SUFFIX=-$2
	shift 2
	;;
    --debug-test)
        MAKE_DEBUG_TEST=1
    shift 1
    ;;
   --amd64-use-128bit)
        amd64_cxx_options='-mcx16'
    shift 1
    ;;
   --)
        shift; break;; 

   *)
       echo "unknown option $1"
       usage
       exit $ERROR_EXIT_CODE;;
   esac
done

# Determine compiler
case $ccompiler in
	gcc)
		if test $cppcompiler = ''; then
			cppcompiler=g++
		fi
		;;
	*)
		echo "ERROR: Unknown compiler: $ccompiler"
		exit $ERROR_EXIT_CODE
		;;
esac

# Determine OS family
if test $OS_FAMILY = 'unknown'; then
	OS_FAMILY=`uname |tr [A-Z] [a-z]|sed "s/-//"`
fi
case $OS_FAMILY in
	hp-ux)
		OS_FAMILY=hpux
		;;
	solaris)
		OS_FAMILY=sunos
		;;
    linux|sunos|hpux|aix|freebsd)
		;;
    *)
		echo "ERROR: Unknown operation system: $OS_FAMILY"
		exit $ERROR_EXIT_CODE
		;;
esac


# Determine processor architecture
if test $processor_arch = 'unknown'; then
	processor_arch=`uname -m|tr [A-Z] [a-z]`
fi
case $processor_arch in
	x86_64)
	    if test $bitsToBuild = 64; then
	        processor_arch='amd64'
	    else
		processor_arch='x86'
	    fi;
	    ;;
	x86|i686)
		if test $bitsToBuild = 64; then
			processor_arch='amd64'
		else
			processor_arch='x86'
		fi
		;;
	amd64|x86|ia64|sparc)
		;;
	*)
		processor_arch=`uname -p|tr [A-Z] [a-z]`
		case $processor_arch in
		    sparc|powerpc)
			;;
		    *)
			echo "ERROR: Unknown processor architecture: $processor_arch"
			exit ${ERROR_EXIT_CODE}
		esac			
		;;
esac	

# Determine compiler flags
case $ccompiler in
	gcc)
		case $processor_arch in
			amd64)
				case $OS_FAMILY in
					linux|freebsd)
						buildCXXflags="-m64 $amd64_cxx_options"
						buildCflags="-m64 $amd64_cxx_options"
						buildLDflags="-m64"
						buildTestLDflags="-m64"
						;;
					*)
						echo "ERROR: cannot determine compiler flags for processor $processor_arch, OS $OS_FAMILY, and compiler $ccompiler"
						exit ${ERROR_EXIT_CODE}
						;;
				esac
				;;
			x86)
				case $OS_FAMILY in
					linux|freebsd)
						buildCXXflags="-m32 -march=pentium4"
						buildCflags="-m32 -march=pentium4"
						buildLDflags="-m32"
						buildTestLDflags="-m32"
						;;
					*)
						echo "ERROR: cannot determine compiler flags for processor $processor_arch, OS $OS_FAMILY, and compiler $ccompiler"
						exit ${ERROR_EXIT_CODE}
						;;
				esac
				;;
			ia64)
			        bitsToBuild=64
				case $OS_FAMILY in
				        linux|freebsd)
						buildCXXflags="-mtune=itanium2"
						buildCflags="-mtune=itanium2"
						buildLDflags="-mtune=itanium2"
						buildTestLDflags="-mtune=itanium2"
						;;
					hpux)
						buildCXXflags="-mlp64 -mtune=itanium2"
						buildCflags="-mlp64 -mtune=itanium2"
						buildLDflags="-mlp64 -mtune=itanium2"
						buildTestLDflags="-mlp64 -mtune=itanium2"
						;;
					*)
						echo "ERROR: cannot determine compiler flags for processor $processor_arch, OS $OS_FAMILY, and compiler $ccompiler"
						exit ${ERROR_EXIT_CODE}
						;;
				esac
				;;
			sparc)
				bitsToBuild=64
				case $OS_FAMILY in
					sunos)
						buildCXXflags="-mcpu=v9 -mtune=ultrasparc3 -m64 -pthreads"
						buildCflags="-mcpu=v9 -mtune=ultrasparc3 -m64  -pthreads"
						buildLDflags="-mcpu=v9 -mtune=ultrasparc3 -m64 -pthreads"
						buildTestLDflags="-mcpu=v9 -mtune=ultrasparc3 -m64 -pthreads"
						cxx_test_release_options="-fPIC"
						ld_test_release_options="-fPIC"
						;;
					*)
						echo "ERROR: cannot determine compiler flags for processor $processor_arch, OS $OS_FAMILY, and compiler $ccompiler"
						exit ${ERROR_EXIT_CODE}
						;;
				esac
				;;
			powerpc)
                bitsToBuild=64
				case $OS_FAMILY in
					aix)
						buildCXXflags="-maix64 -pthread"
						buildCflags="-maix64 -pthread"
						buildLDflags="-maix64 -pthread"
						buildTestLDflags="-maix64 -pthread"
						cxx_test_release_options="-fPIC"
						ld_test_release_options="-fPIC"
						;;
					*)
						echo "ERROR: cannot determine compiler flags for processor $processor_arch, OS $OS_FAMILY, and compiler $ccompiler"
						exit ${ERROR_EXIT_CODE}
						;;
				esac
				;;
			*)
				echo "ERROR: cannot determine compiler flags for processor $processor_arch and compiler $ccompiler"
				exit ${ERROR_EXIT_CODE}
				;;
		esac
		
		cppcompiler_version=`$cppcompiler -dumpversion`
		echo g++ version=$gcc_version
		
		# Setup target options
		# buildCXXflags="-std=gnu++0x $buildCXXflags"
		cxx_debug_options="-D_DEBUG -O0 -g $cxx_debug_options"
		cxx_release_options="-DNDEBUG $cxx_release_optimization_level $cxx_release_options"
		;;
	*)
		echo "ERROR: Unknown compiler: $ccompiler"
		exit ${ERROR_EXIT_CODE}
		;;
esac

if test $BOOST_INCLUDE_PATH != ''; then
	buildCXXflags="$buildCXXflags -I$BOOST_INCLUDE_PATH"
fi

if test 'x$buildTestLDflags' = 'x'; then
	buildTestLDflags=$buildLDflags
fi


EXTRA_CXXFLAGS="$buildCXXflags $EXTRA_CXXFLAGS"
EXTRA_CFLAGS="$buildCflags $EXTRA_CFLAGS"
EXTRA_LDFLAGS="$buildLDflags $EXTRA_LDFLAGS"

EXTRA_TEST_LDFLAGS="$buildTestLDflags $EXTRA_TEST_LDFLAGS"


echo "Building with the following options ..."
echo "Processor: $processor_arch"
echo "Platform: $OS_FAMILY"
echo "C Compiler: $ccompiler"
echo "C++ Compiler: $cppcompiler"
echo "C++ Compiler version: $cppcompiler_version"
echo "Bits to build: $bitsToBuild"
echo "Compile options: $compileroptions $EXTRA_CXXFLAGS"
echo "Link options: $linkeroptions $EXTRA_LDFLAGS"
echo "Link options (cds-unit app): $linkeroptions $EXTRA_TEST_LDFLAGS"

BITSTOBUILD=$bitsToBuild
export BITSTOBUILD

#
# Set the C compiler and C++ compiler environment variables
#

CC="$ccompiler"
export CC

CXX="$cppcompiler"
export CXX

ROOT_DIR=..

GOAL_DIR=$ccompiler-$processor_arch-$OS_FAMILY$OS_VERSION$TOOLSET_SUFFIX-$bitsToBuild
BIN_PATH=$ROOT_DIR/bin/$GOAL_DIR
mkdir -p $BIN_PATH

OBJ_PATH=$ROOT_DIR/obj/$GOAL_DIR
mkdir -p $OBJ_PATH

echo PATH=$PATH
echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH
echo BIN_PATH=$BIN_PATH
echo OBJ_PATH=$OBJ_PATH
echo `${CXX} --version | head -1`
echo Build started

makegoals=
if test $makeclean = 'on'; then
   echo Clean all
   make -f Makefile clean platform=$OS_FAMILY BIN_PATH=$BIN_PATH OBJ_PATH=$OBJ_PATH
fi

echo ---------------------------------
echo Make debug library
CXXFLAGS="$compileroptions -fPIC $cxx_debug_options $EXTRA_CXXFLAGS"
export CXXFLAGS
CFLAGS="$compileroptions -fPIC $cxx_debug_options $EXTRA_CFLAGS $debugflag "
export CFLAGS
LDFLAGS="$linkeroptions -fPIC -shared $ld_debug_options $EXTRA_LDFLAGS "
export LDFLAGS

mkdir -p $OBJ_PATH/debug

$MAKE -f Makefile -j 8 \
     platform=$OS_FAMILY \
     BIN_PATH=$BIN_PATH \
     OBJ_PATH=$OBJ_PATH/debug \
     debug

if test $? -gt 0; then
   exit $?
fi

echo ---------------------------------
echo Make release library

CXXFLAGS="$compileroptions -fPIC $cxx_release_options $EXTRA_CXXFLAGS "
export CXXFLAGS
CFLAGS="$compileroptions -fPIC $cxx_release_options $EXTRA_CFLAGS "
export CFLAGS
LDFLAGS="$linkeroptions -fPIC -shared $ld_resease_options $EXTRA_LDFLAGS "
export LDFLAGS

mkdir -p $OBJ_PATH/release

$MAKE -f Makefile -j 8 \
     platform=$OS_FAMILY \
     BIN_PATH=$BIN_PATH \
     OBJ_PATH=$OBJ_PATH/release \
     release
     
if test $? -gt 0; then
   exit $?
fi


echo ---------------------------------
echo Make tests

if test $MAKE_DEBUG_TEST = '0'; then
    CXXFLAGS="$compileroptions $cxx_release_options $cxx_test_release_options $EXTRA_CXXFLAGS "
    export CXXFLAGS
    CFLAGS="$compileroptions $cxx_release_options $EXTRA_CFLAGS "
    export CFLAGS
    LDFLAGS="$linkeroptions $ld_release_options $ld_test_release_options $EXTRA_TEST_LDFLAGS "
    export LDFLAGS

    $MAKE -f Makefile -j 8 \
        platform=$OS_FAMILY \
        BIN_PATH=$BIN_PATH \
        OBJ_PATH=$OBJ_PATH/test \
        test
        
    if test $? -gt 0; then
        exit $?
    fi
fi    

echo ---------------------------------
echo Make tests debug

if test $MAKE_DEBUG_TEST = '1'; then
    CXXFLAGS="$compileroptions $cxx_debug_options $cxx_test_release_options $EXTRA_CXXFLAGS "
    export CXXFLAGS
    CFLAGS="$compileroptions $cxx_debug_options $EXTRA_CFLAGS "
    export CFLAGS
    LDFLAGS="$linkeroptions $ld_debug_options $ld_test_release_options $EXTRA_TEST_LDFLAGS "
    export LDFLAGS

    $MAKE -f Makefile -j 8 \
        platform=$OS_FAMILY \
        BIN_PATH=$BIN_PATH \
        OBJ_PATH=$OBJ_PATH/test-debug \
        test_debug
        
    if test $? -gt 0; then
        exit $?
    fi
fi    
