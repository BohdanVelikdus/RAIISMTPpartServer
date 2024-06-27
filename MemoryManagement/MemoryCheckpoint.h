#pragma once
#include <crtdbg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <Windows.h>

namespace MemManag {
	class MemoryCheckpoint
	{
	public:
		MemoryCheckpoint(const char* pFile, const char* pFunction);
		~MemoryCheckpoint();

	private:
		_CrtMemState m_Begin;
		std::stringstream sstr;
		std::string strA;
		const char* m_pFile;
		const char* m_pFunction;
	};
#ifdef _DEBUG
#define MEMORYCHECKPOINT MemManag::MemoryCheckpoint cp{__FILE__, __FUNCSIG__};
#else
#define MEMORYCHECKPOINT 
#endif // _DEBUG
}