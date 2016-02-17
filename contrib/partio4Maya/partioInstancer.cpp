/* partio4Maya  5/02/2012, John Cassella  http://luma-pictures.com and  http://redpawfx.com
PARTIO Instancer
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

#include "partioInstancer.h"
#include <maya/MArrayDataBuilder.h>

//static MGLFunctionTable *gGLFT = NULL;

#define ID_PARTIOINSTANCER  0x00116ED5 // id is registered with autodesk no need to change
#define LEAD_COLOR				18	// green
#define ACTIVE_COLOR			15	// white
#define ACTIVE_AFFECTED_COLOR	8	// purple
#define DORMANT_COLOR			4	// blue
#define HILITE_COLOR			17	// pale blue

#define DRAW_STYLE_POINTS 0
#define DRAW_STYLE_LABEL 1
#define DRAW_STYLE_BOUNDING_BOX 3

using namespace Partio;
using namespace std;

/// PARTIO INSTANCER
/*
Names and types of all array attributes  the maya instancer looks for
used by the geometry instancer nodes:  ( * = currently implemented)
position (vectorArray)  *
scale (vectorArray) *
shear (vectorArray)
visibility (doubleArray)
objectIndex (doubleArray) *
rotationType (doubleArray)
rotation (vectorArray) *
aimDirection (vectorArray)
aimPosition (vectorArray)
aimAxis (vectorArray)
aimUpAxis (vectorArray)
aimWorldUp (vectorArray)
age (doubleArray)
id (doubleArray)
*/

MTypeId partioInstancer::id( ID_PARTIOINSTANCER );

/// ATTRS
MObject partioInstancer::time;
MObject partioInstancer::aSize;         // The size of the logo
MObject partioInstancer::aFlipYZ;
MObject partioInstancer::aDrawStyle;
MObject partioInstancer::aPointSize;

/// Cache file related stuff
MObject partioInstancer::aUpdateCache;
MObject partioInstancer::aCacheDir;
MObject partioInstancer::aCacheFile;
MObject partioInstancer::aCacheActive;
MObject partioInstancer::aCacheOffset;
MObject partioInstancer::aCacheStatic;
MObject partioInstancer::aCacheFormat;
MObject partioInstancer::aForceReload;
MObject partioInstancer::aRenderCachePath;

/// point position / velocity
MObject partioInstancer::aComputeVeloPos;
MObject partioInstancer::aVeloMult;

/// attributes
MObject partioInstancer::aPartioAttributes;
MObject partioInstancer::aScaleFrom;
MObject partioInstancer::aRotationType;

MObject partioInstancer::aRotationFrom;
MObject partioInstancer::aAimDirectionFrom;
MObject partioInstancer::aAimPositionFrom;
MObject partioInstancer::aAimAxisFrom;
MObject partioInstancer::aAimUpAxisFrom;
MObject partioInstancer::aAimWorldUpFrom;

MObject partioInstancer::aLastScaleFrom;
MObject partioInstancer::aLastRotationFrom;
MObject partioInstancer::aLastAimDirectionFrom;
MObject partioInstancer::aLastAimPositionFrom;

MObject partioInstancer::aIndexFrom;

/// not implemented yet
//	MObject partioInstancer::aAimPositionFrom;
//	MObject partioInstancer::aShaderIndexFrom;
//	MObject partioInstancer::aInMeshInstances;
//	MObject partioInstancer::aOutMesh;

//  output data to instancer
MObject partioInstancer::aInstanceData;



partioInstReaderCache::partioInstReaderCache():
        token(0),
        bbox(MBoundingBox(MPoint(0,0,0,0),MPoint(0,0,0,0))),
        dList(0),
        particles(NULL),
        flipPos(NULL)
{
}


/// Constructor
partioInstancer::partioInstancer()
    : multiplier(1.0),
      cacheChanged(false),
      mLastFileLoaded(""),
      mLastPath(""),
      mLastFile(""),
      mLastExt(""),
      mFlipped(false),
      frameChanged(false),
      mLastRotationTypeIndex(-1),
      mLastRotationFromIndex(-1),
      mLastLastRotationFromIndex(-1),
      mLastAimDirectionFromIndex(-1),
      mLastLastAimDirecitonFromIndex(-1),
      mLastAimPositionFromIndex(-1),
      mLastLastAimPositionFromIndex(-1),
      mLastAimAxisFromIndex(-1),
      mLastAimUpAxisFromIndex(-1),
      mLastAimWorldUpFromIndex(-1),
      mLastScaleFromIndex(-1),
      mLastLastScaleFromIndex(-1),
      mLastIndexFromIndex(-1),
      canMotionBlur(false),
      mLastStatic(false),
      mLastOffset(0)
{
    pvCache.particles = NULL;
    pvCache.flipPos = (float *) malloc(sizeof(float));

    // create the instanceData object
    MStatus stat;
    pvCache.instanceDataObj = pvCache.instanceData.create ( &stat );
    CHECK_MSTATUS(stat);

}
/// DESTRUCTOR
partioInstancer::~partioInstancer()
{

    if (pvCache.particles)
    {
        pvCache.particles->release();
    }
    free(pvCache.flipPos);

    MSceneMessage::removeCallback( partioInstancerOpenCallback );
    MSceneMessage::removeCallback( partioInstancerImportCallback );
    MSceneMessage::removeCallback( partioInstancerReferenceCallback );

}

void* partioInstancer::creator()
{
    return new partioInstancer;
}

/// POST CONSTRUCTOR
void partioInstancer::postConstructor()
{
    setRenderable(true);
    partioInstancerOpenCallback = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, partioInstancer::reInit, this);
    partioInstancerImportCallback = MSceneMessage::addCallback(MSceneMessage::kAfterImport, partioInstancer::reInit, this);
    partioInstancerReferenceCallback = MSceneMessage::addCallback(MSceneMessage::kAfterReference, partioInstancer::reInit, this);
}

///////////////////////////////////
/// init after opening
///////////////////////////////////

