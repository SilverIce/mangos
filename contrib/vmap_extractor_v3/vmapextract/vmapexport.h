#ifndef VMAPEXPORT_H
#define VMAPEXPORT_H

#include <string>

enum ModelFlags
{
	MOD_M2 = 1,
	MOD_WORLDSPAWN = 1<<1,
    MOD_HAS_BOUND = 1<<2
};

extern const char * szWorkDirWmo;

bool ExtractSingleWmo(std::string& fname);

#endif
