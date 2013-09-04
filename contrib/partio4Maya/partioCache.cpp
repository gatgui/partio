#include "partioCache.h"
#include <maya/MAnimControl.h>

template <typename PT, int D, typename MT>
struct ArrayWriter
{
   static void Write(PT *data, const MT &mayaData)
   {
      for (unsigned int i=0; i<mayaData.length(); ++i, data+=D)
      {
         for (int j=0; j<D; ++j)
         {
            data[j] = PT(mayaData[i][j]);
         }
      }
   }
};

template <typename PT, typename MT>
struct ArrayWriter<PT, 1, MT>
{
   static void Write(PT *data, const MT &mayaData)
   {
      for (unsigned int i=0; i<mayaData.length(); ++i, ++data)
      {
         *data = PT(mayaData[i]);
      }
   }
};

template <typename PT, int D, typename MT>
struct ArrayReader
{
   static void Read(const PT *data, MT &mayaData)
   {
      for (unsigned long i=0; i<mayaData.length(); ++i, data+=D)
      {
         for (int j=0; j<D; ++j)
         {
            mayaData[i][j] = data[j];
         }
      }
   }
};

template <typename PT, typename MT>
struct ArrayReader<PT, 1, MT>
{
   static void Read(const PT *data, MT &mayaData)
   {
      for (unsigned long i=0; i<mayaData.length(); ++i, ++data)
      {
         mayaData[i] = *data;
      }
   }
};

template <Partio::ParticleAttributeType PT, int D, typename MT>
struct ArrayAccessor
{
   typedef typename Partio::ETYPE_TO_TYPE<PT>::TYPE PIOType;

   static MStatus Write(Partio::ParticlesDataMutable *partData, const MString &name, const MT &mayaData)
   {
      if (!partData)
      {
#ifdef _DEBUG
         MGlobal::displayInfo("ArrayAccessor::Write: No PartIO data");
#endif
         return MStatus::kFailure;
      }

      if (partData->numParticles() <= 0)
      {
         partData->addParticles(mayaData.length());
      }
      else if (partData->numParticles() != int(mayaData.length()))
      {
#ifdef _DEBUG
         MGlobal::displayInfo("ArrayAccessor::Write: Particle count mismatch");
#endif
         return MStatus::kFailure;
      }

      Partio::ParticleAttribute pattr;

      if (!partData->attributeInfo(name.asChar(), pattr))
      {
         pattr = partData->addAttribute(name.asChar(), PT, D);
      }

      PIOType *data = partData->dataWrite<PIOType>(pattr, 0);

      ArrayWriter<PIOType, D, MT>::Write(data, mayaData);

      return MStatus::kSuccess;
   }

   static MStatus Read(Partio::ParticlesData *partData, const Partio::ParticleAttribute &pattr, MT &mayaData)
   {
      const PIOType *data = partData->data<PIOType>(pattr, 0);

      mayaData.setLength(partData->numParticles());

      ArrayReader<PIOType, D, MT>::Read(data, mayaData);

      return MStatus::kSuccess;
   }
};

// ---

PartioCache::PartioCache(const MString &ext)
   : MPxCacheFormat()
   , mMode(MPxCacheFormat::kRead)
   , mExt(ext)
   , mRData(0)
   , mWData(0)
   , mLastTime(-1000.0)
{
#ifdef _DEBUG
   MGlobal::displayInfo("Create PartioCache \"" + mExt + "\"");
#endif
   mCurSample = mCacheFiles.end();
   mLastSample = mCacheFiles.end();
}

PartioCache::~PartioCache()
{
#ifdef _DEBUG
   MGlobal::displayInfo("Destroy PartioCache \"" + mExt + "\"");
#endif
   clean();
}


MStatus PartioCache::isValid()
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache isValid: ") + (mCurSample != mCacheFiles.end() ? "true" : "false"));
#endif
   return (mCurSample != mCacheFiles.end() ? MStatus::kSuccess : MStatus::kFailure);
}

