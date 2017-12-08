#ifndef EFSW_FILEWATCHERLINUX_HPP
#define EFSW_FILEWATCHERLINUX_HPP

#include <efsw/FileWatcherImpl.hpp>

#if EFSW_PLATFORM == EFSW_PLATFORM_INOTIFY

#include <efsw/WatcherInotify.hpp>
#include <map>

namespace efsw
{

/// Implementation for Linux based on inotify.
/// @class FileWatcherInotify
class FileWatcherInotify : public FileWatcherImpl
{
	public:
		/// type for a map from WatchID to WatchStruct pointer
		typedef std::map<WatchID, WatcherInotify*> WatchMap;

		FileWatcherInotify( FileWatcher * parent );

		~FileWatcherInotify() override;

		/// Add a directory watch
		/// On error returns WatchID with Error type.
		WatchID addWatch(const std::string& directory, FileWatchListener* watcher, bool recursive) override;

		/// Remove a directory watch. This is a brute force lazy search O(nlogn).
		void removeWatch(const std::string& directory) override;

		/// Remove a directory watch. This is a map lookup O(logn).
		void removeWatch(WatchID watchid) override;

		/// Updates the watcher. Must be called often.
		void watch() override;

		/// Handles the action
		void handleAction(Watcher * watch, const std::string& filename, unsigned long action, std::string oldFilename = "") override;

		/// @return Returns a list of the directories that are being watched
		std::list<std::string> directories() override;
	protected:
		/// Map of WatchID to WatchStruct pointers
		WatchMap mWatches;

		/// User added watches
		WatchMap mRealWatches;

		/// inotify file descriptor
		int mFD;

		Thread * mThread;

		Mutex mWatchesLock;

		WatchID addWatch(const std::string& directory, FileWatchListener* watcher, bool recursive, WatcherInotify * parent = nullptr );

		bool pathInWatches( const std::string& path ) override;
	private:
		void run();

		void removeWatchLocked(WatchID watchid);

		void checkForNewWatcher( Watcher* watch, std::string fpath );

		Watcher * watcherContainsDirectory( std::string dir );
};

}

#endif

#endif