void partioInstancer::initCallback()
{
    /*
    MObject tmo = thisMObject();

    short extENum;
    MPlug(tmo, aCacheFormat).getValue(extENum);

    mLastExt = partio4Maya::setExt(extENum);

    MPlug(tmo,aCacheDir).getValue(mLastPath);
    MPlug(tmo,aCacheFile).getValue(mLastFile);
    MPlug(tmo,aSize).getValue(multiplier);
    MPlug(tmo,aCacheStatic).getValue(mLastStatic);
    MPlug(tmo,aCacheOffset).getValue(mLastOffset);

    cacheChanged = false;
    */
}

void partioInstancer::reInit(void *data)
{
    partioInstancer  *instNode = (partioInstancer*) data;
    instNode->initCallback();
}


MStatus partioInstancer::initialize()
{

    MFnEnumAttribute 	eAttr;
    MFnUnitAttribute 	uAttr;
    MFnNumericAttribute nAttr;
    MFnTypedAttribute 	tAttr;
    MStatus			 	stat;

    time = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0, &stat );
    uAttr.setKeyable( true );

    aSize = uAttr.create( "iconSize", "isz", MFnUnitAttribute::kDistance );
    uAttr.setDefault( 0.25 );

    aFlipYZ = nAttr.create( "flipYZ", "fyz", MFnNumericData::kBoolean);
    nAttr.setDefault ( false );
    nAttr.setKeyable ( true );

    aCacheDir = tAttr.create ( "cacheDir", "cachD", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheFile = tAttr.create ( "cachePrefix", "cachP", MFnStringData::kString );
    tAttr.setReadable ( true );
    tAttr.setWritable ( true );
    tAttr.setKeyable ( true );
    tAttr.setConnectable ( true );
    tAttr.setStorable ( true );

    aCacheOffset = nAttr.create("cacheOffset", "coff", MFnNumericData::kDouble, 0, &stat );
    nAttr.setKeyable(true);

    aCacheStatic = nAttr.create("staticCache", "statC", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(true);

    aCacheActive = nAttr.create("cacheActive", "cAct", MFnNumericData::kBoolean, 1, &stat);
    nAttr.setKeyable(true);

    aCacheFormat = eAttr.create( "cacheFormat", "cachFmt");
    std::map<short,MString> formatExtMap;
    partio4Maya::buildSupportedExtensionList(formatExtMap,false);
    for (unsigned short i = 0; i< formatExtMap.size(); i++)
    {
        eAttr.addField(formatExtMap[i].toUpperCase(),	i);
    }

    eAttr.setDefault(short(Partio::readFormatIndex("pdc")));  // PDC
    eAttr.setChannelBox(true);

    aDrawStyle = eAttr.create( "drawStyle", "drwStyl");
    eAttr.addField("points",	0);
    eAttr.addField("index#",	1);
    //eAttr.addField("spheres",	2);
    eAttr.addField("boundingBox", 3);
    eAttr.setDefault(0);
    eAttr.setChannelBox(true);


    aPartioAttributes = tAttr.create ("partioCacheAttributes", "pioCAts", MFnStringData::kString);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder( true );

    aPointSize = nAttr.create("pointSize", "ptsz", MFnNumericData::kInt, 2, &stat);
    nAttr.setDefault(2);
    nAttr.setKeyable(true);

    aForceReload = nAttr.create("forceReload", "frel", MFnNumericData::kBoolean, false, &stat);
    nAttr.setDefault(false);
    nAttr.setKeyable(false);

    aUpdateCache = nAttr.create("updateCache", "upc", MFnNumericData::kInt, 0);
    nAttr.setHidden(true);

    aRenderCachePath = tAttr.create ( "renderCachePath", "rcp", MFnStringData::kString );
    tAttr.setHidden(true);

	aScaleFrom = nAttr.create("scaleFrom", "sfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aLastScaleFrom = nAttr.create("lastScaleFrom", "lsfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

// ROTATION attrs
	aRotationType = nAttr.create( "rotationType", "rottyp",  MFnNumericData::kInt, -1, &stat);
	nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aRotationFrom = nAttr.create("rotationFrom", "rfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aLastRotationFrom = nAttr.create("lastRotationFrom", "lrfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aAimDirectionFrom = nAttr.create("aimDirectionFrom", "adfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aLastAimDirectionFrom = nAttr.create("lastAimDirectionFrom", "ladfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aAimPositionFrom = nAttr.create("aimPositionFrom", "apfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aLastAimPositionFrom = nAttr.create("lastAimPositionFrom", "lapfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aAimAxisFrom = nAttr.create("aimAxisFrom", "aaxfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aAimUpAxisFrom = nAttr.create("aimUpAxisFrom", "auaxfrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

	aAimWorldUpFrom = nAttr.create("aimWorldUpFrom", "awufrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aIndexFrom = nAttr.create("indexFrom", "ifrm", MFnNumericData::kInt, -1, &stat);
    nAttr.setDefault(-1);
    nAttr.setKeyable(true);

    aInstanceData = tAttr.create( "instanceData", "instd", MFnArrayAttrsData::kDynArrayAttrs, &stat);
    tAttr.setKeyable(false);
    tAttr.setStorable(false);
    tAttr.setHidden(false);
    tAttr.setReadable(true);

    aComputeVeloPos = nAttr.create("computeVeloPos", "cvp", MFnNumericData::kBoolean, false, &stat);
    nAttr.setKeyable(false);

	aVeloMult = nAttr.create("veloMult", "vmul", MFnNumericData::kFloat, 1.0, &stat);
    nAttr.setKeyable(true);
	nAttr.setStorable(true);
    nAttr.setHidden(false);
    nAttr.setReadable(true);


// add attributes

    addAttribute ( aSize );
    addAttribute ( aFlipYZ );
	addAttribute ( aDrawStyle );
	addAttribute ( aPointSize );

	addAttribute ( aUpdateCache );
    addAttribute ( aCacheDir );
    addAttribute ( aCacheFile );
	addAttribute ( aCacheActive );
    addAttribute ( aCacheOffset );
    addAttribute ( aCacheStatic );
    addAttribute ( aCacheFormat );
	addAttribute ( aForceReload );
	addAttribute ( aRenderCachePath );

	addAttribute ( aComputeVeloPos );
	addAttribute ( aVeloMult );

    addAttribute ( aPartioAttributes );
	addAttribute ( aScaleFrom );
	addAttribute ( aRotationType );
    addAttribute ( aRotationFrom );
	addAttribute ( aAimDirectionFrom );
	addAttribute ( aAimPositionFrom );
	addAttribute ( aAimAxisFrom );
	addAttribute ( aAimUpAxisFrom );
	addAttribute ( aAimWorldUpFrom );

	addAttribute ( aLastScaleFrom );
	addAttribute ( aLastRotationFrom );
	addAttribute ( aLastAimDirectionFrom );
	addAttribute ( aLastAimPositionFrom );

    addAttribute ( aIndexFrom );

    addAttribute ( aInstanceData );

	addAttribute ( time );

// attribute affects

    attributeAffects ( aSize, aUpdateCache );
    attributeAffects ( aFlipYZ, aUpdateCache );
	attributeAffects ( aPointSize, aUpdateCache );
    attributeAffects ( aDrawStyle, aUpdateCache );

	attributeAffects ( aCacheDir, aUpdateCache );
    attributeAffects ( aCacheFile, aUpdateCache );
	attributeAffects ( aCacheActive, aUpdateCache );
    attributeAffects ( aCacheOffset, aUpdateCache );
    attributeAffects ( aCacheStatic, aUpdateCache );
    attributeAffects ( aCacheFormat, aUpdateCache );
    attributeAffects ( aForceReload, aUpdateCache );

	attributeAffects ( aComputeVeloPos, aUpdateCache );
	attributeAffects ( aVeloMult, aUpdateCache );

	attributeAffects ( aScaleFrom, aUpdateCache );
	attributeAffects ( aRotationType, aUpdateCache );

    attributeAffects ( aRotationFrom, aUpdateCache );
	attributeAffects ( aAimDirectionFrom, aUpdateCache );
	attributeAffects ( aAimPositionFrom, aUpdateCache );
	attributeAffects ( aAimAxisFrom, aUpdateCache );
	attributeAffects ( aAimUpAxisFrom, aUpdateCache );
	attributeAffects ( aAimWorldUpFrom, aUpdateCache );

	attributeAffects ( aLastScaleFrom, aUpdateCache );
	attributeAffects ( aLastRotationFrom, aUpdateCache );
	attributeAffects ( aLastAimDirectionFrom, aUpdateCache );
	attributeAffects ( aLastAimPositionFrom, aUpdateCache );

    attributeAffects ( aIndexFrom, aUpdateCache );

	attributeAffects ( aInstanceData, aUpdateCache );
    attributeAffects (time, aUpdateCache);
    attributeAffects (time, aInstanceData);


    return MS::kSuccess;
}


partioInstReaderCache* partioInstancer::updateParticleCache()
{
    GetPlugData(); // force update to run compute function where we want to do all the work
    return &pvCache;
}

// COMPUTE FUNCTION

MStatus partioInstancer::compute( const MPlug& plug, MDataBlock& block )
{
    MStatus stat;
	/*int rotationType 				=*/ block.inputValue( aRotationType ).asInt();
    int rotationFromIndex  			= block.inputValue( aRotationFrom ).asInt();
	int lastRotFromIndex			= block.inputValue( aLastRotationFrom ).asInt();
    int scaleFromIndex				= block.inputValue( aScaleFrom ).asInt();
	int lastScaleFromIndex			= block.inputValue( aLastScaleFrom ).asInt();
	int aimDirectionFromIndex		= block.inputValue( aAimDirectionFrom ).asInt();
	int lastAimDirectionFromIndex 	= block.inputValue( aLastAimDirectionFrom ).asInt();
	int aimPositionFromIndex		= block.inputValue( aAimPositionFrom ).asInt();
	int lastAimPositionFromIndex	= block.inputValue( aLastAimPositionFrom ).asInt();
	int aimAxisFromIndex			= block.inputValue( aAimAxisFrom ).asInt();
	int aimUpAxisFromIndex			= block.inputValue( aAimUpAxisFrom ).asInt();
	int aimWorldUpFromIndex			= block.inputValue( aAimWorldUpFrom ).asInt();
    int indexFromIndex 				= block.inputValue( aIndexFrom ).asInt();


    bool cacheActive = block.inputValue(aCacheActive).asBool();

    if (!cacheActive)
    {
        return ( MS::kSuccess );
    }

    // Determine if we are requesting the output plug for this node.
    //
    if (plug != aUpdateCache && plug != aInstanceData)
    {
        return ( MS::kUnknownParameter );
    }

    else
    {
        MString formatExt = "";
        MString frameString = "";
        MString newCacheFile = "";
        MString renderCacheFile = "";

        MString cacheDir = block.inputValue(aCacheDir).asString();
        MString cacheFile = block.inputValue(aCacheFile).asString();
        MString renderCachePath = block.inputValue( aRenderCachePath ).asString();
        bool cacheStatic = block.inputValue( aCacheStatic ).asBool();
        double cacheOffset = block.inputValue( aCacheOffset ).asDouble();
        /*short cacheFormat =*/ block.inputValue( aCacheFormat ).asShort();
        bool forceReload = block.inputValue( aForceReload ).asBool();
        MTime inputTime = block.inputValue( time ).asTime();
        bool flipYZ = block.inputValue( aFlipYZ ).asBool();
        bool computeMotionBlur = block.inputValue( aComputeVeloPos ).asBool();
        float veloMult = block.inputValue ( aVeloMult ).asFloat();

        inputTime = MTime(inputTime.value() + cacheOffset, MTime::uiUnit());

        if (cacheDir == "" || cacheFile == "" )
        {
            // too much noise!
            //MGlobal::displayError("PartioEmitter->Error: Please specify cache file!");
            return MS::kFailure;
        }
        else if (cacheDir != mLastPath || cacheFile != mLastFile)
        {
            MString path = cacheDir + "/" + cacheFile;
            MString dn, bn;
            MTime frame;
            
            renderCacheFile = path;
            
            partio4Maya::identifyPath(path, dn, bn, frameString, frame, formatExt);
            partio4Maya::getFileList(dn, bn, formatExt, mCacheFiles);
            
            if (mCacheFiles.size() > 0)
            {
                int idx = path.rindexW(frameString);
                if (idx != -1)
                {
                    renderCacheFile = path.substringW(0, idx-1);
                    renderCacheFile += "<frame>";
                    renderCacheFile += path.substringW(idx+frameString.length(), path.length()-1);
                }
            }
        }
        else
        {
            formatExt = mLastExt;
            renderCacheFile = renderCachePath;
        }

        float deltaTime = 0;
        bool motionBlurStep = false;
        partio4Maya::CacheFiles::const_iterator fit;
        
        // if file doesn't exists, get previous or next, and interpolate using velocity
        if (cacheStatic)
        {
            newCacheFile = cacheDir + "/" + cacheFile;
        }
        else if (partio4Maya::findCacheFile(mCacheFiles, partio4Maya::FM_CLOSEST, inputTime, fit))
        {
            newCacheFile = fit->second;
            deltaTime = float(inputTime.value() - fit->first.value());
        }

        if ((deltaTime < 1 || deltaTime > -1) && deltaTime != 0)
        {
            motionBlurStep = true;
        }

        cacheChanged = false;

//////////////////////////////////////////////
/// Cache can change manually by changing one of the parts of the cache input...
        if (mLastExt != formatExt ||
            mLastPath != cacheDir ||
            mLastFile != cacheFile ||
            mLastFlipStatus  != flipYZ ||
            mLastStatic != cacheStatic ||
            mLastOffset != cacheOffset ||
            forceReload)
        {
            cacheChanged = true;
            mFlipped = false;
            mLastFlipStatus = flipYZ;
            mLastExt = formatExt;
            mLastPath = cacheDir;
            mLastFile = cacheFile;
            mLastStatic = cacheStatic;
            mLastOffset = cacheOffset;
            block.outputValue(aForceReload).setBool(false);
        }

        if (renderCachePath != newCacheFile || renderCachePath != mLastFileLoaded)
        {
            block.outputValue(aRenderCachePath).setString(newCacheFile);
        }

//////////////////////////////////////////////
/// or it can change from a time input change

        if (!partio4Maya::cacheExists(newCacheFile.asChar()))
        {
            pvCache.particles=0; // resets the particles
            pvCache.bbox.clear();
            pvCache.instanceData.clear();
            mLastFileLoaded = "";
        }

        if ( newCacheFile != "" && partio4Maya::cacheExists(newCacheFile.asChar()) && (newCacheFile != mLastFileLoaded || forceReload) )
        {
            cacheChanged = true;
            mFlipped = false;
            MGlobal::displayWarning(MString("PartioInstancer->Loading: " + newCacheFile));
            pvCache.particles=0; // resets the particles

            pvCache.particles=read(newCacheFile.asChar());

            mLastFileLoaded = newCacheFile;
            if (pvCache.particles->numParticles() == 0)
            {
                pvCache.instanceData.clear();
                return (MS::kSuccess);
            }

            char partCount[50];
            sprintf (partCount, "%d", pvCache.particles->numParticles());
            //MGlobal::displayInfo(MString ("PartioInstancer-> LOADED: ") + partCount + MString (" particles"));
            block.outputValue(aForceReload).setBool(false);
            block.setClean(aForceReload);

            pvCache.instanceData.clear();

        }

        if (pvCache.particles)
        {

			if (!pvCache.particles->attributeInfo("id",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ID",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("particleId",pvCache.idAttr) &&
                !pvCache.particles->attributeInfo("ParticleId",pvCache.idAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find id attribute ");
                return ( MS::kFailure );
            }

            if (!pvCache.particles->attributeInfo("position",pvCache.positionAttr) &&
                !pvCache.particles->attributeInfo("Position",pvCache.positionAttr))
            {
                MGlobal::displayError("PartioInstancer->Failed to find position attribute ");
                return ( MS::kFailure );
            }

            //MFnArrayAttrsData::Type vectorType(MFnArrayAttrsData::kVectorArray);
            //MFnArrayAttrsData::Type doubleType(MFnArrayAttrsData::kDoubleArray);

			// instanceData arrays
			MVectorArray  positionArray;
			MDoubleArray  idArray;

			// this creates or  gets an existing handles to the instanceData and then clears it to be ready to fill

			updateInstanceDataVector( pvCache, positionArray, MString("position"));
			updateInstanceDataDouble( pvCache, idArray, MString("id"));


            canMotionBlur = false;
            if (computeMotionBlur)
            {
                if ((pvCache.particles->attributeInfo("velocity",pvCache.velocityAttr) ||
                     pvCache.particles->attributeInfo("Velocity",pvCache.velocityAttr))||
                     pvCache.particles->attributeInfo("V"		,pvCache.velocityAttr))
                {
                    canMotionBlur = true;
                }
                else
                {
                    MGlobal::displayWarning("PartioInstancer->Failed to find velocity attribute motion blur will be impaired ");
                }
            }

            pvCache.bbox.clear();


			// first we do position and ID because we need those two for sure
			for (int i=0;i<pvCache.particles->numParticles();i++)
			{
				const float * partioPositions = pvCache.particles->data<float>(pvCache.positionAttr,i);
				MPoint pos (partioPositions[0], partioPositions[1], partioPositions[2]);

				if (canMotionBlur)
				{
					const float * vel = pvCache.particles->data<float>(pvCache.velocityAttr,i);

					MVector velo(vel[0],vel[1],vel[2]);
					if (motionBlurStep)
					{
						float mFps = (float)(MTime(1.0, MTime::kSeconds).asUnits(MTime::uiUnit()));
						pos += ((velo*veloMult)/mFps)*deltaTime;
					}
				}

				positionArray.append(pos);

				const int* attrVal    = pvCache.particles->data<int>(pvCache.idAttr,i);
                idArray.append((double)attrVal[0]);

				// resize the bounding box
				pvCache.bbox.expand(pos);
			}


            if  ( 	motionBlurStep 			|| cacheChanged ||
					scaleFromIndex 			!= mLastScaleFromIndex ||
					rotationFromIndex 		!= mLastRotationFromIndex ||
					aimDirectionFromIndex 	!= mLastAimDirectionFromIndex ||
					aimPositionFromIndex 	!= mLastAimPositionFromIndex ||
					aimAxisFromIndex 		!= mLastAimAxisFromIndex ||
					aimUpAxisFromIndex 		!= mLastAimUpAxisFromIndex ||
					aimWorldUpFromIndex 	!= mLastAimWorldUpFromIndex ||
					indexFromIndex 			!= mLastIndexFromIndex ||
					lastScaleFromIndex      != mLastLastScaleFromIndex ||
					lastRotFromIndex		!= mLastLastRotationFromIndex ||
					lastAimDirectionFromIndex!= mLastLastAimDirecitonFromIndex ||
					lastAimPositionFromIndex!= mLastLastAimPositionFromIndex
				)
            {

				MDoubleArray  indexArray;
				MVectorArray  scaleArray;
				MVectorArray  rotationArray;
				MDoubleArray  visibiltyArray;
				MVectorArray  aimDirectionArray;
				MVectorArray  aimPositionArray;
				MVectorArray  aimAxisArray;
				MVectorArray  aimUpAxisArray;
				MVectorArray  aimWorldUpArray;

				// Index
				if (indexFromIndex >=0)
				{
					updateInstanceDataDouble( pvCache, indexArray, MString("objectIndex"));
					pvCache.particles->attributeInfo(indexFromIndex,pvCache.indexAttr);
				}
				// Scale
                if (scaleFromIndex >=0)
                {
					updateInstanceDataVector( pvCache, scaleArray, MString("scale"));
					pvCache.particles->attributeInfo(scaleFromIndex,pvCache.scaleAttr);
					if (lastScaleFromIndex >=0)
					{
						pvCache.particles->attributeInfo(lastScaleFromIndex,pvCache.lastScaleAttr);
					}
					else
					{
						pvCache.particles->attributeInfo(scaleFromIndex,pvCache.lastScaleAttr);
					}
				}
                // Rotation
                if (rotationFromIndex >= 0)
                {
					updateInstanceDataVector( pvCache, rotationArray, MString("rotation"));
					pvCache.particles->attributeInfo(rotationFromIndex,pvCache.rotationAttr);
					if (lastRotFromIndex >= 0)
					{
						pvCache.particles->attributeInfo(lastRotFromIndex,pvCache.lastRotationAttr);
					}
					else
					{
						pvCache.particles->attributeInfo(rotationFromIndex,pvCache.lastRotationAttr);
					}

				}

                // Aim Direction
                if (aimDirectionFromIndex >= 0)
                {
					updateInstanceDataVector( pvCache, aimDirectionArray, MString("aimDirection"));
					pvCache.particles->attributeInfo(aimDirectionFromIndex,pvCache.aimDirAttr);
					if (lastAimDirectionFromIndex >= 0)
					{
						pvCache.particles->attributeInfo(lastAimDirectionFromIndex,pvCache.lastAimDirAttr);
					}
					else
					{
						pvCache.particles->attributeInfo(aimDirectionFromIndex,pvCache.lastAimDirAttr);
					}
				}
				// Aim Position
				if (aimPositionFromIndex >= 0)
				{
					updateInstanceDataVector( pvCache, aimPositionArray, MString("aimPosition"));
					pvCache.particles->attributeInfo(aimPositionFromIndex,pvCache.aimPosAttr);
					if (lastAimPositionFromIndex >= 0)
					{
						pvCache.particles->attributeInfo(lastAimPositionFromIndex,pvCache.lastAimPosAttr);
					}
					else
					{
						pvCache.particles->attributeInfo(aimPositionFromIndex,pvCache.lastAimPosAttr);
					}
				}
				// Aim Axis
				if (aimAxisFromIndex >= 0)
				{
					updateInstanceDataVector( pvCache, aimAxisArray, MString("aimAxis"));
					pvCache.particles->attributeInfo(aimAxisFromIndex,pvCache.aimAxisAttr);
				}
				// Aim Up Axis
				if (aimUpAxisFromIndex >= 0)
				{
					updateInstanceDataVector( pvCache, aimUpAxisArray, MString("aimUpAxis"));
					pvCache.particles->attributeInfo(aimUpAxisFromIndex,pvCache.aimUpAttr);
				}
				// World Up Axis
				if (aimWorldUpFromIndex >= 0)
				{
					updateInstanceDataVector( pvCache, aimWorldUpArray, MString("aimWorldUp"));
					pvCache.particles->attributeInfo(aimWorldUpFromIndex,pvCache.aimWorldUpAttr);
				}

				// MAIN LOOP ON PARTICLES
				for (int i=0;i<pvCache.particles->numParticles();i++)
				{
					// SCALE
                    if (pvCache.scaleAttr.count == 1)  // single float value for scale
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
						float scale = attrVal[0];
						if (canMotionBlur)
						{
							if (pvCache.lastScaleAttr.count == 1)
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr,i);
								scale += (attrVal[0] - lastAttrVal[0])*deltaTime;
							}
						}
						scaleArray.append(MVector(scale,scale,scale));
                    }
                    else if (pvCache.scaleAttr.count >= 3)   // we have a 4 float attribute ?
                    {

						const float * attrVal = pvCache.particles->data<float>(pvCache.scaleAttr,i);
						MVector scale = MVector(attrVal[0],attrVal[1],attrVal[2]);
						if (canMotionBlur)
						{
							if (pvCache.lastScaleAttr.count >=3 )
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastScaleAttr,i);
								scale.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
								scale.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
								scale.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
							}
						}
						scaleArray.append(scale);
                    }
					// ROTATION
					if (pvCache.rotationAttr.count == 1)  // single float value for rotation
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
						float rot = attrVal[0];
						if (canMotionBlur && lastRotFromIndex >= 0)
						{
							if (pvCache.lastRotationAttr.count == 1)
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr,i);
								rot += (attrVal[0] - lastAttrVal[0])*deltaTime;
							}
						}
						rotationArray.append(MVector(rot,rot,rot));
					}
					else if (pvCache.rotationAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.rotationAttr,i);
						MVector rot = MVector(attrVal[0],attrVal[1],attrVal[2]);
						if (canMotionBlur && lastRotFromIndex >= 0)
						{
							if (pvCache.lastRotationAttr.count >=3 )
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastRotationAttr,i);
								rot.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
								rot.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
								rot.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
							}
						}
						rotationArray.append(rot);
                    }

					// AIM DIRECTION
					if (pvCache.aimDirAttr.count == 1)  // single float value for aimDirection
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr,i);
						float aimDir = attrVal[0];
						if (canMotionBlur)
						{
							if (pvCache.lastAimDirAttr.count == 1)
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr,i);
								aimDir += (attrVal[0] - lastAttrVal[0])*deltaTime;
							}
						}
						aimDirectionArray.append(MVector(aimDir,aimDir,aimDir));
                    }
                    else if (pvCache.aimDirAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimDirAttr,i);
						MVector aimDir = MVector(attrVal[0],attrVal[1],attrVal[2]);
						if (canMotionBlur)
						{
							if (pvCache.lastAimDirAttr.count >=3 )
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimDirAttr,i);
								aimDir.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
								aimDir.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
								aimDir.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
								/// TODO: figure out why this is not working on subframes correctly
								//cout << lastAttrVal[0] << " " << lastAttrVal[1] << " " << lastAttrVal[2] << endl;
							}
						}
						aimDirectionArray.append(aimDir);
                    }
                    // AIM POSITION
					if (pvCache.aimPosAttr.count == 1)  // single float value for aimDirection
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr,i);
						float aimPos = attrVal[0];
						if (canMotionBlur)
						{
							if (pvCache.lastAimPosAttr.count == 1)
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr,i);
								aimPos += (attrVal[0] - lastAttrVal[0])*deltaTime;
							}
						}
						aimPositionArray.append(MVector(aimPos,aimPos,aimPos));
                    }
                    else if (pvCache.aimPosAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimPosAttr,i);
						MVector aimPos = MVector(attrVal[0],attrVal[1],attrVal[2]);
						if (canMotionBlur)
						{
							if (pvCache.lastAimPosAttr.count >=3 )
							{
								const float * lastAttrVal = pvCache.particles->data<float>(pvCache.lastAimPosAttr,i);
								aimPos.x += (attrVal[0] - lastAttrVal[0])*deltaTime;
								aimPos.y += (attrVal[1] - lastAttrVal[1])*deltaTime;
								aimPos.z += (attrVal[2] - lastAttrVal[2])*deltaTime;
							}
						}
						aimPositionArray.append(aimPos);
                    }
                    // AIM Axis
					if (pvCache.aimAxisAttr.count == 1)  // single float value for aimDirection
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr,i);
						float aimAxis = attrVal[0];
						aimAxisArray.append(MVector(aimAxis,aimAxis,aimAxis));
                    }
                    else if (pvCache.aimAxisAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimAxisAttr,i);
						MVector aimAxis = MVector(attrVal[0],attrVal[1],attrVal[2]);
						aimAxisArray.append(aimAxis);
                    }
                    // AIM Up Axis
					if (pvCache.aimUpAttr.count == 1)  // single float value for aimDirection
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr,i);
						float aimUp = attrVal[0];
						aimUpAxisArray.append(MVector(aimUp,aimUp,aimUp));
                    }
                    else if (pvCache.aimUpAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimUpAttr,i);
						MVector aimUp = MVector(attrVal[0],attrVal[1],attrVal[2]);
						aimUpAxisArray.append(aimUp);
                    }
                    // World Up Axis
					if (pvCache.aimWorldUpAttr.count == 1)  // single float value for aimDirection
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr,i);
						float worldUp = attrVal[0];
						aimWorldUpArray.append(MVector(worldUp,worldUp,worldUp));
                    }
                    else if (pvCache.aimWorldUpAttr.count >= 3)   // we have a 4 float attribute ?
                    {
						const float * attrVal = pvCache.particles->data<float>(pvCache.aimWorldUpAttr,i);
						MVector worldUp = MVector(attrVal[0],attrVal[1],attrVal[2]);
						aimWorldUpArray.append(worldUp);
                    }
					// INDEX
                    if (pvCache.indexAttr.count == 1)  // single float value for index
					{
						if (pvCache.indexAttr.type == Partio::FLOAT)
						{
							const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
							indexArray.append((double)(int)attrVal[0]);
						}
						else if (pvCache.indexAttr.type == Partio::INT)
						{
							const int * attrVal = pvCache.particles->data<int>(pvCache.indexAttr,i);
							indexArray.append((double)attrVal[0]);
						}
					}
					else if (pvCache.indexAttr.count >= 3)   // we have a 3or4 float attribute
					{
						const float * attrVal = pvCache.particles->data<float>(pvCache.indexAttr,i);
						indexArray.append((double)(int)attrVal[0]);
					}
				} // end frame loop

				mLastScaleFromIndex = scaleFromIndex;
                mLastRotationFromIndex = rotationFromIndex;
				mLastAimDirectionFromIndex = aimDirectionFromIndex;
				mLastAimPositionFromIndex = aimPositionFromIndex;
				mLastAimAxisFromIndex = aimAxisFromIndex;
				mLastAimUpAxisFromIndex = aimUpAxisFromIndex;
				mLastAimWorldUpFromIndex = aimWorldUpFromIndex;
				mLastIndexFromIndex = indexFromIndex;
				mLastLastScaleFromIndex = lastScaleFromIndex;
				mLastLastRotationFromIndex = lastRotFromIndex;
				mLastLastAimDirecitonFromIndex = lastAimDirectionFromIndex;
				mLastLastAimPositionFromIndex = lastAimPositionFromIndex;

            } // end if frame/attrs changed

        } // end if particles
		else
		{
			pvCache.instanceData.clear();
		}
        //cout << pvCache.instanceData.list()<< endl;
        block.outputValue(aInstanceData).set(pvCache.instanceDataObj);
        block.setClean(aInstanceData);
    }


    if (pvCache.particles) // update the AE Controls for attrs in the cache
    {
        unsigned int numAttr=pvCache.particles->numAttributes();
        MPlug zPlug (thisMObject(), aPartioAttributes);

        if ((rotationFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aRotationFrom).setInt(-1);
        }
        if ((scaleFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aScaleFrom).setInt(-1);
        }
        if ((lastRotFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aLastRotationFrom).setInt(-1);
        }
        if ((lastScaleFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aLastScaleFrom).setInt(-1);
        }
        if ((indexFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aIndexFrom).setInt(-1);
        }
        if ((aimDirectionFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aAimDirectionFrom).setInt(-1);
        }
        if ((aimPositionFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aAimPositionFrom).setInt(-1);
        }
        if ((aimAxisFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aAimAxisFrom).setInt(-1);
        }
        if ((aimWorldUpFromIndex+1) > int(zPlug.numElements()))
        {
            block.outputValue(aAimWorldUpFrom).setInt(-1);
        }

        if (cacheChanged || zPlug.numElements() != numAttr) // update the AE Controls for attrs in the cache
        {
            //cout << "partioInstancer->refreshing AE controls" << endl;

            attributeList.clear();

            for (unsigned int i=0;i<numAttr;i++)
            {
                ParticleAttribute attr;
                pvCache.particles->attributeInfo(i,attr);

                // crazy casting string to  char
                char *temp;
                temp = new char[(attr.name).length()+1];
                strcpy (temp, attr.name.c_str());

                MString  mStringAttrName("");
                mStringAttrName += MString(temp);

                zPlug.selectAncestorLogicalIndex(i,aPartioAttributes);
                zPlug.setValue(MString(temp));
                attributeList.append(mStringAttrName);

                delete [] temp;
            }

            MArrayDataHandle hPartioAttrs = block.inputArrayValue(aPartioAttributes);
            MArrayDataBuilder bPartioAttrs = hPartioAttrs.builder();
            // do we need to clean up some attributes from our array?
            if (bPartioAttrs.elementCount() > numAttr)
            {
                unsigned int current = bPartioAttrs.elementCount();
                //unsigned int attrArraySize = current - 1;

                // remove excess elements from the end of our attribute array
                for (unsigned int x = numAttr; x < current; x++)
                {
                    bPartioAttrs.removeElement(x);
                }
            }
        }
    }
    block.setClean(plug);
    return MS::kSuccess;
}


