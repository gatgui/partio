/*
PARTIO SOFTWARE
Copyright 2013 Disney Enterprises, Inc. All rights reserved

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
#include <iostream>
#include <cassert>
#include "Mutex.h"
#include "../Partio.h"

//#####################################################################
ENTER_PARTIO_NAMESPACE

namespace
{
    static PartioMutex mutex;
}

// cached read write
std::map<ParticlesData*, int> cachedParticlesCount;
std::map<std::string, ParticlesData*> cachedParticles;

ParticlesData* readCached(const char* filename, const bool sort, const bool verbose, std::ostream &error)
{
    if (!filename)
    {
        return 0;
    }

    ParticlesData* p=0;

    std::string key = filename;
    // Normalize path before querying cache
#ifdef _WIN32
    for (std::string::iterator ci=key.begin(); ci!=key.end(); ++ci)
    {
        char c = *ci;
        if (c == '\\')
        {
            *ci = '/';
        }
        else if ('A' <= c && c <= 'Z')
        {
            *ci = 'a' + (c - 'A');
        }
    }
#else
    // Actually, on OSX too, the file system is by default case insensitive...
#endif

    mutex.lock();

    std::map<std::string,ParticlesData*>::iterator i = cachedParticles.find(key);

    if (i != cachedParticles.end())
    {
        p = i->second;
        cachedParticlesCount[p]++;
    }
    else
    {
        ParticlesDataMutable* p_rw = read(key.c_str(), verbose, error);
        if (p_rw)
        {
            if (sort)
            {
                p_rw->sort();
            }
            p = p_rw;
            cachedParticles[key] = p;
            cachedParticlesCount[p] = 1;
        }
    }

    mutex.unlock();

    return p;
}

void freeCached(ParticlesData* particles)
{
    if (!particles)
    {
        return;
    }

    mutex.lock();

    std::map<ParticlesData*, int>::iterator i = cachedParticlesCount.find(particles);

    if (i == cachedParticlesCount.end())
    {
        // Not found in cache, just free
        delete (ParticlesInfo*)particles;
    }
    else
    {
        // found in cache

        // decrement ref count
        i->second--;

        if (i->second <= 0)
        {
            // ref count is now zero, remove from structure
            delete (ParticlesInfo*)particles;

            cachedParticlesCount.erase(i);

            for (std::map<std::string, ParticlesData*>::iterator i2 = cachedParticles.begin();
                 i2 != cachedParticles.end(); ++i2)
            {
                if (i2->second == particles)
                {
                    cachedParticles.erase(i2);
                    break;
                }
            }
        }
    }

    mutex.unlock();
}

void beginCachedAccess(ParticlesData* particles)
{
    if (!particles)
    {
        return;
    }

    mutex.lock();

    std::map<ParticlesData*, int>::iterator i = cachedParticlesCount.find(particles);
    if (i != cachedParticlesCount.end())
    {
        i->second++;
    }

    mutex.unlock();
}

void endCachedAccess(ParticlesData* particles)
{
    if (!particles)
    {
        return;
    }

    mutex.lock();

    std::map<ParticlesData*, int>::iterator i = cachedParticlesCount.find(particles);
    if (i != cachedParticlesCount.end())
    {
        if (i->second - 1 <= 0)
        {
            mutex.unlock();
            freeCached(particles);
            return;
        }
        else
        {
            i->second--;
        }
    }

    mutex.unlock();
}

EXIT_PARTIO_NAMESPACE
