/* partio4Maya  3/12/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Export
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

#include "partioExport.h"
#include <maya/MPlugArray.h>
#include <maya/MPlug.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <maya/MAnimControl.h>
#include <maya/MPoint.h>

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


using namespace std;



bool PartioExport::hasSyntax()
{
    return true;
}

////// Has Syntax Similar to DynExport  to be a drop in replacement

MSyntax PartioExport::createSyntax()
{

    MSyntax syntax;

    syntax.addFlag(kHelpFlagS, kHelpFlagL,  MSyntax::kNoArg);
    syntax.addFlag(kPathFlagS, kPathFlagL,  MSyntax::kString);
    syntax.addFlag(kAttributeFlagS, kAttributeFlagL, MSyntax::kString);
    syntax.makeFlagMultiUse( kAttributeFlagS );
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

    if ( argData.isQuery() )
    {
        if ( argData.isFlagSet(kFormatFlagL) )
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

    if ( argData.isFlagSet(kHelpFlagL))
    {
        printUsage();
        return MStatus::kFailure;
    }


    if ( Args.length() < 3)
    {
        MGlobal::displayError("PartioExport-> Need the export path and a particle shape's name, and at least one attribute's name you want to export.");
        printUsage();
        return MStatus::kFailure;
    }

    MStringArray outFiles;
    MString Path;   // directory path
    MString Format;
    MString fileNamePrefix;
    bool hasFilePrefix = false;
    bool perFrame = false;
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

    bool swapUP = false;

    if (argData.isFlagSet(kFlipFlagL))
    {
        swapUP = true;
    }

    if (argData.isFlagSet(kPerFrameFlagL))
    {
        perFrame = true;
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

    if ( objPath.apiType() != MFn::kParticle && objPath.apiType() != MFn::kNParticle )
    {
        MGlobal::displayError("PartioExport-> Can't find your PARTICLESHAPE.");
        setResult(outFiles);
        return MStatus::kFailure;
    }

    /// parse attribute flags
    unsigned int numUses = argData.numberOfFlagUses( kAttributeFlagL );

    /// loop thru the rest of the attributes given
    MStringArray  attrNames;
    attrNames.append(MString("id"));
    attrNames.append(MString("position"));
    bool hasVelocity = false;
    bool hasAcceleration = false;
    // If -ws/-worldSpace is not set, treat worldPosition, worldVelocity, worldAcceleration as additional attributes
    // If -ws/-worldSpace is set, treat object space and world space attributes as one (only output 'position', 'velocity' and 'acceleration' in world space)

    for ( unsigned int i = 0; i < numUses; i++ )
    {
        MArgList argList;
        status = argData.getFlagArgumentList( kAttributeFlagL, i, argList );
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

        if ( attrName == "position" || attrName == "id" || attrName == "particleId" )
        {
            continue;
        }
        
        if ( worldSpace )
        {
            if ( attrName == "worldPosition" )
            {
                continue;
            }
            else if ( attrName == "worldVelocity" )
            {
                if ( !hasVelocity )
                {
                    hasVelocity = true;
                    attrNames.append( "velocity" );
                }
                continue;
            }
            else if ( attrName == "worldAcceleration" )
            {
                if ( !hasAcceleration )
                {
                    hasAcceleration = true;
                    attrNames.append( "acceleration" );
                }
                continue;
            }
        }
        
        if ( attrName == "velocity" )
        {
            if ( hasVelocity )
            {
                continue;
            }
            hasVelocity = true;
        }
        else if ( attrName == "acceleration" )
        {
            if ( hasAcceleration )
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

    double outFrame;
    bool firstFrame = true;

    //for  (int frame = startFrame; frame<=endFrame; frame++)
    for (double frame=startFrame; frame<=endFrame; frame+=frameStep)
    {
        MTime dynTime;

        dynTime.setValue(frame);

        if (!skipDynamics)
        {
            PS.evaluateDynamics(dynTime, firstFrame);
        }

        /// Why is this being done AFTER the evaluate dynamics stuff?
        MGlobal::viewFrame(dynTime);
        outFrame = dynTime.value();

        firstFrame = false;

        char padNum[16];

        // Temp usage for this..  PDC's  are counted by 250s..  TODO:  implement  "substeps"  setting

        if (Format == "pdc")
        {
            double ticksPerFrame = (1.0 / MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit()));
            int substepFrame = int(outFrame * ticksPerFrame);
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

        MString  outputPath = Path;
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

        Partio::ParticlesDataMutable* p = Partio::createInterleave();
        p->addParticles((const int)particleCount);
        Partio::ParticlesDataMutable::iterator it=p->begin();

        if (particleCount > 0)
        {
            for (unsigned int i = 0; i< attrNames.length(); i++)
            {
                // you must reset the iterator before adding new attributes or accessors
                it=p->begin();
                MString  attrName =  attrNames[i];
                //cout << attrName ;

                ///  INT Attribute found
                if (PS.isPerParticleIntAttribute(attrName) || attrName == "id" || attrName == "particleId")
                {

                    //cout << "-> INT " << endl;
                    MIntArray IA;

                    if ( attrName == "id" || attrName == "particleId" )
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
                                srcPS.particleIds(IA);
                            }
                            else
                            {
                                MGlobal::displayWarning("Filling in dummy IDs for \"" + PS.name() + "\" particles");
                                for (unsigned long j=0; j<PS.count(); ++j)
                                {
                                    IA.append(j);
                                }
                            }
                        }
                        else
                        {
                            PS.particleIds( IA );
                        }
                        attrName = "id";
                        if (Format == "pdc")
                        {
                            attrName = "particleId";
                        }
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , IA, &status);
                    }

                    Partio::ParticleAttribute  intAttribute = p->addAttribute(attrName.asChar(), Partio::INT, 1);
                    //cout <<  "partio add Int attribute" << endl;

                    Partio::ParticleAccessor intAccess(intAttribute);
                    it.addAccessor(intAccess);
                    int a = 0;

                    for (it=p->begin();it!=p->end();++it)
                    {
                        Partio::Data<int,1>& intAttr=intAccess.data<Partio::Data<int,1> >(it);
                        intAttr[0] = IA[a];
                        a++;
                    }

                }/// add INT attribute

                /// DOUBLE Attribute found
                else if  (PS.isPerParticleDoubleAttribute(attrName))
                {
                    //cout << "-> FLOAT " << endl;
                    MDoubleArray DA;

                    if ( attrName == "radius" || attrName  == "radiusPP")
                    {
                        attrName = "radiusPP";
                        PS.radius( DA );
                    }
                    if ( attrName == "age" )
                    {
                        PS.age( DA );
                    }
                    else if ( attrName == "opacity" || attrName == "opacityPP")
                    {
                        attrName = "opacityPP";
                        PS.opacity( DA );
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , DA, &status);
                    }

                    Partio::ParticleAttribute  floatAttribute = p->addAttribute(attrName.asChar(), Partio::FLOAT, 1);
                    Partio::ParticleAccessor floatAccess(floatAttribute);
                    it.addAccessor(floatAccess);
                    int a = 0;

                    for (it=p->begin();it!=p->end();++it)
                    {

                        Partio::Data<float,1>& floatAttr=floatAccess.data<Partio::Data<float,1> >(it);
                        floatAttr[0] = float(DA[a]);
                        a++;

                    }

                } /// add double attr

                /// VECTOR Attribute found
                else if (PS.isPerParticleVectorAttribute(attrName))
                {

                    //cout << "-> VECTOR " << endl;
                    MVectorArray VA;

                    if ( attrName == "position" )
                    {
                        PS.position( VA );
                    }
                    else if ( attrName == "velocity")
                    {
                        PS.velocity( VA );
                    }
                    else if (attrName == "acceleration" )
                    {
                        PS.acceleration( VA );
                    }
                    else if (attrName == "rgbPP" )
                    {
                        PS.rgb( VA );
                    }
                    else if (attrName == "incandescencePP")
                    {
                        PS.emission( VA );
                    }
                    else
                    {
                        PS.getPerParticleAttribute( attrName , VA, &status);
                    }

                    Partio::ParticleAttribute vectorAttribute = p->addAttribute(attrName.asChar(), Partio::VECTOR, 3);

                    Partio::ParticleAccessor vectorAccess(vectorAttribute);

                    it.addAccessor(vectorAccess);

                    int a = 0;

                    for (it=p->begin(); it!=p->end(); ++it, ++a)
                    {

                        Partio::Data<float,3>& vecAttr=vectorAccess.data<Partio::Data<float,3> >(it);

                        MVector P = VA[a];
                        
                        // Note: positions returned by MFnParticleSystem::position are actually world space positions
                        if (!worldSpace && attrName == "position")
                        {
                            MPoint pt(P);
                            pt = pt * iW;
                            P = pt;
                        }
                        
                        // Note: velocities and accelerations from MFnParticleSystem::velocity and MFnParticleSystem::acceleration are in local space
                        if (worldSpace && (attrName == "velocity" || attrName == "acceleration"))
                        {
                            P = P * W;
                        }
                        
                        vecAttr[0] = (float)P.x;

                        // Note: swapUP only works for object-space coordinates...
                        //       Should be done in world space as this actually seems to convert between Y-up and Z-up coord sys
                        if (swapUP && (attrName == "position" || attrName == "velocity" || attrName == "acceleration"))
                        {
                            vecAttr[1] = -(float)P.z;
                            vecAttr[2] = (float)P.y;
                        }
                        else
                        {
                            vecAttr[1] = (float)P.y;
                            vecAttr[2] = (float)P.z;
                        }
                    }

                } /// add vector attr

            } /// loop attributes

            // check if directory exists, and if not, create it
            struct stat st;
            if (stat(Path.asChar(),&st) < 0)
            {
#ifdef _WIN32
                HWND hwnd = NULL;
                const SECURITY_ATTRIBUTES *psa = NULL;
                SHCreateDirectoryEx(hwnd, Path.asChar(), psa);
#else
                mode_t userMask = umask(0);
                umask(userMask);
                mode_t DIR_MODE = ((0777) ^ userMask);
                mkdir (Path.asChar(), DIR_MODE );
#endif
            }

            Partio::write(outputPath.asChar(), *p );
            outFiles.append(outputPath);
            //cout << "partioCount" << endl;
            //cout << "end FRAME: " << outFrame << endl;
            //cout << "num particles" << p->numParticles() << endl;
            //cout << "num Attributes" << p->numAttributes() << endl;

            p->release();
            //cout << "released  memory" << endl;
        } /// if particle count > 0

        // support escaping early  in export command
        if (computation.isInterruptRequested())
        {   MGlobal::displayWarning("PartioExport detected escape being pressed, ending export early!" ) ;
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
    usage += "\t\t-p/path	 <directory file path> \n";
    usage += "\t\t-fp/filePrefix <fileNamePrefix>  \n";
    usage += "\t\t-flp/flip  (flip y->z axis to go to Z up packages) \n";
    usage += "\n";
    usage += "\tExample:\n";
    usage += "\n";
    usage += "partioExport  -mnf 1 -mxf 10 -f prt -atr position -atr rgbPP -at opacityPP -fp \"SmokeShapepart1of20\" -p \"/file/path/to/output/directory\"  particleShape1 \n\n";

    MGlobal::displayInfo(usage);

}


