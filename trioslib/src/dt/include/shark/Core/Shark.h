/**
 * \mainpage Shark Machine Learning Library Ver. 2.0.0.
 * Shark is a modular C++ library for the design and
 * optimization of adaptive systems. It provides methods for linear
 * and nonlinear optimization, in particular evolutionary and
 * gradient-based algorithms, kernel-based learning algorithms and
 * neural networks, and various other machine learning
 * techniques. SHARK serves as a toolbox to support real world
 * applications as well as research in different domains of
 * computational intelligence and machine learning. The sources are
 * compatible with the following platforms: Windows, Solaris, MacOS X,
 * and Linux.
 *
 *  \date    2011
 *
 *  \par Copyright (c) 2007-2011:
 *      Institut f&uuml;r Neuroinformatik<BR>
 *      Ruhr-Universit&auml;t Bochum<BR>
 *      D-44780 Bochum, Germany<BR>
 *      Phone: +49-234-32-25558<BR>
 *      Fax:   +49-234-32-14209<BR>
 *      eMail: Shark-admin@neuroinformatik.ruhr-uni-bochum.de<BR>
 *      www:   http://www.neuroinformatik.ruhr-uni-bochum.de<BR>
 *
 *
 *  <BR><HR>
 *  This library is free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  GNU General Public License as published by the Free Software
 *  Foundation; either version 3, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, see <http://www.gnu.org/licenses/>.
 *  
 */
#ifndef SHARK_CORE_SHARK_H
#define SHARK_CORE_SHARK_H

#include <boost/version.hpp>
#include <boost/static_assert.hpp>

/**
 * \brief Bails out the compiler if the boost version is < 1.44.
 */
BOOST_STATIC_ASSERT( BOOST_VERSION >= 104400 );

#include <shark/Core/Logger.h>

#include <boost/assign.hpp>
#include <boost/config.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <map>
#include <string>

/**
 * \brief Convenience macro to specify arguments and corresponding description inline.
 *
 * See the following example for declaring a complex boost::signal:
 * \code
 *	typedef boost::signals2::signal< 
 *	void
 *	( 
 *		SHARK_ARGUMENT( const typename Algo::SolutionSetType & , "Actual result announced to the outside world" ),
 *		SHARK_ARGUMENT( const std::string &, "Result directory to place results into" ),
 *		SHARK_ARGUMENT( const std::string &, "Name of the optimizer" ),
 *		SHARK_ARGUMENT( const std::string &, "Name of the objective function" ),
 *		SHARK_ARGUMENT( std::size_t, "Seed of the experiment" ),
 *		SHARK_ARGUMENT( std::size_t, "Search space dimension" ),
 *		SHARK_ARGUMENT( std::size_t, "Objective space dimension" ),
 *		SHARK_ARGUMENT( std::size_t, "Current number of function evaluations" ),
 *		SHARK_ARGUMENT( double, "Timestamp of the results, in ms." ),
 *		SHARK_ARGUMENT( bool, "true if this is the final result" )
 *	)
 *	> event_type;
 * \endcode
 */
#define SHARK_ARGUMENT( argument, description ) argument

namespace shark {	

/**
 * \namespace General namespace of the whole Shark machine learning library.
 */

/**
 * \brief Models the build type.
 */
enum BuildType {
  RELEASE_BUILD_TYPE, ///< A release build.
  DEBUG_BUILD_TYPE ///< A debug build.
};

namespace tag {

/**
 * \namespace tag Tagging namespace for type-based dispatching.
 */

/**
 * \brief Tags the build type.
 */
struct BuildTypeTag {
  /**
   * \brief The build settings that the library has been compiled with.
   */
#if defined( _DEBUG ) || defined( DEBUG )
  static const BuildType VALUE = DEBUG_BUILD_TYPE; 
#else
  static const BuildType VALUE = RELEASE_BUILD_TYPE; 
#endif
};

/**
 * \brief Tags whether OpenMP has been enabled.
 */
struct OpenMpTag {
#ifdef _OPENMP
  static const bool VALUE = true; 
#else
  static const bool VALUE = false;
#endif
};

/**
 * \brief Tags official releases of the shark library.
 */
struct OfficialReleaseTag {
#ifdef SHARK_OFFICIAL_RELEASE
  static const bool VALUE = true;
#else	
  static const bool VALUE = false;
#endif
};
		
}

/**
 * \brief Allows for querying compile settings at runtime. Provides the 
 * current command line arguments to the rest of the library.
 */
class Shark {
 protected:
  // Not implemented
  Shark();
  Shark( const Shark & shark );
  Shark & operator=( const Shark & rhs );

