/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
Copyright 2013 (c)  All rights reserved

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

#include "partioExport.h"

#include <limits>

#define  kAttributeFlagS    "-atr"
#define  kAttributeFlagL    "-attribute"
#define  kMinFrameFlagS     "-mnf"
#define  kMinFrameFlagL     "-minFrame"
#define  kMaxFrameFlagS     "-mxf"
#define  kMaxFrameFlagL     "-maxFrame"
#define  kFrameStepL        "-frameStep"
#define  kFrameStepS        "-fst"
#define  kHelpFlagS         "-h"
#define  kHelpFlagL         "-help"
#define  kPathFlagS         "-p"
#define  kPathFlagL         "-path"
#define  kFlipFlagS         "-flp"
#define  kFlipFlagL         "-flip"
#define  kFormatFlagS       "-f"
#define  kFormatFlagL       "-format"
#define  kFilePrefixFlagS   "-fp"
#define  kFilePrefixFlagL   "-filePrefix"
#define  kPerFrameFlagS     "-pf"
#define  kPerFrameFlagL     "-perFrame"
#define  kSkipDynamicsL     "-skipDynamics"
#define  kSkipDynamicsS     "-sd"
#define  kWorldSpaceL       "-worldSpace"
#define  kWorldSpaceS       "-ws"
#define  kNoXMLS            "-nx"
#define  kNoXMLL            "-noXML"



bool PartioExport::hasSyntax() const
{
    return true;
}

////// Has Syntax Similar to DynExport  to be a drop in replacement

MSyntax PartioExport::createSyntax()
{

    MSyntax syntax;

    syntax.addFlag(kHelpFlagS, kHelpFlagL, MSyntax::kNoArg);
    syntax.addFlag(kPathFlagS, kPathFlagL, MSyntax::kString);
    syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse(kAttributeFlagS);
    syntax.addFlag(kFlipFlagS, kFlipFlagL, MSyntax::kNoArg);
    syntax.addFlag(kFormatFlagS, kFormatFlagL, MSyntax::kString);
    syntax.addFlag(kMinFrameFlagS,kMinFrameFlagL, MSyntax::kDouble);
    syntax.addFlag(kMaxFrameFlagS, kMaxFrameFlagL, MSyntax::kDouble);
    syntax.addFlag(kFrameStepS, kFrameStepL, MSyntax::kDouble);
    syntax.addFlag(kFilePrefixFlagS,kFilePrefixFlagL, MSyntax::kString);
    syntax.addFlag(kPerFrameFlagS,kPerFrameFlagL, MSyntax::kString);
    syntax.addFlag(kSkipDynamicsS,kSkipDynamicsL, MSyntax::kNoArg);
    syntax.addFlag(kNoXMLS,kNoXMLL, MSyntax::kNoArg);
    syntax.addFlag(kWorldSpaceS,kWorldSpaceL, MSyntax::kNoArg);
    syntax.setObjectType(MSyntax::kStringObjects, 1, 1);
    syntax.useSelectionAsDefault(false);
    syntax.enableQuery(true); // for format flag only
    syntax.enableEdit(false);

    return syntax;
}

void* PartioExport::creator()
{
    return new PartioExport;
}

