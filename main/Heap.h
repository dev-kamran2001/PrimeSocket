/*
 * MIT License
 * Copyright (c) 2023 Kamran
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#pragma warning(disable:4005) // Macro redefinition warning

class Heap
{
public:
#define malloc malloc
#define memcpy memcpy
	static void DbgZeroMemory(void* dst, int size)
	{
		for (int i = 0; i < size; i++)
		{
			if (*(DWORD*)((DWORD)dst + i) == heapSignature)
			{
				printf("\n[CIRITCAL-HEAP] Heap Corruption Detected!\n");
				throw std::invalid_argument("Heap Corrupted");
			}
			*(BYTE*)((DWORD)dst + i) = 0x0;
		}
	}
	static void* DbgCopyMemory(void* _Dst, void const* _Src, size_t _Size)
	{
		DWORD signature = *(DWORD*)((DWORD)_Dst + _Size);
		if (signature != heapSignature)
			return memcpy(_Dst, _Src, _Size);

		void* b = memcpy(_Dst, _Src, _Size);
		signature = *(DWORD*)((DWORD)_Dst + _Size);
		if (signature != heapSignature)
		{
			printf("\n[CIRITCAL-HEAP] Heap Corruption Detected!\n");
			throw std::invalid_argument("Heap Corrupted");
		}
		return b;
	}
	static void* DbgMalloc(size_t _Size)
	{
		void* block = malloc(_Size + sizeof(DWORD));
		*(DWORD*)((DWORD)block + _Size) = heapSignature;
		return block;
	}
	static void DbgHeapCheck(void* _HeapBlock, size_t _Size)
	{
		DWORD signature = *(DWORD*)((DWORD)_HeapBlock + _Size);
		if (signature != heapSignature)
		{
			printf("\n[CIRITCAL-HEAP] Heap Corruption Detected!\n");
			throw std::invalid_argument("Heap Corrupted");
		}
	}

private:
	static const DWORD heapSignature = 0xDEADBEEF;
};

// TODO: memset redefinition
#define ZeroMemory(Dest, Size) Heap::DbgZeroMemory((Dest), (Size))
#define memcpy(Dest, Src, Size) Heap::DbgCopyMemory((Dest), (Src), (Size));
#define malloc(Size) Heap::DbgMalloc((Size))