/////////////////////////////////////////////////////
/// procs to override bounding box mode...
bool partioInstancer::isBounded() const
{
    return true;
}

MBoundingBox partioInstancer::boundingBox() const
{
    // Returns the bounding box for the shape.
    partioInstancer* nonConstThis = const_cast<partioInstancer*>(this);
    partioInstReaderCache* geom = nonConstThis->updateParticleCache();
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
    MPoint corner1 = geom->bbox.min();
    MPoint corner2 = geom->bbox.max();
    return MBoundingBox( corner1, corner2 );
}

// these two functions  check and clean out the instance array members if they exist or make them if they don't

void partioInstancer::updateInstanceDataVector ( partioInstReaderCache &pvCache, MVectorArray &arrayToCheck, MString arrayChannel )
{
	MStatus stat;
	MFnArrayAttrsData::Type vectorType(MFnArrayAttrsData::kVectorArray);

	if (pvCache.instanceData.checkArrayExist(arrayChannel,vectorType))
	{
		arrayToCheck = pvCache.instanceData.getVectorData(arrayChannel,&stat);
		CHECK_MSTATUS(stat);
	}
	else
	{
		arrayToCheck = pvCache.instanceData.vectorArray(arrayChannel,&stat);
		CHECK_MSTATUS(stat);
	}
	arrayToCheck.clear();
}

