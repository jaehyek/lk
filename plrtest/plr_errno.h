#ifndef __PLR_ERROR__
#define __PLR_ERROR__

enum PLR_ERROR
{
	PLR_NOERROR = 0,

	//device level error
	PLR_ENODEV	= 1,
	PLR_EIO,
	PLR_ECLEAN,
	PLR_EINVAL,

	//verification level crash
	PLR_ECRASH = 5,

	//user defined error
	PLR_EUSERDEFINED,
};

#endif