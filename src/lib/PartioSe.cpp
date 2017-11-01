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

#ifdef PARTIO_SE_ENABLED

#include "PartioSe.h"
#include <cstdlib>

ENTER_PARTIO_NAMESPACE

PartioSe::PartioSe(Partio::ParticlesDataMutable *parts,
                   const char *expression,
                   const char *attrSuffix,
                   const char *fixedAttrPrefix)
    : SeExpr2::Expression(expression)
    , currentIndex(0)
    , parts(parts)
    , outputIndex(-1)
{
    addParticleAttributes(fixedAttrPrefix, attrSuffix);

    outputData.resize(3 * parts->numParticles());
    outputIndex = blockCreator.registerVariable("__output", SeExpr2::ExprType().FP(3).Varying());
    outputName = "";

    setVarBlockCreator(&blockCreator);

    setDesiredReturnType(SeExpr2::ExprType().FP(3).Varying());

    isValid();
}

PartioSe::~PartioSe()
{
    for (IntVarMap::iterator it=intVars.begin(), itend=intVars.end(); it!=itend; ++it)
    {
        delete it->second;
    }
    for (FloatVarMap::iterator it=floatVars.begin(), itend=floatVars.end(); it!=itend; ++it)
    {
        delete it->second;
    }
}

void PartioSe::addParticleAttributes(const char *fixedPrefix, const char *suffix)
{
    int attrn;

    attrn = parts->numAttributes();

    for (int i=0; i<attrn; i++)
    {
        Partio::ParticleAttribute attr;
        parts->attributeInfo(i, attr);
        if (attr.type == Partio::FLOAT || attr.type == Partio::VECTOR)
        {
            floatVars[attr.name + suffix] = new PerParticleAttribVar<float>(parts, attr, currentIndex);
        }
        else if (attr.type == Partio::INT || attr.type == Partio::INDEXEDSTR)
        {
            intVars[attr.name + suffix] = new PerParticleAttribVar<int>(parts, attr, currentIndex);
        }
        else
        {
            std::cerr << "Failed to map variable " << attr.name << std::endl;
        }
    }

    attrn = parts->numFixedAttributes();

    for (int i=0; i<attrn; i++)
    {
        Partio::FixedAttribute attr;
        parts->fixedAttributeInfo(i, attr);
        if (attr.type == Partio::FLOAT || attr.type == Partio::VECTOR)
        {
            floatVars[(fixedPrefix + attr.name) + suffix] = new FixedAttribVar<float>(parts, attr);
        }
        else if (attr.type == Partio::INT || attr.type == Partio::INDEXEDSTR)
        {
            intVars[(fixedPrefix + attr.name) + suffix] = new FixedAttribVar<int>(parts, attr);
        }
        else
        {
            std::cerr << "Failed to map fixed variable " << attr.name << std::endl;
        }
    }
}

SeExpr2::VarBlock PartioSe::createVarBlock()
{
    SeExpr2::VarBlock block = blockCreator.create();
    block.Pointer(outputIndex) = outputData.data();
    return block;
}

bool PartioSe::bindOutput(const char *partAttrName)
{
    Partio::ParticleAttribute hattr;

    if (parts->attributeInfo(partAttrName, hattr))
    {
        if (hattr.type == Partio::INDEXEDSTR)
        {
            return false;
        }
    }

    outputName = partAttrName;

    return true;
}

void PartioSe::writeOutput()
{
    if (outputName == "")
    {
        return;
    }

    Partio::ParticleAttribute hattr;

    if (!parts->attributeInfo(outputName.c_str(), hattr))
    {
        hattr = parts->addAttribute(outputName.c_str(), Partio::VECTOR, 3);
    }

    switch (hattr.type)
    {
    case Partio::FLOAT:
    case Partio::VECTOR:
        {
            size_t i = 0; // input index
            size_t o = 0; // output index
            int a = (hattr.count < 3 ? hattr.count : 3);

            float *out = parts->dataWrite<float>(hattr, 0);

            for (int p=0; p<parts->numParticles(); ++p, i+=3, o+=hattr.count)
            {
                for (int d=0; d<a; ++d)
                {
                    out[o+d] = float(outputData[i+d]);
                }
                for (int d=a; d<hattr.count; ++d)
                {
                    out[o+d] = 0.0f;
                }
            }
        }
        break;
    case Partio::INT:
        {
            size_t i = 0; // input index
            size_t o = 0; // output index
            int a = (hattr.count < 3 ? hattr.count : 3);

            int *out = parts->dataWrite<int>(hattr, 0);

            for (int p=0; p<parts->numParticles(); ++p, i+=3, o+=hattr.count)
            {
                for (int d=0; d<a; ++d)
                {
                    out[o+d] = int(outputData[i+d]);
                }
                for (int d=a; d<hattr.count; ++d)
                {
                    out[o+d] = 0;
                }
            }
        }
    default:
        break;
    }
}