void partioInstancer::updateInstanceDataDouble ( partioInstReaderCache &pvCache, MDoubleArray &arrayToCheck, MString arrayChannel )
{
	MStatus stat;
	MFnArrayAttrsData::Type doubleType(MFnArrayAttrsData::kDoubleArray);

	if (pvCache.instanceData.checkArrayExist(arrayChannel,doubleType))
	{
		arrayToCheck = pvCache.instanceData.getDoubleData(arrayChannel,&stat);
		CHECK_MSTATUS(stat);
	}
	else
	{
		arrayToCheck = pvCache.instanceData.doubleArray(arrayChannel,&stat);
		CHECK_MSTATUS(stat);
	}
	arrayToCheck.clear();
}


//
// Select function. Gets called when the bbox for the object is selected.
// This function just selects the object without doing any intersection tests.
//
bool partioInstancerUI::select( MSelectInfo &selectInfo,
                                MSelectionList &selectionList,
                                MPointArray &worldSpaceSelectPts ) const
{
    MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
    MSelectionList item;
    item.add( selectInfo.selectPath() );
    MPoint xformedPt;
    selectInfo.addSelection( item, xformedPt, selectionList,
                             worldSpaceSelectPts, priorityMask, false );
    return true;
}

//////////////////////////////////////////////////////////////////
////  getPlugData is a util to update the drawing of the UI stuff

