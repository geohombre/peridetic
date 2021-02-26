//
//
// MIT License
//
// Copyright (c) 2021 Stellacore Corporation.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
// KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//


// common data store and I/O code for both hello{Sans,With}Peri.cpp
#include "helloCommon.h"


int
main
	()
{
	// allocate same data space used in the 'with Peridetic' example
	hello::WorkSpace workSpace;

	// mimic data transformation operations - but *w/o* Peridetic
	workSpace.theDataTmp = workSpace.theDataSrc;
	workSpace.theDataOut = workSpace.theDataTmp;

	// output data values to confirm that
	// (simulated) transform is actually present in executable
	using hello::operator<<;
	std::cout << workSpace << std::endl;
	return 0;
}

