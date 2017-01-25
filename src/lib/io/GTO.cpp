#include "../Partio.h"
#include "../core/ParticleHeaders.h"

#include <Gto/Protocols.h>
#include <Gto/Reader.h>
#include <Gto/Writer.h>

#include <iostream>
#include <limits>
#include <map>
#include <deque>
#include <cstdlib>
#include <stdexcept>
#include <iterator>

namespace Partio
{

Gto::DataType PartioToGtoType[] = {Gto::ErrorType, Gto::Float, Gto::Float, Gto::Int, Gto::String};
const char* PartioTypeStr[] = {"none", "vector", "float", "int", "indexedstr"};
int PartioTypeDim[] = {0, 3, 1, 1, 1};

class PartioGtoReader : public Gto::Reader
{
public:
   
   typedef std::map<std::string, size_t> IndexMap;
   
   class PProp
   {
   public:
      PProp()
         : mIt(0)
      {
      }
      
      PProp(PropertyInfo *it)
         : mIt(it)
      {
      }
      
      PProp(const PProp &rhs)
      {
         operator=(rhs);
      }
      
      ~PProp()
      {
      }
      
      PProp& operator=(const PProp &rhs)
      {
         if (this != &rhs)
         {
            std::swap(mIt, rhs.mIt);
         }
         return *this;
      }
      
      int name() const
      {
         return (mIt ? mIt->name : -1);
      }
      
      Gto::DataType type() const
      {
         return (mIt ? Gto::DataType(mIt->type) : Gto::ErrorType);
      }
      
      unsigned int size() const
      {
         return (mIt ? mIt->size : 0);
      }
      
      unsigned int width() const
      {
         return (mIt ? mIt->width : 0);
      }
      
      int interpretation()
      {
         return (mIt ? mIt->interpretation : -1);
      }
      
      operator PropertyInfo& ()
      {
         static PropertyInfo _undefined;
         return (mIt ? *mIt : _undefined);
      }
      
   private:
      mutable PropertyInfo *mIt;
   };
   
   typedef std::deque<PProp> PropDQ;
   
   class PComp
   {
   public:
      PComp()
         : mIt(0)
      {
      }
      
      PComp(ComponentInfo *it)
         : mIt(it)
      {
      }
      
      PComp(const PComp &rhs)
      {
         operator=(rhs);
      }
      
      ~PComp()
      {
      }
      
      PComp& operator=(const PComp &rhs)
      {
         if (this != &rhs)
         {
            std::swap(mIt, rhs.mIt);
            std::swap(mPropIndices, rhs.mPropIndices);
            std::swap(mProps, rhs.mProps);
         }
         return *this;
      }
      
      int name() const
      {
         return (mIt ? mIt->name : -1);
      }
      
      PProp& addProperty(const std::string &name, PropertyInfo *pit)
      {
         IndexMap::iterator pmit = mPropIndices.find(name);
         if (pmit == mPropIndices.end())
         {
            mPropIndices[name] = mProps.size();
            mProps.push_back(PProp(pit));
            return mProps.back();
         }
         else
         {
            return mProps[pmit->second];
         }
      }
      
      size_t numProperties() const
      {
         return mProps.size();
      }
      
      bool hasProperty(const std::string &name) const
      {
         return (mPropIndices.find(name) != mPropIndices.end());
      }
      
      PProp& operator[](const std::string &name)
      {
         IndexMap::iterator it = mPropIndices.find(name);
         if (it == mPropIndices.end())
         {
            throw std::runtime_error("Invalid property name");
         }
         return mProps[it->second];
      }
      
      PProp& operator[](size_t idx)
      {
         return mProps[idx];
      }
      
      const PProp& operator[](const std::string &name) const
      {
         IndexMap::const_iterator it = mPropIndices.find(name);
         if (it == mPropIndices.end())
         {
            throw std::runtime_error("Invalid property name");
         }
         return mProps[it->second];
      }
      
      const PProp& operator[](size_t idx) const
      {
         return mProps[idx];
      }
      
   private:
      mutable ComponentInfo *mIt;
      mutable IndexMap mPropIndices;
      mutable PropDQ mProps;
   };
   
   typedef std::deque<PComp> PCompDQ;
   
   class PObj
   {
   public:
      PObj()
         : mIt(0)
      {
      }
      
      PObj(ObjectInfo *it)
         : mIt(it)
      {
      }
      
      PObj(const PObj &rhs)
      {
         operator=(rhs);
      }
      
      ~PObj()
      {
      }
      
      PObj& operator=(const PObj &rhs)
      {
         if (this != &rhs)
         {
            std::swap(mIt, rhs.mIt);
            std::swap(mCompIndices, rhs.mCompIndices);
            std::swap(mComps, rhs.mComps);
         }
         return *this;
      }
      
