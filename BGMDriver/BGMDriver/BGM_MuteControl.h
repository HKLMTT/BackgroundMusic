// This file is part of Background Music.
//
// Background Music is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 2 of the
// License, or (at your option) any later version.
//
// Background Music is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Background Music. If not, see <http://www.gnu.org/licenses/>.

//
//  BGM_MuteControl.h
//  BGMDriver
//
//  Copyright © 2017, 2026 Kyle Neideck
//

#ifndef BGMDriver__BGM_MuteControl
#define BGMDriver__BGM_MuteControl

// Superclass Includes
#include "BGM_Control.h"

// PublicUtility Includes
#include "CAMutex.h"

// STL Includes
#include <atomic>

// System Includes
#include <MacTypes.h>
#include <CoreAudio/CoreAudio.h>


#pragma clang assume_nonnull begin

class BGM_MuteControl
:
    public BGM_Control
{

#pragma mark Construction/Destruction

public:
                              BGM_MuteControl(AudioObjectID inObjectID,
                                              AudioObjectID inOwnerObjectID,
                                              AudioObjectPropertyScope inScope =
                                                      kAudioObjectPropertyScopeOutput,
                                              AudioObjectPropertyElement inElement =
                                                      kAudioObjectPropertyElementMain);

#pragma mark Property Operations

    bool                      HasProperty(AudioObjectID inObjectID,
                                          pid_t inClientPID,
                                          const AudioObjectPropertyAddress& inAddress) const;

    bool                      IsPropertySettable(AudioObjectID inObjectID,
                                                 pid_t inClientPID,
                                                 const AudioObjectPropertyAddress& inAddress) const;

    UInt32                    GetPropertyDataSize(AudioObjectID inObjectID,
                                                  pid_t inClientPID,
                                                  const AudioObjectPropertyAddress& inAddress,
                                                  UInt32 inQualifierDataSize,
                                                  const void* inQualifierData) const;

    void                      GetPropertyData(AudioObjectID inObjectID,
                                              pid_t inClientPID,
                                              const AudioObjectPropertyAddress& inAddress,
                                              UInt32 inQualifierDataSize,
                                              const void* inQualifierData,
                                              UInt32 inDataSize,
                                              UInt32& outDataSize,
                                              void* outData) const;

    void                      SetPropertyData(AudioObjectID inObjectID,
                                              pid_t inClientPID,
                                              const AudioObjectPropertyAddress& inAddress,
                                              UInt32 inQualifierDataSize,
                                              const void* inQualifierData,
                                              UInt32 inDataSize,
                                              const void* inData);

#pragma mark IO Operations

public:
    /*!
     Set whether this control should apply its mute to the device's audio data itself, i.e. whether
     clients should use ApplyMuteToAudioRT while doing IO. Used when the real output device has no
     mute control of its own for BGMApp to copy the mute state to (e.g. HDMI/DisplayPort displays).
     */
    void                      SetWillApplyMuteToAudio(bool inWillApplyMuteToAudio)
                                  { mWillApplyMuteToAudio = inWillApplyMuteToAudio; }

    /*! @return True if clients should use ApplyMuteToAudioRT while doing IO. */
    bool                      WillApplyMuteToAudioRT() const { return mWillApplyMuteToAudio; }

    /*!
     Silence the samples in ioBuffer if this control is currently muted. Realtime safe. Does nothing
     unless SetWillApplyMuteToAudio has been used to enable applying the mute to audio data.
     */
    void                      ApplyMuteToAudioRT(Float32* ioBuffer,
                                                 UInt32 inBufferFrameSize) const;

#pragma mark Implementation

private:
    CAMutex                   mMutex;
    // Atomic because it's read on the realtime IO threads by ApplyMuteToAudioRT.
    std::atomic<bool>         mMuted;
    // Set from non-realtime threads and read on the realtime IO threads, so it has to be atomic.
    std::atomic<bool>         mWillApplyMuteToAudio;

};

#pragma clang assume_nonnull end

#endif /* BGMDriver__BGM_MuteControl */

