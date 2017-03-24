/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Import
Copyright 2012 (c)  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

#include "partioImport.h"
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MDGModifier.h>
#include <maya/MAnimControl.h>

static const char *kAttributeFlagS	= "-atr";
static const char *kAttributeFlagL  = "-attribute";
static const char *kFlipFlagS		= "-flp";
static const char *kFlipFlagL		= "-flip";
static const char *kHelpFlagS 		= "-h";
static const char *kHelpFlagL 		= "-help";
static const char *kParticleL		= "-particle";
static const char *kParticleS		= "-p";
static const char *kAllAttribFlagS  = "-aa";
static const char *kAllAttribFlagL  = "-allAttributes";
static const char *kListAttribS     = "-la";
static const char *kListAttribL     = "-listAttributes";
static const char *kAttribTypeS     = "-at";
static const char *kAttribTypeL     = "-attributeType";
static const char *kExclAttrFlagS   = "-ea";
static const char *kExclAttrFlagL   = "-excludeAttribute";
static const char *kDontEmitS       = "-de";
static const char *kDontEmitL       = "-dontEmit";

using namespace std;
using namespace Partio;

bool PartioImport::hasSyntax()
{
    return true;
}

MSyntax PartioImport::createSyntax()
{

    MSyntax syntax;

    syntax.addFlag(kParticleS, kParticleL ,  MSyntax::kString);
    syntax.addFlag(kHelpFlagS, kHelpFlagL ,  MSyntax::kNoArg);
    syntax.addFlag(kAllAttribFlagS, kAllAttribFlagL, MSyntax::kNoArg);
    syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse( kAttributeFlagS );
    syntax.addFlag(kExclAttrFlagS, kExclAttrFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse( kExclAttrFlagS );
    syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
    syntax.addFlag(kListAttribS, kListAttribL, MSyntax::kNoArg);
    syntax.addFlag(kAttribTypeS, kAttribTypeL, MSyntax::kString);
    syntax.makeFlagMultiUse( kAttribTypeS );
    syntax.makeFlagQueryWithFullArgs( kAttribTypeS, true );
    syntax.addFlag(kDontEmitS, kDontEmitL, MSyntax::kNoArg);
    syntax.setObjectType(MSyntax::kStringObjects, 1, 1);
    syntax.useSelectionAsDefault(false);

    syntax.enableQuery(true);
    syntax.enableEdit(false);


    return syntax;
}



void* PartioImport::creator()
{
    return new PartioImport;
}

MStatus PartioImport::doIt(const MArgList& Args)
{
    bool makeParticle = false;

    MStatus status;
    MArgDatabase argData(syntax(), Args, &status);

    if (status == MStatus::kFailure)
    {
        MGlobal::displayError("Error parsing arguments" );
        return MStatus::kFailure;
    }

    if (argData.isQuery())
    {
        MString cachePath;
        MStringArray rv;

        argData.getObjects(rv);
        if (rv.length() != 1)
        {
            MGlobal::displayError("No or many Particle Cache specified");
            return MStatus::kFailure;
        }
        cachePath = rv[0];

        Partio::ParticlesInfo *info = Partio::readHeaders(cachePath.asChar());
        if (!info)
        {
            MGlobal::displayError("Could not read particles header");
            return MStatus::kFailure;
        }

        rv.clear();

        Partio::ParticleAttribute attr;

        if (argData.isFlagSet(kListAttribL))
        {
            if (argData.numberOfFlagUses(kAttribTypeS) > 0)
            {
                MGlobal::displayError("-at/-attributeType and -la/-listAttributes are mutually exclusive in query mode");
                return MStatus::kFailure;
            }
            for (int i=0; i<info->numParticles(); ++i)
            {
                if (info->attributeInfo(i, attr))
                {
                    rv.append(attr.name.c_str());
                }
            }
        }
        else
        {
            unsigned int n = argData.numberOfFlagUses(kAttribTypeS);

            if (n > 0)
            {
                MString attrib;

                for (unsigned int i=0; i<n; ++i)
                {
                    MArgList argList;

                    argData.getFlagArgumentList(kAttribTypeS, i, argList);

                    attrib = argList.asString(0);

                    if (!info->attributeInfo(attrib.asChar(), attr))
                    {
                        MGlobal::displayError("No attribute \"" + attrib + "\" in cache");
                        return MStatus::kFailure;
                    }

                    switch (attr.type)
                    {
                    case Partio::FLOAT:
                        rv.append("FLOAT");
                        break;
                    case Partio::VECTOR:
                        rv.append("VECTOR");
                        break;
                    case Partio::INT:
                        rv.append("INT");
                        break;
                    case Partio::INDEXEDSTR:
                        rv.append("INDEXEDSTR");
                        break;
                    default:
                        rv.append("NONE");
                        break;
                    }
                }
            }
            else
            {
                MGlobal::displayWarning("Nothing to query");
            }
        }

        setResult(rv);
        return MStatus::kSuccess;
    }

    if ( argData.isFlagSet(kHelpFlagL) )
    {
        printUsage();
        return MStatus::kFailure;
    }

    MString particleShape;
    if (argData.isFlagSet(kParticleL) )
    {
        argData.getFlagArgument(kParticleL, 0, particleShape );
        if (particleShape == "")
        {
            printUsage();
            MGlobal::displayError("Please supply particleShape argument" );
            return MStatus::kFailure;
        }
    }
    else
    {
        makeParticle = true;
    }

    if ( argData.isFlagSet(kFlipFlagL) || argData.isFlagSet(kFlipFlagS))
    {
    }

    /// create all attributes?
    bool allAttribs = (argData.isFlagSet(kAllAttribFlagS) || argData.isFlagSet(kAllAttribFlagL));

    bool dontEmit = argData.isFlagSet(kDontEmitL);

    /// parse attribute  flags
    unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );

    /// loop thru the rest of the attributes given
    MStringArray attrNames;
    MStringArray exclNames;
    MStringArray mayaAttrNames;
    MString idAttrName = "";
    MString positionAttrName = "";
    MString velocityAttrName = "";
    bool worldPosition = false;
    bool worldVelocity = false;

    if (allAttribs)
    {
        MString exclName;
        unsigned int numExcl = argData.numberOfFlagUses( kExclAttrFlagL );
        for (unsigned int i=0; i<numExcl; ++i)
        {
            argData.getFlagArgument( kExclAttrFlagL, 0, exclName );
            exclNames.append(exclName);
        }
    }

    for (unsigned int i = 0; i < numUses; i++)
    {
        MArgList argList;
        
        status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
        if (!status)
        {
            return status;
        }

        MString attrName = argList.asString( 0, &status );
        if (!status)
        {
            return status;
        }

        MString mayaAttrName = attrName;
        if (argList.length() > 1)
        {
            mayaAttrName = argList.asString(1);
        }
        
        if (mayaAttrName == "position")
        {
            if (positionAttrName == "" || worldPosition)
            {
                // position attribute not found or is in world space (give precedence to object space)
                positionAttrName = attrName;
                worldPosition = false;
            }
        }
        else if (mayaAttrName == "velocity")
        {
            // Give precedence to local space velocity when not emitting the particles
            if (velocityAttrName == "" || (dontEmit && worldVelocity))
            {
                // velocity attribute not found or is in world space (give precedence to world space [set not below on MFnParticleSystem.emit])
                velocityAttrName = attrName;
                worldVelocity = false;
            }
        }
        else if (mayaAttrName == "worldPosition")
        {
            if (positionAttrName == "")
            {
                positionAttrName = attrName;
                worldPosition = true;
            }
        }
        else if (mayaAttrName == "worldVelocity")
        {
            if (velocityAttrName == "" || (!dontEmit && !worldVelocity))
            {
                velocityAttrName = attrName;
                worldVelocity = true;
            }
        }
        else if (mayaAttrName == "particleId" || mayaAttrName == "id")
        {
            if (idAttrName == "")
            {
                idAttrName = attrName;
            }
        }
        else
        {
            attrNames.append(attrName);
            mayaAttrNames.append(mayaAttrName);
        }
    }

    MStringArray objects;
    argData.getObjects(objects);
    if (objects.length() != 1)
    {
        MGlobal::displayError("No or many Particle Cache specified");
        return MStatus::kFailure;
    }

    MString particleCache = objects[0];
    if (!partio4Maya::cacheExists(particleCache.asChar()))
    {
        MGlobal::displayError("Particle Cache Does not exist");
        return MStatus::kFailure;
    }

    if (makeParticle)
    {
        MStringArray foo;
        MGlobal::executeCommand("nParticle -n partioImport", foo);
        particleShape = foo[1];
    }

    MSelectionList list;
    list.add(particleShape);
    MDagPath objPath;
    list.getDagPath(0, objPath);

    if (objPath.apiType() != MFn::kParticle && objPath.apiType() != MFn::kNParticle)
    {
        MGlobal::displayError("PartioImport-> can't find your PARTICLESHAPE.");
        return MStatus::kFailure;
    }

    MStatus stat;
    MFnParticleSystem partSys(objPath, &stat);

    if (stat == MStatus::kSuccess) // particle object was found and attached to
    {
        Partio::ParticlesDataMutable* particles;
        Partio::ParticleAttribute idAttr;
        Partio::ParticleAttribute positionAttr;
        Partio::ParticleAttribute velocityAttr;
        
        MString partName = objPath.partialPathName();
        
        if (!makeParticle)
        {
            MVectorArray empty;
            
            // Reset particle count and for empty position and velocity
            partSys.setCount(0);
            partSys.setPerParticleAttribute("position", empty);
            partSys.setPerParticleAttribute("velocity", empty);
        }
        else
        {
            MPlug plug;

            // Disable dynamics on newly created particle shapes
            plug = partSys.findPlug("isDynamic");
            if (!plug.isNull())
            {
                plug.setBool(false);
            }

            // Set new particles render type to 'points' (easier on the viewport)
            plug = partSys.findPlug("particleRenderType");
            if (!plug.isNull())
            {
                plug.setInt(3);
            }

            // Set new particles shading color to constant
            plug = partSys.findPlug("colorInput");
            if (!plug.isNull())
            {
                plug.setInt(0);
            }
        }
        
        MGlobal::displayInfo(MString ("PartioImport-> LOADING: ") + particleCache);
        particles = read(particleCache.asChar());

        if (!particles || particles->numParticles() <=0)
        {
            MGlobal::displayError("Particle Cache cannot be read, or does not Contain any particles");
            return MStatus::kFailure;
        }

        char partCount[50];
        sprintf (partCount, "%d", particles->numParticles());
        MGlobal::displayInfo(MString ("PartioImport-> LOADED: ") + partCount + MString (" particles"));

        // Check for particle positions attribute
        bool validPosition = false;
        if (positionAttrName != "")
        {
            validPosition = particles->attributeInfo(positionAttrName.asChar(), positionAttr);
        }
        else
        {
            // Local space position search first
            positionAttrName = "position";
            validPosition = particles->attributeInfo(positionAttrName.asChar(), positionAttr);
            if (!validPosition)
            {
                positionAttrName = "Position";
                validPosition = particles->attributeInfo(positionAttrName.asChar(), positionAttr);
            }
            if (!validPosition)
            {
                positionAttrName = "worldPosition";
                validPosition = particles->attributeInfo(positionAttrName.asChar(), positionAttr);
                worldPosition = true;
            }
        }
        if (!validPosition)
        {
            MGlobal::displayError("PartioImport-> Failed to find position attribute ");
            return ( MS::kFailure );
        }

        // Check for particle ids attribute
        bool validId = false;
        int nextId = -1;
        if (idAttrName != "")
        {
            validId = particles->attributeInfo(idAttrName.asChar(), idAttr);
        }
        else if (allAttribs)
        {
            idAttrName = "id";
            validId = particles->attributeInfo(idAttrName.asChar(), idAttr);
            if (!validId)
            {
                idAttrName = "ID";
                validId = particles->attributeInfo(idAttrName.asChar(), idAttr);
            }
            if (!validId)
            {
                idAttrName = "particleId";
                validId = particles->attributeInfo(idAttrName.asChar(), idAttr);
            }
            if (!validId)
            {
                idAttrName = "ParticleId";
                validId = particles->attributeInfo(idAttrName.asChar(), idAttr);
            }
            if (validId)
            {
                // Check if attribute is excluded
                for (unsigned int i=0; i<exclNames.length(); ++i)
                {
                    if (idAttrName == exclNames[i])
                    {
                        idAttrName = "";
                        break;
                    }
                }
            }
        }
        if (!validId)
        {
            idAttrName = "";
            MGlobal::displayWarning("PartioImport-> Failed to find Id attribute ");
        }

        // Check for particle velocity attribute
        bool validVelocity = false;
        if (velocityAttrName != "")
        {
            validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
        }
        else if (allAttribs)
        {
            // Check for predefined velocity attribute names only if all attributes are to be read
            // when emitting, give precedence to worldVelocity first
            if (!dontEmit)
            {
                worldVelocity = true;
                velocityAttrName = "worldVelocity";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                worldVelocity = false;
                velocityAttrName = "velocity";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                velocityAttrName = "Velocity";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                velocityAttrName = "vel";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                velocityAttrName = "Vel";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                velocityAttrName = "v";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity)
            {
                velocityAttrName = "V";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
            }
            if (!validVelocity && dontEmit)
            {
                // When not emitting, look for world velocities last
                velocityAttrName = "worldVelocity";
                validVelocity = particles->attributeInfo(velocityAttrName.asChar(), velocityAttr);
                worldVelocity = true;
            }
            if (validVelocity)
            {
                // Check if attribute is excluded
                for (unsigned int i=0; i<exclNames.length(); ++i)
                {
                    if (velocityAttrName == exclNames[i])
                    {
                        velocityAttrName = "";
                        validVelocity = false;
                        break;
                    }
                }
            }
        }
        if (!validVelocity)
        {
            velocityAttrName = "";
            MGlobal::displayWarning("PartioImport-> Failed to find Velocity attribute ");
        }

        // Now add all remaining attributes if required
        if (allAttribs)
        {
            Partio::ParticleAttribute pioAttr;
            for (int ai=0; ai<particles->numAttributes(); ++ai)
            {
                particles->attributeInfo(ai, pioAttr);
                // Here, we use same attribute name for maya
                MString attrName = pioAttr.name.c_str();
                // Skip excluded attribute
                bool exclude = false;
                for (unsigned int aj=0; aj<exclNames.length(); ++aj)
                {
                    if (attrName == exclNames[aj])
                    {
                        exclude = true;
                        break;
                    }
                }
                if (exclude)
                {
                    continue;
                }
                // First check if attribute is not already bound
                bool found = false;
                for (unsigned int aj=0; aj<mayaAttrNames.length(); ++aj)
                {
                    if (mayaAttrNames[aj] == attrName)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    // At this point, we have already looked up position and velocity synonyms
                    if (attrName == positionAttrName ||
                        attrName == velocityAttrName ||
                        attrName == idAttrName ||
                        attrName == "position" ||
                        attrName == "velocity" ||
                        attrName == "worldPosition" ||
                        attrName == "worldVelocity" ||
                        attrName == "particleId" ||
                        attrName == "id")
                    {
                        continue;
                    }
                    else
                    {
                        attrNames.append(attrName);
                        mayaAttrNames.append(attrName);
                    }
                }
            }
        }
        
        MDoubleArray ids;
        MPointArray positions;
        MVectorArray velocities;
        std::map<std::string, MVectorArray> vectorAttrArrays;
        std::map<std::string, MDoubleArray> doubleAttrArrays;
        // We use this mapping to allow for direct writing of attrs to PP variables
        std::map<std::string, std::string> userPPMapping;

        for (unsigned int i=0;i<attrNames.length();i++)
        {
            Partio::ParticleAttribute testAttr;
            if (particles->attributeInfo(attrNames[i].asChar(), testAttr))
            {
                if (testAttr.count == 3)
                {
                    if (!partSys.isPerParticleVectorAttribute(mayaAttrNames[i]))
                    {
                        MPlug plug = partSys.findPlug(mayaAttrNames[i]);
                        if (plug.isNull())
                        {
                            MGlobal::displayInfo(MString("PartioImport-> Adding ppAttr " + mayaAttrNames[i]) );
                            MFnTypedAttribute tattr;
                            
                            MString initAttr = mayaAttrNames[i] + "0";
                            
                            MObject obj0 = tattr.create(initAttr, initAttr, MFnData::kVectorArray);
                            partSys.addAttribute(obj0);
                            
                            MObject obj = tattr.create(mayaAttrNames[i], mayaAttrNames[i], MFnData::kVectorArray);
                            tattr.setKeyable(true);
                            partSys.addAttribute(obj);
                        }
                    }
                    MVectorArray vAttribute;
                    vAttribute.setLength(particles->numParticles());
                    vectorAttrArrays[attrNames[i].asChar()] = vAttribute;
                    userPPMapping[attrNames[i].asChar()] = mayaAttrNames[i].asChar();
                }
                else if (testAttr.count == 1)
                {
                    if (!partSys.isPerParticleDoubleAttribute(mayaAttrNames[i]))
                    {
                        MPlug plug = partSys.findPlug(mayaAttrNames[i]);
                        if (plug.isNull())
                        {
                            MGlobal::displayInfo(MString("PartioImport-> Adding ppAttr " + mayaAttrNames[i]));
                            MFnTypedAttribute tattr;
                            
                            MString initAttr = mayaAttrNames[i] + "0";
                            
                            MObject obj0 = tattr.create(initAttr, initAttr, MFnData::kDoubleArray);
                            partSys.addAttribute(obj0);
                            
                            MObject obj = tattr.create(mayaAttrNames[i], mayaAttrNames[i], MFnData::kDoubleArray);
                            tattr.setKeyable(true);
                            partSys.addAttribute(obj);
                        }
                    }
                    MDoubleArray dAttribute;
                    dAttribute.setLength(particles->numParticles());
                    doubleAttrArrays[attrNames[i].asChar()] = dAttribute;
                    userPPMapping[attrNames[i].asChar()] = mayaAttrNames[i].asChar();
                }
                else
                {
                    MGlobal::displayWarning(MString("PartioImport-> Skipping attr: " + MString(attrNames[i])) + " (unsupported dimension)");
                }
            }
        }

        /// Final particle loop
        std::map <std::string, MVectorArray >::iterator vecIt;
        std::map <std::string, MDoubleArray >::iterator doubleIt;

        MMatrix iwm, wm;
        wm = objPath.inclusiveMatrix();
        iwm = objPath.inclusiveMatrixInverse();

        // Note 1: If we emit, velocities MUST be in worldspace
        // Note 2: If particle node was just created (makeParticle true), world space == object space

        if (makeParticle)
        {
            // Setup worldPosition and worldVelocity flags to avoid matrix multiplications
            worldPosition = false;
            worldVelocity = !dontEmit;
        }

        bool ignoreVelocity = true;

        for (int i=0;i<particles->numParticles();i++)
        {
            const float * partioPositions = particles->data<float>(positionAttr,i);
            MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);
            positions.append(worldPosition ? pos*iwm : pos);

            int cid = int(i);
            if (validId)
            {
                const int *partioIds = particles->data<int>(idAttr,i);
                cid = partioIds[0];
            }
            ids.append(cid);
            if (cid + 1 > nextId)
            {
                nextId = cid + 1;
            }

            if (validVelocity)
            {
                const float * partioVelocities = particles->data<float>(velocityAttr,i);
                MVector vel(partioVelocities[0], partioVelocities[1], partioVelocities[2]);
                if (!dontEmit && (vel.x * vel.x + vel.y * vel.y + vel.z * vel.z) > 0.000001f)
                {
                    // if velocity length below 0.001, consider insignificant
                    ignoreVelocity = false;
                }
                velocities.append(dontEmit ? (worldVelocity ? vel*iwm : vel) : (worldVelocity ? vel : vel*wm));
            }

            if (vectorAttrArrays.size() > 0)
            {
                for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
                {
                    ParticleAttribute vectorAttr;
                    particles->attributeInfo(vecIt->first.c_str(), vectorAttr);
                    const float* vecVal = particles->data<float>(vectorAttr, i);
                    vectorAttrArrays[vecIt->first][i] = MVector(vecVal[0],vecVal[1],vecVal[2]);
                }
            }

            if (doubleAttrArrays.size() > 0)
            {
                for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
                {
                    ParticleAttribute doubleAttr;
                    particles->attributeInfo(doubleIt->first.c_str(),doubleAttr);
                    const float*  doubleVal = particles->data<float>(doubleAttr, i);
                    doubleAttrArrays[doubleIt->first][i] = doubleVal[0];
                }
            }
        }

        if (particles)
        {
            particles->release();
        }

        if (dontEmit)
        {
            unsigned int npt = positions.length();

            partSys.setCount(npt);

            MVector zv(0.0, 0.0, 0.0);
            MVectorArray va(npt, zv);
            MDoubleArray da(npt, 0.0);

            for (unsigned int i=0; i<npt; ++i) va[i] = MVector(positions[i]);
            partSys.setPerParticleAttribute("position", va);

            partSys.setPerParticleAttribute("velocity", velocities);

            for (unsigned int i=0; i<npt; ++i) va[i] = zv;
            partSys.setPerParticleAttribute("acceleration", va);

            double t = MAnimControl::currentTime().as(MTime::kSeconds);
            for (unsigned int i=0; i<npt; ++i) da[i] = t;
            partSys.setPerParticleAttribute("birthTime", da);

            for (unsigned int i=0; i<npt; ++i) da[i] = 0.0;
            partSys.setPerParticleAttribute("age", da);

            for (unsigned int i=0; i<npt; ++i) da[i] = 1.0;
            partSys.setPerParticleAttribute("mass", da);

            // 0: Live forever
            // 1: Constant ('lifespan' attribute's value)
            // 2: Random Range
            // 3: lifespanPP only
            partSys.findPlug("lifespanMode").setInt(3);
            //partSys.findPlug("lifespan").setDouble(3.4028234663852886e+38);
            for (unsigned int i=0; i<npt; ++i) da[i] = 3.4028234663852886e+38;
            partSys.setPerParticleAttribute("lifespanPP", da); // or should it be finalLifespanPP
        }
        else
        {
            // Seems to MFnParticleSystem expects velocities in world space and positions in local space
            // Don't ask me why but this is the only way to get matching object-space velocities
            // ... AND THIS IS DEAD SLOW WITH A MILLION PARTICLES!

            if (validVelocity && ignoreVelocity)
            {
                MGlobal::displayInfo("Ignoring particle velocities");
                validVelocity = false;
            }

            if (validVelocity)
            {
                stat = partSys.emit(positions, velocities);
            }
            else
            {
                stat = partSys.emit(positions);
            }
        }

        partSys.setPerParticleAttribute("id", ids);
        partSys.findPlug("nid").setInt(nextId);

        for (doubleIt = doubleAttrArrays.begin(); doubleIt != doubleAttrArrays.end(); doubleIt++)
        {
            partSys.setPerParticleAttribute(MString(userPPMapping[doubleIt->first].c_str()), doubleAttrArrays[doubleIt->first]);
        }
        for (vecIt = vectorAttrArrays.begin(); vecIt != vectorAttrArrays.end(); vecIt++)
        {
            partSys.setPerParticleAttribute(MString(userPPMapping[vecIt->first].c_str()), vectorAttrArrays[vecIt->first]);
        }

        partSys.saveInitialState();
    }
    else
    {
        if (makeParticle)
        {
            MObject obj = objPath.node();
            MDGModifier dgmod;
            dgmod.deleteNode(obj);
            dgmod.doIt();
        }
        return stat;
    }

    if (makeParticle)
    {
        setResult(particleShape);
    }

    return MStatus::kSuccess;
}