      int name() const
      {
         return (mIt ? mIt->name : -1);
      }
      
      PComp& addComponent(const std::string &name, ComponentInfo *cit)
      {
         IndexMap::iterator cmit = mCompIndices.find(name);
         if (cmit == mCompIndices.end())
         {
            mCompIndices[name] = mComps.size();
            mComps.push_back(PComp(cit));
            return mComps.back();
         }
         else
         {
            return mComps[cmit->second];
         }
      }
      
      size_t numComponents() const
      {
         return mComps.size();
      }
      
      bool hasComponent(const std::string &name) const
      {
         return (mCompIndices.find(name) != mCompIndices.end());
      }
      
      PComp& operator[](const std::string &name)
      {
         IndexMap::iterator it = mCompIndices.find(name);
         if (it == mCompIndices.end())
         {
            throw std::runtime_error("Invalid component name");
         }
         return mComps[it->second];
      }
      
      PComp& operator[](size_t idx)
      {
         return mComps[idx];
      }
      
      const PComp& operator[](const std::string &name) const
      {
         IndexMap::const_iterator it = mCompIndices.find(name);
         if (it == mCompIndices.end())
         {
            throw std::runtime_error("Invalid component name");
         }
         return mComps[it->second];
      }
      
      const PComp& operator[](size_t idx) const
      {
         return mComps[idx];
      }
      
   private:
      mutable ObjectInfo *mIt;
      mutable IndexMap mCompIndices;
      mutable PCompDQ mComps;
   };
   
   typedef std::deque<PObj> PObjDQ;
   
private:
   
   IndexMap mObjIndices;
   PObjDQ mObjs;
   ParticlesDataMutable *mParticles;
   
public:
   
   PartioGtoReader(ParticlesDataMutable *particles, bool header=false)
      : Gto::Reader(header ? Gto::Reader::HeaderOnly : Gto::Reader::RandomAccess)
      , mParticles(particles)
   {
   }
   
   virtual ~PartioGtoReader()
   {
      clear();
   }
   
   // called when all header data read
   
   virtual void descriptionComplete()
   {
      Objects &objs = objects();
      Components &comps = components();
      Properties &props = properties();
      
      if (objs.size() == 0 || comps.size() == 0 || props.size() == 0)
      {
         return;
      }
      
      PropertyInfo *phead = &props[0];
      Properties::iterator pit;
      
      std::string oname;
      std::string cname;
      std::string pname;
      
      pit = props.begin();
      while (pit != props.end())
      {
         if (stringFromId(pit->component->object->protocolName) == GTO_PROTOCOL_PARTICLE &&
             pit->component->object->protocolVersion == 1)
         {
            pname = stringFromId(pit->name);
            cname = stringFromId(pit->component->name);
            oname = stringFromId(pit->component->object->name);
            
            PObj &obj = addObject(oname, (ObjectInfo*) pit->component->object);
            PComp &comp = obj.addComponent(cname, (ComponentInfo*) pit->component);
            comp.addProperty(pname, phead + (pit - props.begin()));
         }
         
         ++pit;
      }
   }
   
   // called when mode is not RandomAccess (called for HeaderOnly then)
   // or during accessObject, accessComponent, accessProperty
   // may be called before descriptionComplete !
   // as we either read in RandomAccess or HeaderOnly, it doesn't hurt to accept all
   
   virtual Gto::Reader::Request object(const std::string& name,
                                       const std::string& protocol,
                                       unsigned int protocolVersion,
                                       const Gto::Reader::ObjectInfo &header)
   {
      return Gto::Reader::Request(protocol == GTO_PROTOCOL_PARTICLE && protocolVersion == 1);
   }


   virtual Gto::Reader::Request component(const std::string& name,
                                          const std::string& interp,
                                          const Gto::Reader::ComponentInfo &header)
   {
      return Gto::Reader::Request(true);
   }


   virtual Gto::Reader::Request property(const std::string& name,
                                         const std::string& interp,
                                         const Gto::Reader::PropertyInfo &header)
   {
      return Gto::Reader::Request(true);
   }
   
   // called in readProperty
   
   virtual void* data(const Gto::Reader::PropertyInfo& header, size_t bytes)
   {
      std::string name = stringFromId(header.name);
      ParticleAttribute attr;
      if (mParticles->attributeInfo(name.c_str(), attr))
      {
         switch (header.type)
         {
         case Gto::Float:
            return mParticles->dataWrite<float>(attr, 0);
         case Gto::Int:
         case Gto::String:
            return mParticles->dataWrite<int>(attr, 0);
         default:
            return NULL;
         }
      }
      return NULL;
   }
   
