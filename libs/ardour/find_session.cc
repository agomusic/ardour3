#include <unistd.h>
#include <sys/stat.h>

#include <climits>
#include <cerrno>

#include <pbd/compose.h>
#include <pbd/error.h>

#include <ardour/session_utils.h>
#include <ardour/filename_extensions.h>
#include <ardour/utils.h>

#include "i18n.h"

using namespace PBD;

int
ARDOUR::find_session (string str, string& path, string& snapshot, bool& isnew)
{
	struct stat statbuf;
	char buf[PATH_MAX+1];

	isnew = false;

	if (!realpath (str.c_str(), buf) && (errno != ENOENT && errno != ENOTDIR)) {
		error << string_compose (_("Could not resolve path: %1 (%2)"), buf, strerror(errno)) << endmsg;
		return -1;
	}

	str = buf;
	
	/* check to see if it exists, and what it is */

	if (stat (str.c_str(), &statbuf)) {
		if (errno == ENOENT) {
			isnew = true;
		} else {
			error << string_compose (_("cannot check session path %1 (%2)"), str, strerror (errno))
			      << endmsg;
			return -1;
		}
	}

	if (!isnew) {

		/* it exists, so it must either be the name
		   of the directory, or the name of the statefile
		   within it.
		*/

		if (S_ISDIR (statbuf.st_mode)) {

			string::size_type slash = str.find_last_of ('/');
		
			if (slash == string::npos) {
				
				/* a subdirectory of cwd, so statefile should be ... */

				string tmp;
				tmp = str;
				tmp += '/';
				tmp += str;
				tmp += statefile_suffix;

				/* is it there ? */
				
				if (stat (tmp.c_str(), &statbuf)) {
					error << string_compose (_("cannot check statefile %1 (%2)"), tmp, strerror (errno))
					      << endmsg;
					return -1;
				}

				path = str;
				snapshot = str;

			} else {

				/* some directory someplace in the filesystem.
				   the snapshot name is the directory name
				   itself.
				*/

				path = str;
				snapshot = str.substr (slash+1);
					
			}

		} else if (S_ISREG (statbuf.st_mode)) {
			
			string::size_type slash = str.find_last_of ('/');
			string::size_type suffix;

			/* remove the suffix */
			
			if (slash != string::npos) {
				snapshot = str.substr (slash+1);
			} else {
				snapshot = str;
			}

			suffix = snapshot.find (statefile_suffix);
			
			if (suffix == string::npos) {
				error << string_compose (_("%1 is not an Ardour snapshot file"), str) << endmsg;
				return -1;
			}

			/* remove suffix */

			snapshot = snapshot.substr (0, suffix);
			
			if (slash == string::npos) {
				
				/* we must be in the directory where the 
				   statefile lives. get it using cwd().
				*/

				char cwd[PATH_MAX+1];

				if (getcwd (cwd, sizeof (cwd)) == 0) {
					error << string_compose (_("cannot determine current working directory (%1)"), strerror (errno))
					      << endmsg;
					return -1;
				}

				path = cwd;

			} else {

				/* full path to the statefile */

				path = str.substr (0, slash);
			}
				
		} else {

			/* what type of file is it? */
			error << string_compose (_("unknown file type for session %1"), str) << endmsg;
			return -1;
		}

	} else {

		/* its the name of a new directory. get the name
		   as "dirname" does.
		*/

		string::size_type slash = str.find_last_of ('/');

		if (slash == string::npos) {
			
			/* no slash, just use the name, but clean it up */
			
			path = legalize_for_path (str);
			snapshot = path;
			
		} else {
			
			path = str;
			snapshot = str.substr (slash+1);
		}
	}

	return 0;
}