MStatus PartioCache::rewind()
{
   // This should be a NOOP as we have one file per sample
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache rewind");
#endif
   if (mCacheFiles.size() > 0)
   {
      close();
      if (!open(mFilename, mMode))
      {
         return MStatus::kFailure;
      }
      else
      {
         return MStatus::kSuccess;
      }
   }
   else
   {
      return MStatus::kFailure;
   }
}

MString PartioCache::extension()
{
   return mExt;
}

MStatus PartioCache::open(const MString &fileName, MPxCacheFormat::FileAccessMode mode)
{
   MTime curTime = MAnimControl::currentTime();
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache open \"" + fileName + MString("\" ") + (mode == MPxCacheFormat::kRead ? "R" : (mode == MPxCacheFormat::kWrite ? "W" : "RW")));
   MGlobal::displayInfo(MString("  current time: ") + curTime.value());
#endif

   bool success = true;

   //bool samefile = (fileName == mFilename);
   bool samefile = (curTime == mLastTime);
   if (samefile)
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Re-use last sample");
#endif
      mCurSample = mLastSample;
   }
   else
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Reset last sample");
#endif
      mLastSample = mCacheFiles.end();
   }

   mFilename = fileName;
   mMode = mode;
   mLastTime = curTime;

   // if we are opening the same file, only check if it is valid, else readHeader (to select current sample)
   //if ((samefile && isValid()) || (readHeader() == MStatus::kSuccess))
   if (!samefile || isValid())
   {
      if (mode == MPxCacheFormat::kRead)
      {
         if (!samefile)
         {
            success = success && (readHeader() == MStatus::kSuccess);
         }
         success = success && (Partio::readFormatIndex(mExt.asChar()) != Partio::InvalidIndex && mCacheFiles.size() > 0);
      }
      else if (mode == MPxCacheFormat::kWrite)
      {
         // Do not try to read header
         success = (Partio::writeFormatIndex(mExt.asChar()) != Partio::InvalidIndex);
      }
      else
      {
         // Read/Write
         // Try to read header but do not fail if file doesn't exists
         if (!samefile)
         {
            readHeader();
         }
         success = success && (Partio::writeFormatIndex(mExt.asChar()) != Partio::InvalidIndex && Partio::readFormatIndex(mExt.asChar()) != Partio::InvalidIndex);
      }
   }

#ifdef _DEBUG
   MGlobal::displayInfo(success ? "  Open succeeded" : "  Open failed");
#endif

   return (success ? MStatus::kSuccess : MStatus::kFailure);
}

void PartioCache::close()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache close");
#endif
   mCurSample = mCacheFiles.end();
   //resetWriteAttr();
   //resetReadAttr();
   //resetPartioData();
}

void PartioCache::resetWriteAttr()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache resetWriteAttr");
#endif
   mWChan = "";
   mWTime = 0.0;
}

void PartioCache::resetReadAttr()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache resetReadAttr");
#endif
   mRAttrIdx = -1;
   mRAttr.type = Partio::NONE;
   mRAttr.attributeIndex = -1;
   mRAttr.count = 0;
   mRAttr.name = "";
}

void PartioCache::resetPartioData()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache resetPartioData");
#endif
   if (mRData)
   {
      mRData->release();
      mRData = 0;
   }
   if (mWData)
   {
      mWData->release();
      mWData = 0;
   }
   resetReadAttr();
   resetWriteAttr();
}

void PartioCache::clean()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache clean");
#endif
   close();
   resetPartioData();
   mFilename = "";
   mDirname = "";
   mBasenameNoExt = "";
   mMode = MPxCacheFormat::kRead;
   mCacheFiles.clear();
   mLastSample = mCacheFiles.end();
   mLastTime.setValue(-1000.0);
}

