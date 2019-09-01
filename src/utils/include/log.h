/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <absl/types/optional.h>
#include <plog/Log.h>

#define logINFO plog::info
#define logWARNING plog::warning
#define logDEBUG plog::debug
#define logERROR plog::error
#define logDEBUG1 plog::verbose
#define logDEBUG2 plog::verbose
#define logDEBUG3 plog::verbose
#define logDEBUG4 plog::verbose

namespace plog {
class GloggFormatter {
  public:
    static util::nstring header()
    {
        return util::nstring();
    }

    static util::nstring
    format( const Record& record ) // This method returns a string from a record.
    {
        tm t;
        util::localtime_s( &t, &record.getTime().time );

        util::nstringstream ss;
        ss << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_hour << PLOG_NSTR( ":" )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_min << PLOG_NSTR( ":" )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 2 ) << t.tm_sec << PLOG_NSTR( "." )
           << std::setfill( PLOG_NSTR( '0' ) ) << std::setw( 3 ) << record.getTime().millitm
           << PLOG_NSTR( " " );
        ss << std::setfill( PLOG_NSTR( ' ' ) ) << std::setw( 5 ) << std::left
           << severityToString( record.getSeverity() ) << PLOG_NSTR( " " );
        ss << PLOG_NSTR( "[" ) << record.getTid() << PLOG_NSTR( "] " );
        ss << PLOG_NSTR( "[" ) << record.getFunc() << PLOG_NSTR( "@" ) << record.getLine()
           << PLOG_NSTR( "] " );
        ss << record.getMessage() << PLOG_NSTR( "\n" );

        return ss.str();
    }
};

inline void EnableLogging(bool isEnabled, uint8_t logLevel)
{
    if ( isEnabled ) {
        const auto severity = static_cast<plog::Severity>( logLevel );
        plog::get<0>()->setMaxSeverity( severity );
        plog::get<1>()->setMaxSeverity( severity );

        LOG(logINFO) << "Logging enabled at level " << plog::severityToString( severity );
    }
    else {
         LOG(logINFO) << "Logging disabled";
         plog::get<1>()->setMaxSeverity( plog::none );
    }
}

} // namespace plog
#endif //__LOG_H__
