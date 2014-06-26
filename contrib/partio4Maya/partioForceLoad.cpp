#include "partioForceLoad.h"

// Reminder:
//   ID_PARTIOVISUALIZER  0x00116ECF
//   ID_PARTIOEMITTER     0x00116ED0
//   ID_PARTIOINSTANCER   0x00116ED5

// This should be safe
#define ID_PARTIOFORCELOAD 0x00116ED2

MTypeId partioForceLoad::id(ID_PARTIOFORCELOAD);

partioForceLoad::partioForceLoad()
   : MPxNode()
{
}

partioForceLoad::~partioForceLoad()
{
}

MStatus partioForceLoad::compute(const MPlug &plug, MDataBlock &data)
{
   return MStatus::kUnknownParameter;
}

void* partioForceLoad::creator()
{
   return new partioForceLoad();
}

MStatus partioForceLoad::initialize()
{
   return MStatus::kSuccess;
}