MStatus PartioExport::doIt(const MArgList& Args)
{
    MStatus status;
    MArgDatabase argData(syntax(), Args, &status);

    if (status == MStatus::kFailure)
    {
        MGlobal::displayError("Error parsing arguments" );
        return MStatus::kFailure;
    }

    if (argData.isQuery())
    {
        if (argData.isFlagSet(kFormatFlagL))
        {
            MStringArray rv;
            
            size_t n = Partio::numWriteFormats();
            for (size_t i=0; i<n; ++i)
            {
                rv.append(Partio::writeFormatExtension(i));
            }
            
            setResult(rv);
        }
        return MS::kSuccess;
    }

    if (argData.isFlagSet(kHelpFlagL))
    {
        printUsage();
        return MStatus::kFailure;
    }

    if (Args.length() < 3)
    {
        MGlobal::displayError("PartioExport-> need the EXPORT PATH and a PARTICLESHAPE's NAME, and at least one ATTRIBUTE's NAME you want to export.");
        printUsage();
        return MStatus::kFailure;
    }

    MStringArray outFiles;
    MString Path;   // directory path
    MString Format;
    MString fileNamePrefix;
    bool hasFilePrefix = false;
    // bool perFrame = false;
    double frameStep = 1.0;
    bool subFrames = false;
    bool noXML = argData.isFlagSet(kNoXMLL);
    bool skipDynamics = argData.isFlagSet(kSkipDynamicsL);
    bool worldSpace = argData.isFlagSet(kWorldSpaceL);

    if (argData.isFlagSet(kPathFlagL))
    {
        argData.getFlagArgument(kPathFlagL, 0, Path);
    }
    if (argData.isFlagSet(kFormatFlagL))
    {
        argData.getFlagArgument(kFormatFlagL, 0, Format);
    }
    if (argData.isFlagSet(kFilePrefixFlagL))
    {
        argData.getFlagArgument(kFilePrefixFlagL, 0, fileNamePrefix);
        if (fileNamePrefix.length() > 0)
        {
            hasFilePrefix = true;
        }
    }
    if (argData.isFlagSet(kFrameStepS))
    {
        argData.getFlagArgument(kFrameStepS, 0, frameStep);
        if (fabs(frameStep - floor(frameStep)) >= 0.001)
        {
            subFrames = true;
        }
    }

    Format = Format.toLowerCase();

    if (Partio::writeFormatIndex(Format.asChar()) == Partio::InvalidIndex)
    {
        MString writefmts;
        size_t n = Partio::numWriteFormats();
        for (size_t i=0; i<n; ++i)
        {
            writefmts += Partio::writeFormatExtension(i);
            if (i+1 < n)
            {
                writefmts += ", ";
            }
        }
        MGlobal::displayError("PartioExport-> Format is one of: " + writefmts);
        setResult(outFiles);
        return MStatus::kFailure;
    }

    double startFrame, endFrame;

    if (argData.isFlagSet(kMinFrameFlagL))
    {
        argData.getFlagArgument(kMinFrameFlagL, 0, startFrame);
    }
    else
    {
        startFrame = float(MAnimControl::animationStartTime().value());
    }
    if (fabs(startFrame - floor(startFrame)) >= 0.001)
    {
        subFrames = true;
    }

    if (argData.isFlagSet(kMaxFrameFlagL))
    {
        argData.getFlagArgument(kMaxFrameFlagL, 0, endFrame);
    }
    else
    {
        endFrame = float(MAnimControl::animationEndTime().value());
    }

    const bool swapUP = argData.isFlagSet(kFlipFlagL);

    if (argData.isFlagSet(kPerFrameFlagL))
    {
        // perFrame = true;
    }

    MStringArray objects;
    argData.getObjects(objects);
    if (objects.length() != 1)
    {
        MGlobal::displayError("No or many Particle Shape specified");
        return MStatus::kFailure;
    }

    MString PSName; // particle shape name
    PSName = objects[0];
    MSelectionList list;
    list.add(PSName);
    MDagPath objPath;
    list.getDagPath(0, objPath);

    if (objPath.apiType() != MFn::kParticle && objPath.apiType() != MFn::kNParticle)
    {
        MGlobal::displayError("PartioExport-> Can't find your PARTICLESHAPE.");
        setResult(outFiles);
        return MStatus::kFailure;
    }

    /// parse attribute flags
    unsigned int numUses = argData.numberOfFlagUses(kAttributeFlagL);

    /// loop thru the rest of the attributes given
    MStringArray attrNames;
    attrNames.append(MString("id"));
    attrNames.append(MString("position"));
    bool hasVelocity = false;
    bool hasAcceleration = false;
    // If -ws/-worldSpace is not set, treat worldPosition, worldVelocity, worldAcceleration as additional attributes
    // If -ws/-worldSpace is set, treat object space and world space attributes as one (only output 'position', 'velocity' and 'acceleration' in world space)

    for (unsigned int i = 0; i < numUses; i++)
    {
        MArgList argList;
        status = argData.getFlagArgumentList(kAttributeFlagL, i, argList);
        if ( !status )
        {
            setResult(outFiles);
            return status;
        }

        MString attrName = argList.asString( 0, &status );
        if ( !status )
        {
            setResult(outFiles);
            return status;
        }

        if (attrName == "position" || attrName == "id" || attrName == "particleId")
        {
            continue;
        }
        
        if (worldSpace)
        {
            if (attrName == "worldPosition")
            {
                continue;
            }
            else if (attrName == "worldVelocity")
            {
                if (!hasVelocity)
                {
                    hasVelocity = true;
                    attrNames.append("velocity");
                }
                continue;
            }
            else if (attrName == "worldAcceleration")
            {
                if (!hasAcceleration)
                {
                    hasAcceleration = true;
                    attrNames.append("acceleration");
                }
                continue;
            }
        }
        
        if (attrName == "velocity")
        {
            if (hasVelocity)
            {
                continue;
            }
            hasVelocity = true;
        }
        else if (attrName == "acceleration")
        {
            if (hasAcceleration)
            {
                continue;
            }
            hasAcceleration = true;
        }
        
        attrNames.append(attrName);
    }
    /// ARGS PARSED

    MComputation computation;
    computation.beginComputation();

    MFnParticleSystem PS(objPath);
    MMatrix W = objPath.inclusiveMatrix();
    MMatrix iW = W.inverse();

    double outFrame; // -123456
    bool firstFrame = true;
    double ticksPerFrame = (1.0 / MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit()));

    //for  (int frame = startFrame; frame<=endFrame; frame++)
    for (double frame=startFrame; frame<=endFrame; frame+=frameStep)
    {
        MTime dynTime;

        dynTime.setValue(frame);

        if (!skipDynamics)
        {
            // For some reason nParticles have started to not eval properly using lifespan unless you turn this off. 
            PS.evaluateDynamics(dynTime, firstFrame);
        }

        MGlobal::viewFrame(dynTime);

        /// Why is this being done AFTER the evaluate dynamics stuff?
        outFrame = dynTime.value();

        firstFrame = false;

        char padNum[16];

        // Temp usage for this..  PDC's  are counted by 250s..  TODO:  implement  "substeps"  setting

        if (Format == "pdc")
        {
            int substepFrame = int(outFrame * ticksPerFrame);
            // int substepFrame = outFrame;
            // substepFrame = outFrame * 250;
            sprintf(padNum, "%d", substepFrame);
        }
        else
        {
            if (subFrames)
            {
                int ff, sf;
                partio4Maya::getFrameAndSubframe(outFrame, ff, sf, 3);
                sprintf(padNum, "%04d.%03d", ff, sf);
            }
            else
            {
                sprintf(padNum, "%04d", int(outFrame));
            }
        }

        MString outputPath = Path;
        outputPath += "/";

        // If we have supplied a fileName prefix, then use it instead of the particle shape name
        if (hasFilePrefix)
        {
            outputPath += fileNamePrefix;
        }
        else
        {
            outputPath += PSName;
        }
        outputPath += ".";
        outputPath += padNum;
        outputPath += ".";
        outputPath += Format;

        MGlobal::displayInfo("PartioExport-> Exporting: "+ outputPath);

        unsigned int particleCount = PS.count();
        unsigned int numAttributes = attrNames.length();

        // In some cases, especially whenever particles are dying
        // the length of the attribute vector returned
        // from maya is smaller than the total number of particles.
        // So we have to first read all the attributes, then
        // determine the minimum amount of particles that all have valid data
        // then write the data out for them in one go. This introduces
        // code complexity, but still better than zeroing out the data
        // wherever we don't have valid data. Using zeroes could potentially
        // create popping artifacts especially if the particle system is
        // used for an instancer.
        if (particleCount > 0)
        {
            PARTIO::ParticlesDataMutable* p = PARTIO::createInterleave();
            unsigned int minParticleCount = particleCount;
            std::vector<std::pair<MIntArray*, PARTIO::ParticleAttribute> > intAttributes;
            std::vector<std::pair<MDoubleArray*, PARTIO::ParticleAttribute> > doubleAttributes;
            std::vector<std::pair<MVectorArray*, PARTIO::ParticleAttribute> > vectorAttributes;
            intAttributes.reserve(numAttributes);
            doubleAttributes.reserve(numAttributes);
            vectorAttributes.reserve(numAttributes);

            for (unsigned int i=0; i<numAttributes && minParticleCount>0; ++i)
            {
                MString attrName = attrNames[i];

                if (PS.isPerParticleIntAttribute(attrName) || attrName == "id" || attrName == "particleId")
                {
                    ///  INT Attribute found
                    MIntArray* IA = new MIntArray();
                    if (attrName == "id" || attrName == "particleId")
                    {
                        MPlugArray plugs;
                        MPlug plug = PS.findPlug("deformedPosition");
                        plug.connectedTo(plugs, true, false);
                        if (plugs.length() >= 1)
                        {
                            MObject srcP = plugs[0].node();
                            while (!srcP.hasFn(MFn::kParticle))
                            {
                                MFnDependencyNode dn(srcP);
                                plug = dn.findPlug("inputGeometry");
                                if (!plug.isNull())
                                {
                                    plugs.clear();
                                    plug.connectedTo(plugs, true, false);
                                    if (plugs.length() >= 1)
                                    {
                                        srcP = plugs[0].node();
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                            if (srcP.hasFn(MFn::kParticle))
                            {
                                MFnParticleSystem srcPS(srcP);
                                srcPS.particleIds(*IA);
                            }
                            else
                            {
                                MGlobal::displayWarning("Filling in dummy IDs for \"" + PS.name() + "\" particles");
                                for (unsigned long j=0; j<PS.count(); ++j)
                                {
                                    IA->append(j);
                                }
                            }
                        }
                        else
                        {
                            PS.particleIds(*IA);
                        }

                        attrName = "id";
                        if (Format == "pdc")
                        {
                            attrName = "particleId";
                        }
                    }
                    else
                    {
                        PS.getPerParticleAttribute(attrName, *IA, &status);
                    }
                    PARTIO::ParticleAttribute intAttribute = p->addAttribute(attrName.asChar(), PARTIO::INT, 1);
                    intAttributes.push_back(std::make_pair(IA, intAttribute));
                    minParticleCount = std::min(minParticleCount, IA->length());
                }
                else if (PS.isPerParticleDoubleAttribute(attrName))
                {
                    // DOUBLE Attribute found
                    MDoubleArray* DA = new MDoubleArray();

                    if (attrName == "radius" || attrName == "radiusPP")
                    {
                        attrName = "radiusPP";
                        PS.radius(*DA);
                    }
                    else if (attrName == "age")
                    {
                        PS.age(*DA);
                    }
                    else if (attrName == "opacity" || attrName == "opacityPP")
                    {
                        attrName = "opacityPP";
                        PS.opacity(*DA);
                    }
                    else
                    {
                        PS.getPerParticleAttribute(attrName, *DA, &status);
                    }

                    PARTIO::ParticleAttribute floatAttribute = p->addAttribute(attrName.asChar(), PARTIO::FLOAT, 1);
                    doubleAttributes.push_back(std::make_pair(DA, floatAttribute));
                    minParticleCount = std::min(minParticleCount, DA->length());
                }
                else if (PS.isPerParticleVectorAttribute(attrName))
                {
                    // VECTOR Attribute found
                    MVectorArray* VA = new MVectorArray();
                    if (attrName == "position")
                    {
                        PS.position(*VA);
                    }
                    else if (attrName == "velocity")
                    {
                        PS.velocity(*VA);
                    }
                    else if (attrName == "acceleration")
                    {
                        PS.acceleration(*VA);
                    }
                    else if (attrName == "rgbPP")
                    {
                        PS.rgb(*VA);
                    }
                    else if (attrName == "incandescencePP")
                    {
                        PS.emission(*VA);
                    }
                    else
                    {
                        PS.getPerParticleAttribute(attrName, *VA, &status);
                    }

                    PARTIO::ParticleAttribute vectorAttribute = p->addAttribute(attrName.asChar(), PARTIO::VECTOR, 3);
                    //Partio::ParticleAccessor vectorAccess(vectorAttribute);
                    vectorAttributes.push_back(std::make_pair(VA, vectorAttribute));
                    minParticleCount = std::min(minParticleCount, VA->length());
                }
            }

            // check if directory exists, and if not, create it
            struct stat st;
            if (stat(Path.asChar(), &st) < 0)
            {
#ifdef WIN32
                HWND hwnd = NULL;
                const SECURITY_ATTRIBUTES *psa = NULL;
                SHCreateDirectoryEx(hwnd, Path.asChar(), psa);
#else
                mode_t userMask = umask(0);
                umask(userMask);
                mode_t DIR_MODE = ((0777) ^ userMask);
                mkdir(Path.asChar(), DIR_MODE);
#endif
            }

            if (minParticleCount > 0)
            {
                p->addParticles(minParticleCount);

                for (std::vector<std::pair<MIntArray*, PARTIO::ParticleAttribute> >::iterator it = intAttributes.begin();
                     it != intAttributes.end(); ++it)
                {
                    PARTIO::ParticleIterator<false> pit = p->begin();
                    PARTIO::ParticleAccessor intAccessor(it->second);
                    pit.addAccessor(intAccessor);

                    for (int id = 0; pit != p->end(); ++pit)
                    {
                        intAccessor.data<PARTIO::Data<int, 1> >(pit)[0] = it->first->operator[](id++);
                    }
                }

                for (std::vector<std::pair<MDoubleArray*, PARTIO::ParticleAttribute> >::iterator it = doubleAttributes.begin();
                     it != doubleAttributes.end(); ++it)
                {
                    PARTIO::ParticleIterator<false> pit = p->begin();
                    PARTIO::ParticleAccessor doubleAccessor(it->second);
                    pit.addAccessor(doubleAccessor);

                    for (int id = 0; pit != p->end(); ++pit)
                    {
                        doubleAccessor.data<PARTIO::Data<float, 1> >(pit)[0] = static_cast<float>(it->first->operator[](id++));
                    }
                }

                for (std::vector<std::pair<MVectorArray*, PARTIO::ParticleAttribute> >::iterator it = vectorAttributes.begin();
                     it != vectorAttributes.end(); ++it)
                {
                    bool isPos = (it->second.name == "position");
                    bool isVel = (it->second.name == "velocity");
                    bool isAcc = (it->second.name == "acceleration");

                    PARTIO::ParticleIterator<false> pit = p->begin();
                    PARTIO::ParticleAccessor vectorAccessor(it->second);
                    pit.addAccessor(vectorAccessor);

                    for (int id = 0; pit != p->end(); ++pit)
                    {
                        PARTIO::Data<float, 3>& vecAttr = vectorAccessor.data<PARTIO::Data<float, 3> >(pit);
                        MVector P = it->first->operator[](id++);

                        // Note: positions returned by MFnParticleSystem::position are actually world space positions
                        if (!worldSpace && isPos)
                        {
                            MPoint pt(P);
                            pt = pt * iW;
                            P = pt;
                        }
                        
                        // Note: velocities and accelerations from MFnParticleSystem::velocity and MFnParticleSystem::acceleration are in local space
                        if (worldSpace && (isVel || isAcc))
                        {
                            P = P * W;
                        }
                        
                        vecAttr[0] = (float)P.x;

                        // Note: swapUP only works for object-space coordinates...
                        //       Should be done in world space as this actually seems to convert between Y-up and Z-up coord sys
                        if (swapUP && (isPos || isVel || isAcc))
                        {
                            vecAttr[1] = -static_cast<float>(P.z);
                            vecAttr[2] = static_cast<float>(P.y);
                        }
                        else
                        {
                            vecAttr[1] = static_cast<float>(P.y);
                            vecAttr[2] = static_cast<float>(P.z);
                        }
                    }
                }

                PARTIO::write(outputPath.asChar(), *p);
                outFiles.append(outputPath);
            }
            else
            {
                std::cout << "[partioExport] There are no particles with valid data." << std::endl;
            }

            for (std::vector<std::pair<MIntArray*, PARTIO::ParticleAttribute> >::iterator it = intAttributes.begin();
                 it != intAttributes.end(); ++it)
                delete it->first;

            for (std::vector<std::pair<MDoubleArray*, PARTIO::ParticleAttribute> >::iterator it = doubleAttributes.begin();
                 it != doubleAttributes.end(); ++it)
                delete it->first;

            for (std::vector<std::pair<MVectorArray*, PARTIO::ParticleAttribute> >::iterator it = vectorAttributes.begin();
                 it != vectorAttributes.end(); ++it)
                delete it->first;

            p->release();
        }

        // support escaping early  in export command
        if (computation.isInterruptRequested())
        {
            MGlobal::displayWarning("PartioExport detected escape being pressed, ending export early!");
            break;
        }

    } /// loop frames

    // Also output xml file
    if (outFiles.length() > 0 && !noXML)
    {
        MString scriptsDir = MGlobal::executeCommandStringResult("partioScriptsDir();");
        
        MString pyCmd = "import sys\n";
        pyCmd += "if not \"" + scriptsDir + "\" in sys.path:\n";
        pyCmd += "    sys.path.append(\"" + scriptsDir + "\")\n";
        pyCmd += "import cacheXML\n";
        pyCmd += "cacheXML.Create(\"" + outFiles[outFiles.length()-1] + "\", channelPrefix=\"" + PS.name() + "\", frameRange=(" + startFrame + ", " + endFrame + "), frameStep=" + frameStep + ")\n";
        
        MGlobal::executePythonCommand(pyCmd);
    }

    setResult(outFiles);
    return MStatus::kSuccess;
}

void PartioExport::printUsage()
{
    MString writefmts;
    size_t n = Partio::numWriteFormats();
    for (size_t i=0; i<n; ++i)
    {
        writefmts += Partio::writeFormatExtension(i);
        if (i+1 < n)
        {
            writefmts += ", ";
        }
    }
    
    MString usage = "\n-----------------------------------------------------------------------------\n";
    usage += "\tpartioExport [Options]  node \n";
    usage += "\n";
    usage += "\t[Options]\n";
    usage += "\t\t-mnf/minFrame <int> \n";
    usage += "\t\t-mxf/maxFrame <int> \n";
    usage += "\t\t-f/format <string> (format is one of: " + writefmts + ")\n";
    usage += "\t\t-atr/attribute (multi use)  <PP attribute name>\n";
    usage += "\t\t     (position/id) are always exported \n";
    usage += "\t\t-p/path    <directory file path> \n";
    usage += "\t\t-fp/filePrefix <fileNamePrefix>  \n";
    usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
    usage += "\n";
    usage += "\tExample:\n";
    usage += "\n";
    usage += "partioExport  -mnf 1 -mxf 10 -f prt -atr position -atr rgbPP -at opacityPP -fp \"SmokeShapepart1of20\" -p \"/file/path/to/output/directory\"  particleShape1 \n\n";

    MGlobal::displayInfo(usage);
}