bool partioInstancer::GetPlugData()
{
    MObject thisNode = thisMObject();
    int update = 0;
    MPlug updatePlug(thisNode, aUpdateCache );
    updatePlug.getValue( update );
    if (update != dUpdate)
    {
        dUpdate = update;
        return true;
    }
    else
    {
        return false;
    }
    return false;

}

// note the "const" at the end, its different than other draw calls
void partioInstancerUI::draw( const MDrawRequest& request, M3dView& view ) const
{
    MDrawData data = request.drawData();

    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    partioInstReaderCache* cache = (partioInstReaderCache*) data.geometry();

    MObject thisNode = shapeNode->thisMObject();
    MPlug sizePlug( thisNode, shapeNode->aSize );
    MDistance sizeVal;
    sizePlug.getValue( sizeVal );

    shapeNode->multiplier= (float) sizeVal.asCentimeters();

    int drawStyle = 0;
    MPlug drawStylePlug( thisNode, shapeNode->aDrawStyle );
    drawStylePlug.getValue( drawStyle );

    view.beginGL();

    if (drawStyle < 3 && view.displayStyle() != M3dView::kBoundingBox )
    {
        drawPartio(cache,drawStyle,view);
    }
    else
    {
        drawBoundingBox();
    }

    partio4Maya::drawPartioLogo(shapeNode->multiplier);

    view.endGL();
}