MStatus PartioCache::readHeader()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readHeader");
#endif

   close();
   
   // set mCurSample
   std::string frm = mFilename.asChar();
   
   // Strip directory
   size_t p = frm.find_last_of("\\/");
   if (p != std::string::npos)
   {
      frm = frm.substr(p+1);
   }

   // Strip extension
   p = frm.rfind('.');
   if (p != std::string::npos)
   {
      frm = frm.substr(0, p);
   }

   // Frame should end with Frame%d or Frame%dTick%d
   p = frm.rfind("Frame");
   if (p != std::string::npos)
   {
      frm = frm.substr(p);
   }

   int frame = -1;
   int ticks = -1;
   int rv = sscanf(frm.c_str(), "Frame%dTick%d", &frame, &ticks);

   // Beware: this won't work if cache time has an offset or scale
   //         but I couldn't find a way to get back the right sample time from those
   //         (because of maya limited time precision and the way nCache produce its samples)
   MTime targetTime = MAnimControl::currentTime();
   mCurSample = mCacheFiles.find(targetTime);
   /*
   if (rv == 2)
   {
      MTime targetTime(double(frame) + ticks * MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit()), MTime::uiUnit());
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Frame ") + double(frame) + ", Tick " + double(ticks) + " -> " + targetTime.value());
#endif
      mCurSample = mCacheFiles.find(targetTime);
      return (mCurSample != mCacheFiles.end() ? MStatus::kSuccess : MStatus::kFailure);
   }
   else if (rv == 1)
   {
      MTime targetTime(double(frame), MTime::uiUnit());
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Frame ") + double(frame) + " -> " + targetTime.value());
#endif
      mCurSample = mCacheFiles.find(targetTime);
      return (mCurSample != mCacheFiles.end() ? MStatus::kSuccess : MStatus::kFailure);
   }

#ifdef _DEBUG
   MGlobal::displayWarning("  Could not extract frame number");
#endif
   mCurSample = mCacheFiles.end();
   return MStatus::kFailure;
   */

   return (mCurSample != mCacheFiles.end() ? MStatus::kSuccess : MStatus::kFailure);
}

MStatus PartioCache::beginReadChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache beginReadChunk");
#endif

   if (mCurSample == mCacheFiles.end())
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Invalid sample");
#endif
      return MStatus::kFailure;
   }

   if (mRData && mLastSample != mCurSample)
   {
      resetPartioData();
   }

   if (!mRData)
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Read sample from \"" + mCurSample->second + "\"");
#endif
      mRData = Partio::read(mCurSample->second.asChar());
      mLastSample = mCurSample;
   }
   
   resetReadAttr();

   return (mRData != NULL ? MStatus::kSuccess : MStatus::kFailure);
}

MStatus PartioCache::readTime(MTime &time)
{
   if (mCurSample != mCacheFiles.end())
   {
      time = mCurSample->first;
#ifdef _DEBUG
      MGlobal::displayInfo(MString("PartioCache readTime: ") + time.value());
#endif
      return MStatus::kSuccess;
   }
   else
   {
      return MStatus::kFailure;
   }
}

MStatus PartioCache::findTime(MTime &time, MTime &foundTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache findTime: ") + time.value());
#endif

   partio4Maya::CacheFiles::iterator fit;
   
   if (partio4Maya::findCacheFile(mCacheFiles, partio4Maya::FM_EXACT, time, fit))
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Found: ") + fit->first.value());
#endif
      mCurSample = fit;
      if (beginReadChunk() == MStatus::kSuccess)
      {
         readTime(foundTime);
         return MStatus::kSuccess;
      }
   }

   return MStatus::kFailure;
}

MStatus PartioCache::readNextTime(MTime &foundTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readNextTime");
#endif
   if (mCurSample != mCacheFiles.end())
   {
      ++mCurSample;
      if (mCurSample != mCacheFiles.end())
      {
#ifdef _DEBUG
         MGlobal::displayInfo(MString("  Found: ") + mCurSample->first.value());
#endif
         if (beginReadChunk() == MStatus::kSuccess)
         {
            readTime(foundTime);
            return MStatus::kSuccess;
         }
      }
   }
   return MStatus::kFailure;
}

