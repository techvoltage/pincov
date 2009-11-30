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
#include <string>
#include "pin.H"

using namespace std;

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 callcnt = 0;
static UINT64 retcnt = 0;

typedef struct stack
{
	struct stack * prev;
	ADDRINT call_addr;
	ADDRINT ret_addr;
} stack;

stack root;

ofstream OutFile;

// This function is called before every instruction is executed
VOID push(ADDRINT call, ADDRINT ret) {
	stack * new_st = (stack *)malloc(sizeof(stack));
	memset(new_st, 0x00, sizeof(stack));

	if(!root.prev)
	{
		new_st->call_addr = call;
		new_st->ret_addr = ret;
		root.prev = new_st;
	} else
	{

		stack * temp = root.prev;
	
		while(temp->prev)
		{
			temp = temp->prev;
		}

		new_st->call_addr = call;
		new_st->ret_addr = ret;

		temp->prev = new_st;
	}

	callcnt++;
}
VOID pop(const CONTEXT * cxt) {
retcnt++;
	ADDRINT value;
	ADDRINT stack_addr = PIN_GetContextReg(cxt, REG_ESP);
stack_addr+=4;
	PIN_SafeCopy(&value, &stack_addr, sizeof(ADDRINT));

	OutFile << "addr: " << value << " stack: " << stack_addr << endl;

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

    // Insert a call to docount before every instruction, no arguments are passed
    if(INS_IsCall(ins))
	{
    	INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)push, IARG_ADDRINT, INS_Address(ins), IARG_ADDRINT, INS_NextAddress(ins), IARG_END);
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
	stack * t = NULL;
	stack * temp = root.prev;
    // Write to a file since cout and cerr maybe closed by the application
    OutFile << "Call Count " << callcnt << endl;
    OutFile << "Ret Count " << retcnt << endl;

	while(temp)
	{
		t = temp;
		OutFile << "Call: " << std::hex << temp->call_addr << " Ret: ";
		OutFile << std::hex << temp->ret_addr << endl;

		temp = temp->prev;
		free(t);
	}
    OutFile.close();
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
	root.prev = NULL;
	root.call_addr = 0x00;
	root.ret_addr = 0x00;

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