////////////////////////////////////////////
/// DRAW Bounding box
void  partioInstancerUI::drawBoundingBox() const
{


    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    MPoint  bboxMin = shapeNode->pvCache.bbox.min();
    MPoint  bboxMax = shapeNode->pvCache.bbox.max();

    float xMin = float(bboxMin.x);
    float yMin = float(bboxMin.y);
    float zMin = float(bboxMin.z);
    float xMax = float(bboxMax.x);
    float yMax = float(bboxMax.y);
    float zMax = float(bboxMax.z);

    /// draw the bounding box
    glBegin (GL_LINES);

    glColor3f(1.0f,0.5f,0.5f);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMax,yMin,zMax);

    glVertex3f (xMin,yMin,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMin,zMax);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMin,zMax);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMax,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMax,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMax,yMax,zMax);

    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMax,yMax,zMin);


    glVertex3f (xMin,yMax,zMin);
    glVertex3f (xMin,yMin,zMin);

    glVertex3f (xMax,yMax,zMin);
    glVertex3f (xMax,yMin,zMin);

    glVertex3f (xMin,yMax,zMax);
    glVertex3f (xMin,yMin,zMax);

    glVertex3f (xMax,yMax,zMax);
    glVertex3f (xMax,yMin,zMax);

    glEnd();

}


