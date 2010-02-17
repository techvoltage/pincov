#include <iostream>
#include <vector>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "pin.H"

using namespace std;
using namespace boost;

/* Breakpoint on access globals */
typedef struct bpaInfo
{
	unsigned int bpaId;
	ADDRINT bpaAddress;
	unsigned int bpaLength;
	unsigned char operations;	// r == Read, w == write, x == execute
} BPAINFO;

unsigned int currentBpId;
vector<BPAINFO> bpaVector;
BOOL isInstInsEnabled;

static BOOL DebugInterpreter( THREADID, CONTEXT *, const string &, string *, VOID * );
BOOL HandleBpa( vector<string> );

int main(int argc, char **argv)
{
	if( PIN_Init(argc, argv) == TRUE )
	{
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

	cout << "before add callback " << endl;
	PIN_AddDebugInterpreter(DebugInterpreter, 0);

	cout << "Added debug callback" << endl;

	PIN_StartProgram();
	
	return 1;
}

BOOL HandleBpa(vector<string> cmd)
{
	/* bpa <addr> <len> <ops> */
	if(cmd.size() != 4)
	{
		return false;
	}

	ADDRINT addr = 0x00;

	/* Confirm validity of the bpa command syntax */

	/* If Instruction Instrumentation is not enabled then start it. */

	/* Add a new bpa vector */ 

	if(addr)
		addr++;
	return true;
}

static BOOL DebugInterpreter(THREADID, CONTEXT *cxt, const string &cmd, string *result, VOID *)
{
	/* You have to clear out the result string from previous calls to DebugInterpreter */
	*result = "";

	vector<string> parsedcmd;

	tokenizer<> tok(cmd);

	/* Create a vector of the command and its arguments */
	for(tokenizer<>::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
	{
		parsedcmd.push_back(*beg);
	}

	/* Create a temp string here so that each if statement is not
		required to run the at() function. Hopefully save as many cycles
		as I can. */
	string temp_cmd = parsedcmd.at(0);

	if(temp_cmd == "bpa")
	{
		if ( HandleBpa(parsedcmd) )	
		{
			*result += "Success\n";
		} else
		{
			*result += "Failed\n";
		}
	} else if(temp_cmd == "s")
	{
		*result += "Yay my search command\n";
	} else if (temp_cmd == "bpd")
	{
		*result += "Delete an access breakpoint\n";
	} else
	{
		*result += "As of yet unrecognized\n";
	}

	return 1;
}