MStatus PartioCache::readChannelName(MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readChannelName");
#endif
   if (mRData)
   {
      int N = mRData->numAttributes();
      ++mRAttrIdx;
      if (mRAttrIdx >= 0 && mRAttrIdx < N)
      {
         Partio::ParticleAttribute pattr;
         if (mRData->attributeInfo(mRAttrIdx, mRAttr))
         {
            // skip particle id
            if (mRAttr.name == "id" || mRAttr.name == "particleId")
            {
#ifdef _DEBUG
               MGlobal::displayInfo(MString("  Skip ") + mRAttr.name.c_str());
#endif
               ++mRAttrIdx;
               if (mRAttrIdx < N)
               {
                  if (!mRData->attributeInfo(mRAttrIdx, mRAttr))
                  {
                     return MStatus::kFailure;
                  }
               }
               else if (mRAttrIdx == N)
               {
                  name = "count";
#ifdef _DEBUG
                  MGlobal::displayInfo("  " + name);
#endif
                  return MStatus::kSuccess;
               }
               else
               {
                  return MStatus::kFailure;
               }
            }
            name = mRAttr.name.c_str();
#ifdef _DEBUG
            MGlobal::displayInfo("  " + name);
#endif
            return MStatus::kSuccess;
         }
      }
      else if (mRAttrIdx == mRData->numAttributes())
      {
         // count!
         name = "count";
#ifdef _DEBUG
         MGlobal::displayInfo("  " + name);
#endif
         return MStatus::kSuccess;
      }
   }
   return MStatus::kFailure;
}

MStatus PartioCache::findChannelName(const MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache findChannelName " + name);
#endif
   if (mRData)
   {
      Partio::ParticleAttribute pattr;
      if (name == "count")
      {
         // count
         mRAttrIdx = mRData->numAttributes();
         return MStatus::kSuccess;
      }
      else if (mRData->attributeInfo(name.asChar(), pattr))
      {
#ifdef _DEBUG
         MGlobal::displayInfo("  Found");
#endif
         mRAttrIdx = pattr.attributeIndex;
         mRAttr = pattr;
         return MStatus::kSuccess;
      }
   }
   return MStatus::kFailure;
}

unsigned PartioCache::readArraySize()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readArraySize");
#endif
   if (mRData)
   {
      if (mRAttrIdx >= 0 && mRAttrIdx < mRData->numAttributes()) // or <=
      {
#ifdef _DEBUG
         MGlobal::displayInfo(MString("  ") + mRData->numParticles());
#endif
         return mRData->numParticles();
      }
      else if (mRAttrIdx == mRData->numAttributes())
      {
#ifdef _DEBUG
         MGlobal::displayInfo("PartioCache readArraySize \"count\"");
         MGlobal::displayInfo(MString("  ") + 1);
#endif
         // count attribute
         return 1;
      }
   }
   return 0;
}

