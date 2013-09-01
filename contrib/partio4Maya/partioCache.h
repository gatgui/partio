#ifndef Partio4MayaCache_H
#define Partio4MayaCache_H

#include <maya/MPxCacheFormat.h>
#include <maya/MCacheFormatDescription.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MTimeArray.h>

#include "Partio.h"
#include "partio4MayaShared.h"

class PartioCache : public MPxCacheFormat
{
public:

   PartioCache(const MString &ext);
   virtual ~PartioCache();

   virtual MStatus open( const MString& fileName, FileAccessMode mode);
   virtual void close();

   virtual MStatus isValid();
   virtual MStatus rewind();

   virtual MString extension();

   virtual MStatus readHeader();
   virtual MStatus writeHeader(const MString& version, MTime& startTime, MTime& endTime);


   virtual void beginWriteChunk();
   virtual void endWriteChunk();

   virtual MStatus beginReadChunk();
   virtual void endReadChunk();

   virtual MStatus writeTime(MTime& time);
   virtual MStatus readTime(MTime& time);
   virtual  MStatus findTime(MTime& time, MTime& foundTime);
   virtual MStatus readNextTime(MTime& foundTime);

   virtual unsigned  readArraySize();

   // Write data to the cache.
   virtual MStatus writeDoubleArray(const MDoubleArray&);
   virtual MStatus writeFloatArray(const MFloatArray&);
   virtual MStatus writeIntArray(const MIntArray&);
   virtual MStatus writeDoubleVectorArray(const MVectorArray& array);
   virtual MStatus writeFloatVectorArray(const MFloatVectorArray& array);
   virtual MStatus writeInt32(int);

   // Read data from the cache.
   virtual MStatus readDoubleArray(MDoubleArray&, unsigned size);
   virtual MStatus readFloatArray(MFloatArray&, unsigned size);
   virtual MStatus readIntArray(MIntArray&, unsigned size);
   virtual MStatus readDoubleVectorArray(MVectorArray&, unsigned arraySize);
   virtual MStatus readFloatVectorArray(MFloatVectorArray& array, unsigned arraySize);
   virtual int readInt32();

   virtual MStatus writeChannelName(const MString & name);
   virtual MStatus findChannelName(const MString & name);
   virtual MStatus readChannelName(MString& name);

   // Read and write the description file.
   virtual bool handlesDescription();
   virtual MStatus readDescription(MCacheFormatDescription& description,
                                   const MString& descriptionFileLocation,
                                   const MString& baseFileName );
   virtual MStatus writeDescription(const MCacheFormatDescription& description,
                                    const MString& descriptionFileLocation,
                                    const MString& baseFileName );
   
   // ---

   static size_t TotalNumFormats();
   static const char* FormatExtension(size_t n);

   template <size_t D>
   static void* Create()
   {
      const char *ext = FormatExtension(D);
      if (ext)
      {
         return new PartioCache(ext);
      }
      else
      {
         return 0;
      }
   }

private:

   void clean();
   void resetReadAttr();
   void resetWriteAttr();
   void resetPartioData();
   MString nodeName(const MString &channel);
   MString attrName(const MString &channel);

   MString mFilename;
   MString mDirname;
   MString mBasenameNoExt;
   FileAccessMode mMode;
   MString mExt;
   partio4Maya::CacheFiles mCacheFiles;
   partio4Maya::CacheFiles::iterator mCurSample;
   partio4Maya::CacheFiles::iterator mLastSample;
   Partio::ParticlesData *mRData;
   Partio::ParticlesDataMutable *mWData;
   MString mWChan;
   MTime mWTime;
   int mRAttrIdx;
   Partio::ParticleAttribute mRAttr;

   static std::vector<const char*> msAllFormats;
};

#endif
