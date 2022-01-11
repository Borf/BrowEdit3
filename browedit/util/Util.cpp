#include "Util.h"

namespace util
{
	short swapShort(const short s)
	{
		return ((s&0xff)<<8) | ((s>>8)&0xff);
	}
}