////////////////////////////////////////////
/// DRAW PARTIO

void partioInstancerUI::drawPartio(partioInstReaderCache* pvCache, int drawStyle, M3dView &view) const
{
    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();

    MObject thisNode = shapeNode->thisMObject();
//	MPlug flipYZPlug( thisNode, shapeNode->aFlipYZ );
//	bool flipYZVal;
//	flipYZPlug.getValue( flipYZVal );

    MPlug pointSizePlug( thisNode, shapeNode->aPointSize );
    float pointSizeVal;
    pointSizePlug.getValue( pointSizeVal );

    //int stride =  3*sizeof(float);

    if (pvCache->particles)
    {
        struct Point {
            float p[3];
        };
        glPushAttrib(GL_CURRENT_BIT);

		/// looping thru particles one by one...

        glPointSize(pointSizeVal);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POINTS);

		MVectorArray positions = pvCache->instanceData.vectorArray("position");
        for (unsigned int i=0; i < positions.length(); i++)
        {
            glVertex3d(positions[i].x, positions[i].y, positions[i].z);
        }

        glEnd( );
        glDisable(GL_POINT_SMOOTH);

        if (drawStyle == 1)
        {
            glColor3f(0.0f, 0.0f, 0.0f);
            for (unsigned int i=0; i < positions.length();i++)
            {
				MString idVal;
				if (pvCache->indexAttr.type == Partio::FLOAT)
				{
					const float * attrVal = pvCache->particles->data<float>(pvCache->indexAttr,i);
					idVal = (double)(int)attrVal[0];
				}
				else if (pvCache->indexAttr.type == Partio::INT)
				{
					const int * attrVal = pvCache->particles->data<int>(pvCache->indexAttr,i);
					idVal = (double)attrVal[0];
				}
				else if (pvCache->indexAttr.type == Partio::VECTOR)
				{
					const float * attrVal = pvCache->particles->data<float>(pvCache->indexAttr,i);
					idVal = (double)attrVal[0];
				}
                /// TODO: draw text label per particle here
                view.drawText(idVal, MPoint(positions[i].x, positions[i].y, positions[i].z), M3dView::kLeft);
            }
        }

        glPopAttrib();
    } // if (particles)

}

