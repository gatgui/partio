#ifndef Partio4MayaForceLoad_H
#define Partio4MayaForceLoad_H

#include <maya/MPxNode.h>

class partioForceLoad : public MPxNode
{
public:
   static MTypeId id;

public:
   partioForceLoad();
   virtual ~partioForceLoad();
   
   virtual MStatus compute(const MPlug &plug, MDataBlock &data);
   
   static void* creator();
   static MStatus initialize();
};

#endif