MStatus PartioCache::readDoubleArray(MDoubleArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readDoubleArray");
#endif
   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
      // can be the count attribute
      if (mRAttrIdx == mRData->numAttributes() && size == 1)
      {
#ifdef _DEBUG
         MGlobal::displayInfo("PartioCache readDoubleArray \"count\"");
#endif
         array.setLength(1);
         array[0] = mRData->numParticles();
         return MStatus::kSuccess;
      }
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readDoubleArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MDoubleArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readFloatArray(MFloatArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readFloatArray");
#endif
   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readFloatArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MFloatArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readIntArray(MIntArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readIntArray");
#endif
   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readIntArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::INT, 1, MIntArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readDoubleVectorArray(MVectorArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readDoubleVectorArray");
#endif
   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readDoubleVectorArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::VECTOR, 3, MVectorArray>::Read(mRData, mRAttr, array);
}

MStatus PartioCache::readFloatVectorArray(MFloatVectorArray &array, unsigned size)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache readFloatVectorArray");
#endif
   if (int(size) != mRData->numParticles() || mRAttrIdx < 0 || mRAttrIdx >= mRData->numAttributes())
   {
#ifdef _DEBUG
      MGlobal::displayWarning("PartioCache readFloatVectorArray failed");
#endif
      return MStatus::kFailure;
   }
   return ArrayAccessor<Partio::VECTOR, 3, MFloatVectorArray>::Read(mRData, mRAttr, array);
}

int PartioCache::readInt32()
{
#ifdef _DEBUG
   MGlobal::displayWarning("PartioCache readInt32, not implemeted");
#endif
   return 0;
}

void PartioCache::endReadChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache endReadChunk");
#endif
   // No Operation
}

MStatus PartioCache::writeHeader(const MString &version, MTime &startTime, MTime &endTime)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeHeader: " + version + " " + startTime.value() + " " + endTime.value());
#endif

   // set mCurSample
   std::string frm = mFilename.asChar();
   
   // Strip directory
   size_t p = frm.find_last_of("\\/");
   if (p != std::string::npos)
   {
      if (mDirname.length() == 0)
      {
         mDirname = frm.substr(0, p+1).c_str();
      }
      frm = frm.substr(p+1);
   }

   // Strip extension
   p = frm.rfind('.');
   if (p != std::string::npos)
   {
      frm = frm.substr(0, p);
   }

   // Frame should end with Frame%d or Frame%dTick%d
   p = frm.rfind("Frame");
   if (p != std::string::npos)
   {
      if (mBasenameNoExt.length() == 0)
      {
         mBasenameNoExt = frm.substr(0, p).c_str();
      }
      frm = frm.substr(p);
   }

   int frame = -1;
   int ticks = -1;
   int rv = sscanf(frm.c_str(), "Frame%dTick%d", &frame, &ticks);

   if (rv == 2)
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Frame ") + frame + ", Tick " + ticks);
#endif
      mWTime = MTime(double(frame) + ticks * MTime(1, MTime::k6000FPS).asUnits(MTime::uiUnit()), MTime::uiUnit());
   }
   else if (rv == 1)
   {
#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Frame ") + frame);
#endif
      mWTime = MTime(double(frame), MTime::uiUnit());
   }
   else
   {
#ifdef _DEBUG
      MGlobal::displayWarning("  Could not extract frame number");
#endif
      return MStatus::kFailure;
   }

   mCurSample = mCacheFiles.find(mWTime);
   if (mCurSample == mCacheFiles.end())
   {
      char fext[256];
      if (ticks != -1)
      {
         double tickframes = MTime(1, MTime::k6000FPS).asUnits(MTime::uiUnit());
         int subframe = int(floor(ticks * tickframes * 1000 + 0.5));
         sprintf(fext, ".%04d.%03d.%s", frame, subframe, mExt.asChar());
      }
      else
      {
         sprintf(fext, ".%04d.%s", frame, mExt.asChar());
      }
      MString filename = mDirname + mBasenameNoExt  + fext;
      mCacheFiles[mWTime] = filename;
      mCurSample = mCacheFiles.find(mWTime);
   }

   return (mCurSample != mCacheFiles.end() ? MStatus::kSuccess : MStatus::kFailure);
}

void PartioCache::beginWriteChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache beginWriteChunk");
#endif
   // May need to review this
   if (mWData)
   {
      mWData->release();
   }
   mWData = Partio::create();
}

MString PartioCache::nodeName(const MString &channel)
{
   int p = channel.rindexW('_');
   if (p == -1)
   {
      return "";
   }
   else
   {
      return channel.substring(0, p-1);
   }
}

MString PartioCache::attrName(const MString &channel)
{
   int p = channel.rindexW('_');
   if (p == -1)
   {
      return channel;
   }
   else
   {
      return channel.substring(p+1, channel.length()-1);
   }
}

MStatus PartioCache::writeTime(MTime &time)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache writeTime ") + time.value());
#endif
   mWTime = time;
   return MStatus::kSuccess;
}

MStatus PartioCache::writeChannelName(const MString &name)
{
#ifdef _DEBUG
   MGlobal::displayInfo(MString("PartioCache writeChannelName ") + name);
#endif
   mWChan = attrName(name);
   return MStatus::kSuccess;
}