bool PartioSe::run(int i, SeExpr2::VarBlock *block_p)
{
    if (!block_p)
    {
        return false;
    }

    currentIndex = i;

    indexVar.val = double(i);

    evalMultiple(block_p, outputIndex, i, i+1);

    return true;
}

bool PartioSe::run(int i)
{
    countVar.val = 1.0;

    if (!isValid())
    {
        std::cerr << "Not running expression because it is invalid" << std::endl;
        std::cerr << parseError() << std::endl;
        return false;
    }

    if (i < 0 || i >= parts->numParticles())
    {
        std::cerr << "Invalid index " << i << " specified. Valid range is [0," << parts->numParticles() << ")" << std::endl;
        return false;
    }

    SeExpr2::VarBlock block = createVarBlock();

    if (run(i, &block))
    {
        writeOutput();
        return true;
    }
    else
    {
        return false;
    }
}

bool PartioSe::runRange(int istart, int iend)
{
    countVar.val = double(iend - istart);

    if (!isValid())
    {
        std::cerr << "Not running expression because it is invalid" << std::endl;
        std::cerr << parseError() << std::endl;
        return false;
    }

    if (istart < 0 || iend > parts->numParticles() || iend < istart)
    {
        std::cerr << "Invalid range [" << istart << "," << iend << ") specified. Max valid range is [0," << parts->numParticles() << ")" << std::endl;
        return false;
    }

    SeExpr2::VarBlock block = createVarBlock();

    for (int i=istart; i<iend; ++i)
    {
        if (!run(i, &block))
        {
            return false;
        }
    }

    writeOutput();

    return true;
}

bool PartioSe::runRandom()
{
    countVar.val = double(parts->numParticles());

    if (!isValid())
    {
        std::cerr << "Not running expression because it is invalid" << std::endl;
        std::cerr << parseError() << std::endl;
        return false;
    }

    std::vector<int> order(parts->numParticles());

    for (size_t i=0; i<order.size(); ++i)
    {
        order[i]=i;
    }

    for (size_t i=0; i<order.size(); ++i)
    {
        size_t other = float(rand()) / RAND_MAX * order.size();
        if (other >= order.size())
        {
            other = order.size() - 1;
        }
        std::swap(order[i], order[other]);
    }

    SeExpr2::VarBlock block = createVarBlock();

    for (size_t i=0; i<order.size(); ++i)
    {
        if (!run(order[i], &block))
        {
            return false;
        }
    }

    writeOutput();

    return true;
}

bool PartioSe::runAll()
{
    return runRange(0, parts->numParticles());
}

void PartioSe::setTime(float val)
{
    timeVar.val = double(val);
}

SeExpr2::ExprVarRef* PartioSe::resolveVar(const std::string &s) const
{
    SeExpr2::ExprVarRef *rv = blockCreator.resolveVar(s);
    if (rv)
    {
        return rv;
    }

    IntVarMap::const_iterator iit = intVars.find(s);
    if (iit != intVars.end())
    {
        return iit->second;
    }

    FloatVarMap::const_iterator fit = floatVars.find(s);
    if (fit != floatVars.end())
    {
        return fit->second;
    }

    if (s == "PT")
    {
        return &indexVar;
    }
    else if (s == "NPT")
    {
        return &countVar;
    }
    else if (s == "time")
    {
        return &timeVar;
    }

    return 0;
}

EXIT_PARTIO_NAMESPACE

#endif