void PartioImport::printUsage()
{

    MString usage = "\n-----------------------------------------------------------------------------\n";
    usage += "\tpartioImport -p/particle <particleShapeName> [Options] </full/path/to/particleCacheFile> \n";
    usage += "\n";
    usage += "\t[Options]\n";
    usage += "\t\t-p/particle <particleShapeName> (if none defined, one will be created)";
    usage += "\t\t-atr/attribute (multi use) <cache Attr Name>  <PP attribute name>\n";
    usage += "\t\t     (position/id) are always imported \n";
    //usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
    usage += "\t\t-aa/allAttributes (read all attributes in cache)\n";
    usage += "\t\t-ea/excludeAttribute <cache Attr Name> (use with -aa, exclude a specific attribute from cache)\n";
    usage += "\t\t-la/listAttributes <cache Attr Name (query mode only)\n";
    usage += "\t\t-at/attributeType <cache Attr Name (query mode only)\n";
    usage += "\t\t-de/dontEmit (don't emit particles but directly set their position and optinally velocity instead)\n";
    usage += "\t\t-h/help\n";
    usage += "\n";
    usage += "\tExample:\n";
    usage += "\n";
    usage += "partioImport -p particleShape1 -atr \"pointColor\" \"rgbPP\" -atr \"opacity\" \"opacityPP\"  \"/file/path/to/fooBar.0001.prt\"  \n\n";

    MGlobal::displayInfo(usage);

}








