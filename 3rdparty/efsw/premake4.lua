newoption { trigger = "verbose", description = "Build efsw with verbose mode." }
newoption { trigger = "strip-symbols", description = "Strip debugging symbols in other file ( only for relwithdbginfo configuration )." }

efsw_major_version	= "1"
efsw_minor_version	= "0"
efsw_patch_version	= "0"
efsw_version		= efsw_major_version .. "." .. efsw_minor_version .. "." .. efsw_patch_version

function get_include_paths()
	local function _insert_include_paths( file )
		local function _trim(s)
			return (s:gsub("^%s*(.-)%s*$", "%1"))
		end

		local paths = { }
		local lines = file:read('*all')
		
		for line in string.gmatch(lines, '([^\n]+)')
		do
			table.insert( paths, _trim( line ) )
		end
		
		file:close()
		
		return paths
	end

	local include_paths = { }
	local file = io.popen( "echo | gcc -Wp,-v -x c++ - -fsyntax-only 2>&1 | grep -v '#' | grep '/'", 'r' )

	include_paths = _insert_include_paths( file )
	
	if next(include_paths) == nil then
		file = io.popen( "echo | clang++ -Wp,-v -x c++ - -fsyntax-only 2>&1 | grep -v '#' | grep '/' | grep -v 'nonexistent'", 'r' )
		
		include_paths = _insert_include_paths( file )
		
		if next(include_paths) == nil then
			table.insert( include_paths, "/usr/include" )
			table.insert( include_paths, "/usr/local/include" )
		end
	end
	
	return include_paths
end

function inotify_header_exists()
	local efsw_include_paths = get_include_paths()
	
	for _,v in pairs( efsw_include_paths )
	do
		local cur_path = v .. "/sys/inotify.h"
		
		if os.isfile( cur_path ) then
			return true
		end
	end
	
	return false
end

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end
	
	return false
end

function is_vs()
	return ( string.starts(_ACTION,"vs") )
end

function conf_warnings()
	if not is_vs() then
		buildoptions{ "-Wall -Wno-long-long" }
	else
		defines { "_SCL_SECURE_NO_WARNINGS" }
	end
end

function conf_links()
	if not os.is("windows") and not os.is("haiku") then
		links { "pthread" }
	end

	if os.is("macosx") then
		links { "CoreFoundation.framework", "CoreServices.framework" }
	end
end

function conf_excludes()
	if os.is("windows") then
		excludes { "src/efsw/WatcherKqueue.cpp", "src/efsw/WatcherFSEvents.cpp", "src/efsw/WatcherInotify.cpp", "src/efsw/FileWatcherKqueue.cpp", "src/efsw/FileWatcherInotify.cpp", "src/efsw/FileWatcherFSEvents.cpp" }
	elseif os.is("linux") then
		excludes { "src/efsw/WatcherKqueue.cpp", "src/efsw/WatcherFSEvents.cpp", "src/efsw/WatcherWin32.cpp", "src/efsw/FileWatcherKqueue.cpp", "src/efsw/FileWatcherWin32.cpp", "src/efsw/FileWatcherFSEvents.cpp" }
	elseif os.is("macosx") then
		excludes { "src/efsw/WatcherInotify.cpp", "src/efsw/WatcherWin32.cpp", "src/efsw/FileWatcherInotify.cpp", "src/efsw/FileWatcherWin32.cpp" }
	elseif os.is("freebsd") then
		excludes { "src/efsw/WatcherInotify.cpp", "src/efsw/WatcherWin32.cpp", "src/efsw/WatcherFSEvents.cpp", "src/efsw/FileWatcherInotify.cpp", "src/efsw/FileWatcherWin32.cpp", "src/efsw/FileWatcherFSEvents.cpp" }
	end
	
	if os.is("linux") and not inotify_header_exists() then
		defines { "EFSW_INOTIFY_NOSYS" }
	end
end

solution "efsw"
	location("./make/" .. os.get() .. "/")
	targetdir("./bin")
	configurations { "debug", "release", "relwithdbginfo" }

	if os.is("windows") then
		osfiles = "src/efsw/platform/win/*.cpp"
	else
		osfiles = "src/efsw/platform/posix/*.cpp"
	end
	
	-- Activates verbose mode
	if _OPTIONS["verbose"] then
		defines { "EFSW_VERBOSE" }
	end

	if os.is("macosx") then
		-- Premake 4.4 needed for this
		if not string.match(_PREMAKE_VERSION, "^4.[123]") then
			local ver = os.getversion();

			if not ( ver.majorversion >= 10 and ver.minorversion >= 5 ) then
				defines { "EFSW_FSEVENTS_NOT_SUPPORTED" }
			end
		end
	end

	objdir("obj/" .. os.get() .. "/")
	
	project "efsw-static-lib"
		kind "StaticLib"
		language "C++"
		targetdir("./lib")
		includedirs { "include", "src" }
		files { "src/efsw/*.cpp", osfiles }
		conf_excludes()
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			targetname "efsw-static-debug"
			conf_warnings()

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "efsw-static-release"
			conf_warnings()
	
		configuration "relwithdbginfo"
			defines { "NDEBUG" }
			flags { "Optimize", "Symbols" }
			targetname "efsw-static-reldbginfo"
			conf_warnings()

	project "efsw-test"
		kind "ConsoleApp"
		language "C++"
		links { "efsw-static-lib" }
		files { "src/test/*.cpp" }
		includedirs { "include", "src" }
		conf_links()
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			targetname "efsw-test-debug"
			conf_warnings()

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "efsw-test-release"
			conf_warnings()
			
		configuration "relwithdbginfo"
			defines { "NDEBUG" }
			flags { "Optimize", "Symbols" }
			targetname "efsw-test-reldbginfo"
			conf_warnings()

	project "efsw-shared-lib"
		kind "SharedLib"
		language "C++"
		targetdir("./lib")
		includedirs { "include", "src" }
		files { "src/efsw/*.cpp", osfiles }
		defines { "EFSW_DYNAMIC", "EFSW_EXPORTS" }
		conf_excludes()
		conf_links()
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			targetname "efsw-debug"
			conf_warnings()

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "efsw"
			conf_warnings()

		configuration "relwithdbginfo"
			defines { "NDEBUG" }
			flags { "Optimize", "Symbols" }
			targetname "efsw"
			conf_warnings()
			
			if os.is("linux") or os.is("bsd") or os.is("haiku") then
				targetextension ( ".so." .. efsw_version )
				postbuildcommands { "sh ../../project/build.reldbginfo.sh " .. efsw_major_version .. " " .. efsw_minor_version .. " " .. efsw_patch_version .. " " .. iif( _OPTIONS["strip-symbols"], "strip-symbols", "" ) }
			end