partioInstancerUI::partioInstancerUI()
{
}
partioInstancerUI::~partioInstancerUI()
{
}

void* partioInstancerUI::creator()
{
    return new partioInstancerUI();
}


void partioInstancerUI::getDrawRequests(const MDrawInfo & info,
                                        bool /*objectAndActiveOnly*/, MDrawRequestQueue & queue)
{

    MDrawData data;
    MDrawRequest request = info.getPrototype(*this);
    partioInstancer* shapeNode = (partioInstancer*) surfaceShape();
    partioInstReaderCache* geom = shapeNode->updateParticleCache();

    getDrawData(geom, data);
    request.setDrawData(data);

    // Are we displaying locators?
    if (!info.objectDisplayStatus(M3dView::kDisplayLocators))
        return;

    M3dView::DisplayStatus displayStatus = info.displayStatus();
    M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
    M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;
    switch (displayStatus)
    {
    case M3dView::kLead:
        request.setColor(LEAD_COLOR, activeColorTable);
        break;
    case M3dView::kActive:
        request.setColor(ACTIVE_COLOR, activeColorTable);
        break;
    case M3dView::kActiveAffected:
        request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
        break;
    case M3dView::kDormant:
        request.setColor(DORMANT_COLOR, dormantColorTable);
        break;
    case M3dView::kHilite:
        request.setColor(HILITE_COLOR, activeColorTable);
        break;
    default:
        break;
    }
    queue.add(request);

}

