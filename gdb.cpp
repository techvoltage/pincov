#include <iostream>
#include <boost/algorithm/string.hpp>
#include "pin.H"

using namespace std;
using namespace boost;

static BOOL DebugInterpreter(THREADID, CONTEXT *, const string &, string *, VOID *);

int main(int argc, char **argv)
{

	if( PIN_Init(argc, argv) == TRUE )
	{
		cerr << "Error Parsing Pin Arguments." << endl;
		return -1;
	}

	/* Determine if the application as been set to debug mode which is required
		to use the gdb extension commands. If not, print usage.
	 */
	if( PIN_GetDebugStatus() == DEBUG_STATUS_DISABLED )
	{
		cerr << "USAGE:" << endl;
		cerr << "\t../../../pin -appdebug -t " << argv[0] << " -- <progname>" << endl;
		return -1;
	}

	PIN_AddDebugInterpreter(DebugInterpreter, 0);

	PIN_StartProgram();
	
	return 1;
}

static BOOL DebugInterpreter(THREADID, CONTEXT *cxt, const string &cmd, string *result, VOID *)
{
	boost::algorithm::split_vector_type SplitVec;
	split( SplitVec, cmd, is_any_of(" ") );

	return 1;
}
