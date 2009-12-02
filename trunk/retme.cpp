/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2009 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "pin.H"

using namespace std;

// The vector used to store the "stack" of return addresses.
vector<ADDRINT> st_v;

// The count of calls and rets.
static UINT64 callcnt = 0;
static UINT64 retcnt = 0;

// Output file
ofstream OutFile;

// This is executed when a CALL instruction is encountered
// 	It stores the valid return address.
VOID push(ADDRINT ret) {
	st_v.push_back(ret);
	callcnt++;
}

// Executed when a ret instruction is encountered
//  It is supposed to retrieve the stack pointer, copy the 4
//  bytes at the top of the stack and check that value against
//  the valid return address.
VOID pop(const CONTEXT * cxt) {
retcnt++;
	ADDRINT value;
	ADDRINT stack_addr = PIN_GetContextReg(cxt, REG_ESP);

	PIN_SafeCopy(&value, &stack_addr, sizeof(ADDRINT));

	OutFile << "value: " << value << " stack: " << stack_addr << endl;
	
}
    
// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
	IMG img;
	PIN_LockClient();
	img = IMG_FindByAddress(INS_Address(ins));
	if(!IMG_Valid(img))
		return;

	if(!IMG_IsMainExecutable(img))
		return;
	PIN_UnlockClient();

    if(INS_IsCall(ins))
	{
    	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)push, IARG_ADDRINT, INS_NextAddress(ins), IARG_END);
	} else if (INS_IsRet(ins))
	{
		INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(pop), IARG_CONTEXT, IARG_END);
	}
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "retme.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    OutFile << "Call Count " << callcnt << endl;
    OutFile << "Ret Count " << retcnt << endl;

	while(!st_v.empty())
	{
		OutFile << "End: " << st_v.back() << endl;
		st_v.pop_back();
	}
    OutFile.close();
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    OutFile.open(KnobOutputFile.Value().c_str());
    OutFile << std::hex;
    OutFile.setf(ios::showbase);

    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
   
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