  static int m_argc; ///< Number of command line arguments.
  static char ** mep_argv; ///< Pointer to command line arguments, might be NULL.
	
  static std::map< BuildType, std::string > m_buildTypeMap; ///< Translates BuildType's to human readable form.
 public:

  /**
   * \brief Models a version according to the major.minor.patch versioning scheme.
   */
  template<unsigned int major, unsigned int minor, unsigned int patch>
  struct Version {

    /** \brief Default printf-format for formatting version numbers. */
    static const char * DEFAULT_FORMAT() {
      return( "%d.%d.%d" );
    }

    /** @brief Returns the major revision number. */
    static unsigned int MAJOR() {
      return( major );
    }
		    
    /** @brief Returns the minor revision number. */
    static unsigned int MINOR() { 
      return( minor );
    }

    /** @brief Returns the patch revision number. */
    static unsigned int PATCH() { 
      return( patch );
    }

  };

  /**
   * \brief Marks the current version of the Shark Machine Learning Library.
   */
  typedef Version<
    2,
    9, 
    0
    > version_type;

  /**
   * \brief Marks the boost version Shark has been built against.
   */
  typedef Version< 
    BOOST_VERSION / 100000,
    ((BOOST_VERSION / 100) % 1000),
    (BOOST_VERSION % 100)
    > boost_version_type;

  /**
   * \brief Initializes the library and stores command line arguments.
   * \param [in] argc Number of command line arguments.
   * \param [in] argv Array of command line arguments.
   */
  static void init( int argc, char **argv ) {
    m_argc = argc;
    mep_argv = argv;
			
    SHARK_LOG_DEBUG( logger(), "Shark library initialized.", "shark::Shark::init" );
  }
		
  /**
   * \brief Accesses the default logger of the shark library.
   */
  static const boost::shared_ptr< shark::Logger > & logger() {
    static boost::shared_ptr< shark::Logger > defaultLogger = shark::LoggerPool::instance().registerLogger( "edu.rub.ini.shark.logger" );
    return( defaultLogger );
  }

  /**
   * \brief Accesses the count of command line arguments.
   */
  static int argc() {
    return( m_argc );
  }

  /**
   * \brief Accesses the array of command line arguments.
   */
  static char ** argv() {
    return( mep_argv );
  }

  /**
   * \brief Accesses the build type of the library.
   */
  static BuildType buildType() {
    return( tag::BuildTypeTag::VALUE );
  }

  /**
   * \brief Queries whether Shark has been compiled with OpenMP enabled.
   */
  static bool hasOpenMp() {
    return( tag::OpenMpTag::VALUE );
  }

  /**
   * \brief Checks whether this is an official Shark release.
   */
  static bool isOfficialRelease() {
    return( tag::OfficialReleaseTag::VALUE );
  }

  /**
   * \brief Prints information about the Shark Machine Learning Library to the supplied stream.
   */
  template<typename Stream>
  static void info( Stream & s ) {
    boost::property_tree::ptree pt, version;
    version.add("major", version_type::MAJOR());
    version.add("minor", version_type::MINOR());
    version.add("patch", version_type::PATCH());

    pt.add_child("version", version);
    pt.add("isOfficialRelease", isOfficialRelease());
    pt.add("platform", BOOST_PLATFORM);
    pt.add("compiler", BOOST_COMPILER);
    pt.add("stdLib", BOOST_STDLIB);
    version.put("major", boost_version_type::MAJOR());
    version.put("minor", boost_version_type::MINOR());
    version.put("patch", boost_version_type::PATCH());
    pt.add_child("boostVersion", version);
    pt.add("buildType", m_buildTypeMap[buildType()]);
    pt.add("hasOpenMp", hasOpenMp());

    boost::property_tree::write_json(s, pt);
  }
		
};

/**
 * \brief Initializes the command line argument count to 0.
 */
int Shark::m_argc = 0;

/**
 * \brief Initializes the array of command line arguments to NULL.
 */
char ** Shark::mep_argv = NULL;

/**
 * \brief Fills in values to translate BuildTypes to human readable form.
 */
std::map< BuildType, std::string > Shark::m_buildTypeMap = boost::assign::map_list_of( RELEASE_BUILD_TYPE, "Release" )( DEBUG_BUILD_TYPE, "Debug" );

}

#endif 
