#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include "ldo.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"
}

#include <iostream>

#define toproto(L,i) (clvalue(L->top+(i))->l.p)
static const Proto* combine(lua_State* L, int n)
{
	if (n == 1)
		return toproto(L, -1);
	else
	{
		int i, pc;
		Proto* f = luaF_newproto(L);
		setptvalue2s(L, L->top, f); incr_top(L);
		f->source = luaS_newliteral(L, "=(" "asd" ")");
		f->maxstacksize = 1;
		pc = 2 * n + 1;
		f->code = luaM_newvector(L, pc, Instruction);
		f->sizecode = pc;
		f->p = luaM_newvector(L, n, Proto*);
		f->sizep = n;
		pc = 0;
		for (i = 0; i < n; i++)
		{
			f->p[i] = toproto(L, i - n - 1);
			f->code[pc++] = CREATE_ABx(OP_CLOSURE, 0, i);
			f->code[pc++] = CREATE_ABC(OP_CALL, 0, 1, 1);
		}
		f->code[pc++] = CREATE_ABC(OP_RETURN, 0, 1, 0);
		return f;
	}
}
static int writer(lua_State* L, const void* p, size_t size, void* u)
{
	UNUSED(L);
	return (fwrite(p, size, 1, (FILE*)u) != 1) && (size != 0);
}


void luadump(const char* luaFile, const char* lubFile)
{
	lua_State* L;
	L = lua_open();

	if (luaL_loadfile(L, luaFile) != 0)
		std::cerr << lua_tostring(L, -1) << std::endl;

	auto f = combine(L, 1);

	FILE* D = fopen(lubFile, "wb");
	lua_lock(L);
	luaU_dump(L, f, writer, D, false);
	lua_unlock(L);
	fclose(D);
}