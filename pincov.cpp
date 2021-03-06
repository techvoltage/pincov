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
#include "pin.H"

struct node_t
{
        struct node_t * next;
        struct node_t * prev;
        ADDRINT head;
        USIZE len;
};

struct node_t root;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "mypin.out", "specify output file name");

VOID List(ADDRINT addr, UINT32 size)
{
	PIN_LockClient();
	IMG img = IMG_FindByAddress(addr);
	PIN_UnlockClient();
	if(!IMG_Valid(img))
	{
		return;
	}

	if(!IMG_IsMainExecutable(img))
		return;
	
	struct node_t * new_node = (struct node_t *)malloc(sizeof(struct node_t));

        if(!new_node)
        {
                fprintf(stderr, "ERROR: ListAddNode: malloc\n");
                return;
        }
        
        new_node->head = addr;
        new_node->len = size;

        struct node_t * marker = root.next;

        if( !marker )
        {
                root.next = new_node;
                root.prev = new_node;
                new_node->next = &root;
                new_node->prev = &root;
                return;
	}

	while(marker != & root)
	{
		if(marker->head == addr)
		{
			free(new_node);
			return;
		}

		if(addr < marker->head)
		{
			new_node->prev = marker->prev;
			new_node->next = marker;
			marker->prev->next = new_node;
			marker->prev = new_node;
			return;
		}

		marker = marker->next;
	}

	return;
}

VOID Coalesce()
{
	struct node_t * marker = root.prev;
	struct node_t * temp = NULL;

	while(marker->prev != & root)
	{
		if(marker->prev->head + marker->prev->len == marker->head)
		{
			marker->prev->len += marker->len;
			marker->next->prev = marker->prev;
			marker->prev->next = marker->next;

			temp = marker->next;
			free(marker);

			marker = temp;			
		}
		
		marker = marker->prev;
	}

}
 
// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v)
{
    // Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to docount for every bbl, passing the number of instructions.
        // IPOINT_ANYWHERE allows Pin to schedule the call anywhere in the bbl to obtain best performance.
        // Use a fast linkage for the call.
        BBL_InsertCall(bbl, IPOINT_ANYWHERE, AFUNPTR(List), IARG_ADDRINT, BBL_Address(bbl), IARG_UINT32, BBL_Size(bbl), IARG_END);
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
	ofstream OutFile;
	OutFile.open(KnobOutputFile.Value().c_str());
    	OutFile.setf(ios::showbase);

	struct node_t * marker = root.next;

	Coalesce();

	while(marker!=&root)
	{
		OutFile << "ADDR: " << std::hex << marker->head << " SIZE: " << marker->len << endl;
		marker = marker->next;
	}

	OutFile.close();
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
int main(int argc, char * argv[])
{
    memset(&root, 0x00, sizeof(root));
    root.head = 0xffffffff;

    // Initialize pin
    PIN_Init(argc, argv);

    // Register Instruction to be called to instrument instructions
    TRACE_AddInstrumentFunction(Trace, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 1;
}