MStatus PartioCache::writeDoubleArray(const MDoubleArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeDoubleArray");
#endif
   if (mWChan == "count")
   {
#ifdef _DEBUG
      MGlobal::displayInfo("  Ignore \"count\"");
#endif
      return MStatus::kSuccess;
   }
   return ArrayAccessor<Partio::FLOAT, 1, MDoubleArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeFloatArray(const MFloatArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeFloatArray");
#endif
   return ArrayAccessor<Partio::FLOAT, 1, MFloatArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeIntArray(const MIntArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeIntArray");
#endif
   return ArrayAccessor<Partio::INT, 1, MIntArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeDoubleVectorArray(const MVectorArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeDoubleVectorArray");
#endif
   return ArrayAccessor<Partio::VECTOR, 3, MVectorArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeFloatVectorArray(const MFloatVectorArray &array)
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache writeFloatVectorArray");
#endif
   return ArrayAccessor<Partio::VECTOR, 3, MFloatVectorArray>::Write(mWData, mWChan, array);
}

MStatus PartioCache::writeInt32(int value)
{
#ifdef _DEBUG
   char tmp[256];
   sprintf(tmp, "PartioCache::writeInt32: %d, not implemented", value);
   MGlobal::displayWarning(tmp);
#endif

   return MStatus::kSuccess;
}

void PartioCache::endWriteChunk()
{
#ifdef _DEBUG
   MGlobal::displayInfo("PartioCache endWriteChunk");
#endif
   if (mWData)
   {
      if (mWData->numParticles() > 0)
      {
         char fext[256];

         MString filename;

         int frame = int(floor(mWTime.asUnits(MTime::uiUnit())));
         sprintf(fext, ".%04d.%s", frame, mExt.asChar());

         filename = mDirname + mBasenameNoExt + MString(fext);

#ifdef _DEBUG
         MGlobal::displayInfo("  Write sample \"" + filename + "\"");
#endif

         Partio::write(filename.asChar(), *mWData, false);

         mWData->release();
         mWData = 0;
      }
      else
      {
#ifdef _DEBUG
         MGlobal::displayInfo("  No particles to write");
#endif
         // remove current sample
         mCacheFiles.erase(mCurSample);
      }
   }
}

bool PartioCache::handlesDescription()
{
   return true;
}

MStatus PartioCache::readDescription(MCacheFormatDescription &desc, const MString &descFileLoc, const MString &descFileName)
{
   // Be sure to start with a clean plate
   clean();

#ifdef _DEBUG
   MGlobal::displayInfo("Read description");
#endif

   //MTimeArray times;
   //MStringArray files;

   unsigned long n = partio4Maya::getFileList(descFileLoc, descFileName, mExt, mCacheFiles); //files, times);

   if (n == 0)
   {
#ifdef _DEBUG
      MGlobal::displayWarning("No cache file matching: " + descFileLoc + "/" + descFileName + ".*." + mExt);
#endif
      return MStatus::kFailure;
   }

   mDirname = descFileLoc;
   mBasenameNoExt = descFileName;
   //for (unsigned int i=0; i<n; ++i)
   //{
   //   mCacheFiles[times[i]] = files[i];
   //}
   mCurSample = mCacheFiles.end();
   mLastSample = mCacheFiles.end();

   //MTime start = times[0];
   //MTime end = times[0];
   //for (unsigned int i=1; i<times.length(); ++i)
   partio4Maya::CacheFiles::iterator fit = mCacheFiles.begin();
   MTime start = fit->first;
   MTime end = fit->first;
   ++fit;
   for (; fit!=mCacheFiles.end(); ++fit)
   {
      //if (times[i] < start)
      if (fit->first < start)
      {
         start = fit->first; //times[i];
      }
      //if (times[i] > end)
      if (fit->first > end)
      {
         end = fit->first; //times[i];
      }
   }

   MTime cstart(floor(start.value()), MTime::uiUnit());
   //MTime cend(ceil(end.value()), MTime::uiUnit());

#ifdef _DEBUG
   MGlobal::displayInfo(MString("  Cache range: ") + start.value() + "-" + end.value());
#endif

   Partio::ParticlesInfo *pinfo = Partio::readHeaders(mCacheFiles.begin()->second.asChar());

   if (!pinfo)
   {
#ifdef _DEBUG
      MGlobal::displayWarning("Could not read particle header for " + mCacheFiles.begin()->second);
#endif
      return MStatus::kFailure;
   }
   
   // for the channels
   MTime srate(1.0, MTime::uiUnit());
   if (mCacheFiles.size() > 2)
   {
      // This pre-supposes that the cache has been uniformly sampled
      // Beware though that because of maya time precision limitation

      double accumsteps = 0.0;
      int nsteps = 0;

      partio4Maya::CacheFiles::iterator it1 = mCacheFiles.begin();
      partio4Maya::CacheFiles::iterator it2 = it1;
      ++it2;
      while (it2 != mCacheFiles.end())
      {
         accumsteps += (it2->first.value() - it1->first.value());
         ++nsteps;
         it1 = it2;
         ++it2;
      }

#ifdef _DEBUG
      MGlobal::displayInfo(MString("  Average sample step: ") + (accumsteps / nsteps));
#endif

      // If we do not set srate, sub-samples won't be queried...
      srate = (accumsteps / nsteps);

#ifdef _DEBUG
      MGlobal::displayInfo(MString("  -> rate = ") + srate.value());
#endif
   }

   mStart = cstart;
   mSamplRate = srate;

   desc.setDistribution(MCacheFormatDescription::kOneFilePerFrame);
   desc.setTimePerFrame(MTime(1, MTime::uiUnit()));
   desc.addDescriptionInfo("PartIO cache ." + mExt);

   for (int i=0; i<pinfo->numAttributes(); ++i)
   {
      Partio::ParticleAttribute pattr;
      if (pinfo->attributeInfo(i, pattr))
      {
         // name should be prefixed by "<shapename>_" ?
         MString name = pattr.name.c_str();
         if (name == "id" || name == "particleId")
         {
#ifdef _DEBUG
            MGlobal::displayInfo("  Skip channel: " + name);
#endif
            continue;
         }
#ifdef _DEBUG
         MGlobal::displayInfo("  Found channel: " + name);
#endif
         switch (pattr.type)
         {
         case Partio::INT:
            desc.addChannel(name, name, MCacheFormatDescription::kInt32Array, MCacheFormatDescription::kRegular, srate, cstart, end);
            break;
         case Partio::FLOAT:
            desc.addChannel(name, name, MCacheFormatDescription::kDoubleArray, MCacheFormatDescription::kRegular, srate, cstart, end);
            break;
         case Partio::VECTOR:
            desc.addChannel(name, name, MCacheFormatDescription::kDoubleVectorArray, MCacheFormatDescription::kRegular, srate, cstart, end);
            break;
         case Partio::INDEXEDSTR:
         default:
            //desc.addChannel(name, name, MCacheFormatDescription::kUnknownData, MCacheFormatDescription::kRegular, srate, cstart, end);
            continue;
         }
      }
   }

   // At last, add count attribute
   desc.addChannel("count", "count", MCacheFormatDescription::kDoubleArray, MCacheFormatDescription::kRegular, srate, cstart, end);
   //desc.addChannel("count", "count", MCacheFormatDescription::kDoubleArray, MCacheFormatDescription::kRegular, srate, start, end);
   
   pinfo->release();

   return MStatus::kSuccess;
}

MStatus PartioCache::writeDescription(const MCacheFormatDescription &desc, const MString &descFileLoc, const MString &descFileName)
{
#ifdef _DEBUG
   MGlobal::displayInfo("Write description " + descFileLoc + " " + descFileName);
#endif

   return MStatus::kSuccess;
}

// ---

std::vector<const char*> PartioCache::msAllFormats;

size_t PartioCache::TotalNumFormats()
{
   if (msAllFormats.size() == 0)
   {
      for (size_t i=0; i<Partio::numReadFormats(); ++i)
      {
         const char *ext = Partio::readFormatExtension(i);
         if (!strcmp(ext, "mc"))
         {
            // skip maya cache file
            continue;
         }
         msAllFormats.push_back(ext);
      }
      for (size_t i=0; i<Partio::numWriteFormats(); ++i)
      {
         const char *ext = Partio::writeFormatExtension(i);
         if (!strcmp(ext, "mc"))
         {
            // skip maya cache file
            continue;
         }
         if (Partio::readFormatIndex(ext) == Partio::InvalidIndex)
         {
            msAllFormats.push_back(ext);
         }
      }
   }

   return msAllFormats.size();
}

const char* PartioCache::FormatExtension(size_t n)
{
   return (n < TotalNumFormats() ? msAllFormats[n] : NULL);
}
