#include "MemoryCheckpoint.h"

namespace MemManag {
	MemoryCheckpoint::MemoryCheckpoint(const char* pFile, const char* pFunction) {
		m_pFile = pFile;
		m_pFunction = pFunction;
		_CrtMemCheckpoint(&m_Begin);
	}

	MemoryCheckpoint::~MemoryCheckpoint()
	{
		_CrtMemState end, diff;
		_CrtMemCheckpoint(&end);
		if (_CrtMemDifference(&diff, &m_Begin, &end) == 1) {
			sstr << "Leaks detected\n" <<
				"\tIn file -> " << m_pFile <<
				"\n\tIn Func -> " << m_pFunction << "\n";
			_CrtMemDumpAllObjectsSince(&m_Begin);
			_CrtMemDumpStatistics(&diff);
			strA = sstr.str();
			OutputDebugStringA(strA.c_str());
		}
	}
}