   virtual void dataRead(const Gto::Reader::PropertyInfo& header)
   {
      if (header.type == Gto::String)
      {
         std::string name = stringFromId(header.name);
         ParticleAttribute attr;
         if (mParticles->attributeInfo(name.c_str(), attr))
         {
            int *si = mParticles->dataWrite<int>(attr, 0);
            for (size_t i=0, k=0; i<header.size; ++i)
            {
               for (size_t j=0; j<header.width; ++j, ++k)
               {
                  std::string s = stringFromId(si[k]);
                  si[k] = mParticles->registerIndexedStr(attr, s.c_str());
               }
            }
         }
      }
   }
   
   // override open to clear objects
   
   virtual bool open(void const *pData, size_t dataSize, const char *name)
   {
      clear();
      return Gto::Reader::open(pData, dataSize, name);
   }
   
   virtual bool open(const char *filename)
   {
      clear();
      return Gto::Reader::open(filename);
   }
   
   virtual bool open(std::istream &is, const char *name, unsigned int ormode = 0)
   {
      clear();
      return Gto::Reader::open(is, name, ormode);
   }
   
   // ---
   
   PObj& addObject(const std::string &name, ObjectInfo *oit)
   {
      IndexMap::iterator omit = mObjIndices.find(name);
      if (omit == mObjIndices.end())
      {
         mObjIndices[name] = mObjs.size();
         mObjs.push_back(PObj(oit));
         return mObjs.back();
      }
      else
      {
         return mObjs[omit->second];
      }
   }
   
   size_t numObjects() const
   {
      return mObjs.size();
   }
   
   bool hasObject(const std::string &name) const
   {
      return (mObjIndices.find(name) != mObjIndices.end());
   }
   
   PObj& operator[](size_t idx)
   {
      return mObjs[idx];
   }
   
   PObj& operator[](const std::string &name)
   {
      IndexMap::iterator it = mObjIndices.find(name);
      if (it == mObjIndices.end())
      {
         throw std::runtime_error("Invalid object name");
      }
      return mObjs[it->second];
   }
   
   const PObj& operator[](size_t idx) const
   {
      return mObjs[idx];
   }
   
   const PObj& operator[](const std::string &name) const
   {
      IndexMap::const_iterator it = mObjIndices.find(name);
      if (it == mObjIndices.end())
      {
         throw std::runtime_error("Invalid object name");
      }
      return mObjs[it->second];
   }
      
