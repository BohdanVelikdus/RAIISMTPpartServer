#pragma once
#include <Windows.h>
#include <crtdbg.h>
#include <iostream>
#include <sstream>
#include <string>

namespace MemManag {

	// for corruption
	class HeapCheck
	{
	private:
		std::stringstream sstr;
		std::string strA;
		const char* m_pFunction;
		const char* m_pFile;
		bool m_Initialized = true;
	public:
		HeapCheck(const char* pFun, const char* pFile);
		~HeapCheck();
	};

#ifdef _DEBUG
#define HEAPCHECK MemManag::HeapCheck instance{__FUNCSIG__, __FILE__};
#else
#define HEAPCHECK
#endif
}