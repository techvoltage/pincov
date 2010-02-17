#include <iostream>
#include <vector>
#include <string>
#include <boost/tokenizer.hpp>
#include "pin.H"

using namespace std;
using namespace boost;

static BOOL DebugInterpreter(THREADID, CONTEXT *, const string &, string *, VOID *);

int main(int argc, char **argv)
{

	if( PIN_Init(argc, argv) == TRUE )
	{
		cout << "Error Parsing Pin Arguments." << endl;
		return -1;
	}

	/* Determine if the application as been set to debug mode which is required
		to use the gdb extension commands. If not, print usage.
	 */
	if( PIN_GetDebugStatus() == DEBUG_STATUS_DISABLED )
	{
		cout << "USAGE:" << endl;
		cout << "\t../../../pin -appdebug -t " << argv[0] << " -- <progname>" << endl;
		return -1;
	}

	PIN_AddDebugInterpreter(DebugInterpreter, 0);

	cout << "Added debug callback" << endl;

	PIN_StartProgram();
	
	return 1;
}

static BOOL DebugInterpreter(THREADID, CONTEXT *cxt, const string &cmd, string *result, VOID *)
{
	/* You have to clear out the result string from previous calls to DebugInterpreter */
	*result = "";

	vector<string> parsedcmd;

	tokenizer<> tok(cmd);

	for(tokenizer<>::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
	{
		parsedcmd.push_back(*beg);
	}

	*result += parsedcmd.back();

	parsedcmd.clear();

	return 1;
}
