#include "HeapCheck.h"

namespace MemManag {
	HeapCheck::HeapCheck(const char* pFun, const char* pFile) : m_pFunction(pFun), m_pFile(pFile){}

	HeapCheck::~HeapCheck()
	{
		if (_CrtCheckMemory() == 0) {
			sstr << "### HEAP CORRUPTION DETECTED ###" << "\n";
			sstr << "\tIn function -> " << m_pFunction << "\n";
			sstr << "\tIn file-> " << m_pFile << "\n";
			strA = sstr.str();
			OutputDebugStringA(strA.c_str());
		}

	}
}