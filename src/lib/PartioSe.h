/**
PARTIO SOFTWARE
Copyright 202 Disney Enterprises, Inc. All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
Studios" or the names of its contributors may NOT be used to
endorse or promote products derived from this software without
specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#ifdef PARTIO_USE_SEEXPR

#include <Partio.h>
#include <SeExpr2/Expression.h>
#include <SeExpr2/VarBlock.h>
#include <SeExpr2/Vec.h>
#include <map>

namespace Partio
{

template <class T>
class AttribVar : public SeExpr2::ExprVarRef
{
protected:
    Partio::ParticlesDataMutable *parts;
    int dim;
    const std::vector<std::string> *indexedStrs;

public:
    AttribVar(Partio::ParticlesDataMutable *parts,
              ParticleAttributeType attrType,
              int attrDim,
              bool fixed)
        : SeExpr2::ExprVarRef(attrType == Partio::INDEXEDSTR ?
                              SeExpr2::ExprType().String().Varying() :
                              SeExpr2::TypeVec(attrDim <= 3 ? attrDim : 3))
        , parts(parts)
        , dim(0)
        , indexedStrs(0)
    {
        dim = type().dim();
    }
};

template <class T>
class PerParticleAttribVar : public AttribVar<T>
{
private:
    Partio::ParticleAttribute attr;
    int &currentIndex;

public:
    PerParticleAttribVar(Partio::ParticlesDataMutable *parts,
                         Partio::ParticleAttribute attr,
                         int &currentIndex)
        : AttribVar<T>(parts, attr.type, attr.count, false)
        , attr(attr)
        , currentIndex(currentIndex)
    {
        if (attr.type == Partio::INDEXEDSTR)
        {
            this->indexedStrs = &(parts->indexedStrs(attr));
        }
    }

    void eval(double *result)
    {
        assert(attr.type != Partio::INDEXEDSTR);
        const T* ptr = this->parts->template data<T>(attr, currentIndex);
        for (int k=0; k<this->dim; ++k)
        {
            result[k] = double(ptr[k]);
        }
    }

    void eval(const char **resultStr)
    {
        assert(this->indexedStrs != 0);
        const T* ptr = this->parts->template data<T>(attr, currentIndex);
        resultStr[0] = (*(this->indexedStrs))[ptr[0]].c_str();
    }

private:
    PerParticleAttribVar();
    PerParticleAttribVar(const PerParticleAttribVar&);
    PerParticleAttribVar& operator=(const PerParticleAttribVar&);
};

template <class T>
class FixedAttribVar : public AttribVar<T>
{
private:
    Partio::FixedAttribute attr;

public:
    FixedAttribVar(Partio::ParticlesDataMutable *parts,
                   Partio::FixedAttribute attr)
        : AttribVar<T>(parts, attr.type, attr.count, true)
        , attr(attr)
    {
        if (attr.type == Partio::INDEXEDSTR)
        {
            this->indexedStrs = &(parts->fixedIndexedStrs(attr));
        }
    }

    void eval(double *result)
    {
        assert(attr.type != Partio::INDEXEDSTR);
        const T* ptr = this->parts->template fixedData<T>(attr);
        for (int k=0; k<this->dim; ++k)
        {
            result[k] = double(ptr[k]);
        }
    }

    void eval(const char **resultStr)
    {
        assert(this->indexedStrs != 0);
        const T* ptr = this->parts->template fixedData<T>(attr);
        resultStr[0] = (*(this->indexedStrs))[ptr[0]].c_str();
    }
};

typedef std::map<std::string, AttribVar<int>*> IntVarMap;
typedef std::map<std::string, AttribVar<float>*> FloatVarMap;

class SimpleVar : public SeExpr2::ExprVarRef
{
public:
    double val;

    SimpleVar()
        : SeExpr2::ExprVarRef(SeExpr2::TypeVec(1))
    {
    }

    void eval(double *result)
    {
        result[0] = val;
    }

    void eval(const char **)
    {
        assert(false);
    }
};

/// NOTE: This class is experimental and may be deleted/modified in future versions
class PartioSe : public SeExpr2::Expression
{
private:
    int currentIndex;
    Partio::ParticlesDataMutable* parts;
    IntVarMap intVars;
    FloatVarMap floatVars;
    mutable SimpleVar indexVar, countVar, timeVar;
    SeExpr2::VarBlockCreator blockCreator;
    int outputIndex;
    std::vector<double> outputData;
    std::string outputName;

public:

    PartioSe(Partio::ParticlesDataMutable *parts,
             const char *expr,
             const char *attrSuffix="",
             const char *fixedAttrPrefix="c_");
    virtual ~PartioSe();

    void setTime(float val);

    bool bindOutput(const char *partAttrName);

    bool run(int i);
    bool runRange(int istart, int iend);
    bool runAll();
    bool runRandom();

    virtual SeExpr2::ExprVarRef* resolveVar(const std::string &s) const;

private:

    PartioSe(const PartioSe&);
    PartioSe& operator=(const PartioSe&);

    SeExpr2::VarBlock createVarBlock();

    void addParticleAttributes(const char *fixedPrefix, const char *suffix);

    bool run(int i, SeExpr2::VarBlock *block);

    void writeOutput();
};

}

#endif