   void clear()
   {
      mObjIndices.clear();
      mObjs.clear();
   }
   
protected: 
};

ParticlesDataMutable* readGTO(const char *filename, const bool headersOnly)
{
   ParticlesDataMutable *particles = (headersOnly ? new ParticleHeaders() : create());
   
   PartioGtoReader gr(particles, headersOnly);
   
   if (!gr.open(filename))
   {
      std::cerr << "Partio: Unable to open file \"" << filename << "\"" << std::endl;
      particles->release();
      return NULL;
   }
   
   if (gr.numObjects() == 0)
   {
      std::cerr << "Partio: No particles object in GTO file \"" << filename << "\"" << std::endl;
      particles->release();
      return NULL;
   }
   else if (gr.numObjects() != 1)
   {
      std::cerr << "Partio: More than one particles object in GTO file \"" << filename << "\"" << std::endl;
   }
   
   unsigned int np = 0;
   
   for (size_t oi=0; oi<gr.numObjects(); ++oi)
   {
      PartioGtoReader::PObj &obj = gr[oi];
      if (!obj.hasComponent("points"))
      {
         continue;
      }
      
      PartioGtoReader::PComp &ptcomp = obj["points"];
      if (!ptcomp.hasProperty("position"))
      {
         continue;
      }
      
      PartioGtoReader::PProp &pos = ptcomp["position"];
      if (pos.type() != Gto::Float || pos.width() != 3)
      {
         continue;
      }
      
      np = pos.size();
      
      particles->addParticles(np);
      
      for (size_t pi=0; pi<ptcomp.numProperties(); ++pi)
      {
         PartioGtoReader::PProp &prop = ptcomp[pi];
         
         ParticleAttributeType piot = NONE;
         
         switch (prop.type())
         {
         case Gto::Float:
            piot = FLOAT;
            if (prop.width() == 3)
            {
               std::string interp = gr.stringFromId(prop.interpretation());
               if (interp == "point" || interp == "vector")
               {
                  piot = VECTOR;
               }
            }
            break;
         case Gto::Int:
            piot = INT;
            break;
         case Gto::String:
            piot = INDEXEDSTR;
         default:
            break;
         }
         
         if (piot != NONE)
         {
            std::string name = gr.stringFromId(prop.name());
            
            ParticleAttribute hattr = particles->addAttribute(name.c_str(), piot, prop.width());
            if (!headersOnly)
            {
               if (!gr.accessProperty(prop))
               {
                  std::cerr << "Partio: Failed to read \"" << name << "\" property data" << std::endl;
                  particles->release();
                  return NULL;
               }
            }
         }
      }
      
      break;
   }
   
   if (np == 0)
   {
      std::cerr << "Partio: No valid particles object in GTO file \"" << filename << "\"" << std::endl;
      particles->release();
      return NULL;
   }
   
   return particles;
}

bool writeGTO(const char *filename, const ParticlesData &p, const bool compressed)
{
   Gto::Writer *gw = new Gto::Writer();
   
   if (!gw->open(filename, Gto::Writer::BinaryGTO)) //(compressed ? Gto::Writer::BinaryGTO : Gto::Writer::TextGTO)))
   {
      delete gw;
      std::cerr << "Partio: Unable to open file \"" << filename << "\"" << std::endl;
      return false;
   }
   
   uint64_t np = p.numParticles();
   int na = p.numAttributes();
      
   ParticleIndex *indices = new ParticleIndex[np];
   for (ParticleIndex pi=0; pi<np; ++pi)
   {
      indices[pi] = pi;
   }
   
   std::vector<int> skipAttrs;
   
   // Header
   gw->beginObject("particles", GTO_PROTOCOL_PARTICLE, 1);
   gw->beginComponent(GTO_COMPONENT_POINTS);
   
   ParticleAttribute hattr;
   if (!p.attributeInfo("position", hattr))
   {
      std::cerr <<"Partio: failed to find attr 'position' for GTO output" << std::endl;
      return false;
   }
   for (int ai=0; ai<na; ++ai)
   {
      p.attributeInfo(ai, hattr);
      
      if (hattr.type == NONE)
      {
         skipAttrs.push_back(ai);
         continue;
      }
      
      if (PartioTypeDim[hattr.type] != hattr.count)
      {
         skipAttrs.push_back(ai);
         continue;
      }
      
      if (hattr.name == "particleId")
      {
         gw->property("id", PartioToGtoType[hattr.type], np, hattr.count);
      }
      else
      {
         if (hattr.type == VECTOR)
         {
            std::string interp = "vector";
            // lower case attribute name
            std::string lcname = hattr.name;
            for (size_t ci=0; ci<lcname.length(); ++ci)
            {
               if (lcname[ci] >= 'A' && lcname[ci] <= 'Z')
               {
                  lcname[ci] = 'a' + (lcname[ci] - 'A');
               }
            }
            // lookup for "position" string
            if (lcname.find("position") != std::string::npos)
            {
               // change GTO interpretation to "point"
               interp = "point";
            }
            gw->property(hattr.name.c_str(), PartioToGtoType[hattr.type], np, hattr.count, interp.c_str());
         }
         else
         {
            gw->property(hattr.name.c_str(), PartioToGtoType[hattr.type], np, hattr.count);
            // Intern strings
            if (hattr.type == INDEXEDSTR)
            {
               const std::vector<std::string>& strs = p.indexedStrs(hattr);
               for (size_t si=0; si<strs.size(); ++si)
               {
                  gw->intern(strs[si]);
               }
            }
         }
      }
   }
   gw->endComponent();
   gw->endObject();
   
   // Data
   gw->beginData();
   
   // process position, velocity and id first, then remaining
   size_t si = 0;
   int sa = (si < skipAttrs.size() ? skipAttrs[si] : std::numeric_limits<int>::max());
   
   for (int ai=0; ai<na; ++ai)
   {
      // skip attribute?
      if (ai == sa)
      {
         ++si;
         sa = (si < skipAttrs.size() ? skipAttrs[si] : std::numeric_limits<int>::max());
         continue;
      }
      
      p.attributeInfo(ai, hattr);
      
      switch (hattr.type)
      {
      case VECTOR:
      case FLOAT:
         {
            float *dat = new float[np * hattr.count];
            p.data<float>(hattr, int(np), indices, true, dat);
            gw->propertyData(dat);
            delete[] dat;
         }
         break;
      case INT:
         {
            int *dat = new int[np * hattr.count];
            p.data<int>(hattr, int(np), indices, true, dat);
            gw->propertyData(dat);
            delete[] dat;
         }
         break;
      case INDEXEDSTR:
         {
            const std::vector<std::string>& strs = p.indexedStrs(hattr);
            int *dat = new int[np * hattr.count];
            // Here, need to convert string indices so loop per-particle
            int *curi = dat;
            for (ParticleIndex pi=0; pi<np; ++pi)
            {
               const int *srci = p.data<int>(hattr, pi);
               for (int d=0; d<hattr.count; ++d, ++curi, ++srci)
               {
                  *curi = gw->lookup(strs[*srci]);
               }
            }
            gw->propertyData(dat);
            delete[] dat;
         }
         break;
      default:
         break;
      }
      
   }
   
   gw->endData();
   
   // Done
   gw->close();
   delete gw;
      
   return true;
}

}
