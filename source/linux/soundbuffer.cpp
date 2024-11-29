#include <StdAfx.h>

#include "soundbuffer.h"

HRESULT SoundBuffer::Init(DWORD dwFlags, DWORD dwBufferSize, DWORD nSampleRate, int nChannels, LPCSTR pDevName)
{
  myBufferSize = dwBufferSize;
  mySoundBuffer.resize(dwBufferSize);
  myNumberOfUnderruns = 0;
  mySampleRate = nSampleRate;
  myChannels = nChannels;

  return DS_OK;
}

HRESULT SoundBuffer::Release()
{
  return DS_OK;
}

HRESULT SoundBuffer::Unlock( LPVOID lpvAudioPtr1, DWORD dwAudioBytes1, LPVOID lpvAudioPtr2, DWORD dwAudioBytes2 )
{
  const size_t totalWrittenBytes = dwAudioBytes1 + dwAudioBytes2;
  this->myWritePosition = (this->myWritePosition + totalWrittenBytes) % this->mySoundBuffer.size();
  myMutex.unlock();
  return DS_OK;
}

HRESULT SoundBuffer::Stop()
{
  const DWORD mask = DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;
  this->myStatus &= ~mask;
  return DS_OK;
}

HRESULT SoundBuffer::SetCurrentPosition( DWORD dwNewPosition )
{
  return DS_OK;
}

HRESULT SoundBuffer::Play( DWORD dwReserved1, DWORD dwReserved2, DWORD dwFlags )
{
  this->myStatus |= DSBSTATUS_PLAYING;
  if (dwFlags & DSBPLAY_LOOPING)
  {
    this->myStatus |= DSBSTATUS_LOOPING;
  }
  return S_OK;
}

HRESULT SoundBuffer::SetVolume( LONG lVolume )
{
  this->myVolume = lVolume;
  return DS_OK;
}

HRESULT SoundBuffer::GetStatus( LPDWORD lpdwStatus )
{
  *lpdwStatus = this->myStatus;
  return DS_OK;
}

HRESULT SoundBuffer::Restore()
{
  return DS_OK;
}

HRESULT SoundBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID * lplpvAudioPtr1, DWORD * lpdwAudioBytes1, LPVOID * lplpvAudioPtr2, DWORD * lpdwAudioBytes2, DWORD dwFlags )
{
  myMutex.lock();
  // No attempt is made at restricting write buffer not to overtake play cursor
  if (dwFlags & DSBLOCK_ENTIREBUFFER)
  {
    *lplpvAudioPtr1 = this->mySoundBuffer.data();
    *lpdwAudioBytes1 = this->mySoundBuffer.size();
    if (lplpvAudioPtr2 && lpdwAudioBytes2)
    {
      *lplpvAudioPtr2 = nullptr;
      *lpdwAudioBytes2 = 0;
    }
  }
  else
  {
    const DWORD availableInFirstPart = this->mySoundBuffer.size() - dwWriteCursor;

    *lplpvAudioPtr1 = this->mySoundBuffer.data() + dwWriteCursor;
    *lpdwAudioBytes1 = std::min(availableInFirstPart, dwWriteBytes);

    if (lplpvAudioPtr2 && lpdwAudioBytes2)
    {
      if (*lpdwAudioBytes1 < dwWriteBytes)
      {
        *lplpvAudioPtr2 = this->mySoundBuffer.data();
        *lpdwAudioBytes2 = std::min(dwWriteCursor, dwWriteBytes - *lpdwAudioBytes1);
      }
      else
      {
        *lplpvAudioPtr2 = nullptr;
        *lpdwAudioBytes2 = 0;
      }
    }
  }

  return DS_OK;
}

DWORD SoundBuffer::Read( DWORD dwReadBytes, LPVOID * lplpvAudioPtr1, DWORD * lpdwAudioBytes1, LPVOID * lplpvAudioPtr2, DWORD * lpdwAudioBytes2)
{
  const std::lock_guard<std::mutex> guard(myMutex);

  // Read up to dwReadBytes, never going past the write cursor
  // Positions are updated immediately
  const DWORD available = (this->myWritePosition - this->myPlayPosition) % this->myBufferSize;
  if (available < dwReadBytes)
  {
    dwReadBytes = available;
    ++myNumberOfUnderruns;
  }

  const DWORD availableInFirstPart = this->mySoundBuffer.size() - this->myPlayPosition;

  *lplpvAudioPtr1 = this->mySoundBuffer.data() + this->myPlayPosition;
  *lpdwAudioBytes1 = std::min(availableInFirstPart, dwReadBytes);

  if (lplpvAudioPtr2 && lpdwAudioBytes2)
  {
    if (*lpdwAudioBytes1 < dwReadBytes)
    {
      *lplpvAudioPtr2 = this->mySoundBuffer.data();
      *lpdwAudioBytes2 = dwReadBytes - *lpdwAudioBytes1;
    }
    else
    {
      *lplpvAudioPtr2 = nullptr;
      *lpdwAudioBytes2 = 0;
    }
  }
  this->myPlayPosition = (this->myPlayPosition + dwReadBytes) % this->mySoundBuffer.size();
  return dwReadBytes;
}

DWORD SoundBuffer::GetBytesInBuffer()
{
  const std::lock_guard<std::mutex> guard(myMutex);
  const DWORD available = (this->myWritePosition - this->myPlayPosition) % this->myBufferSize;
  return available;
}

HRESULT SoundBuffer::GetCurrentPosition( LPDWORD lpdwCurrentPlayCursor, LPDWORD lpdwCurrentWriteCursor )
{
  *lpdwCurrentPlayCursor = this->myPlayPosition;
  *lpdwCurrentWriteCursor = this->myWritePosition;
  return DS_OK;
}

HRESULT SoundBuffer::GetVolume( LONG * lplVolume )
{
  *lplVolume = this->myVolume;
  return DS_OK;
}

double SoundBuffer::GetLogarithmicVolume() const
{
  const double volume = (double(myVolume) - DSBVOLUME_MIN) / (0.0 - DSBVOLUME_MIN);
  return volume;
}

size_t SoundBuffer::GetBufferUnderruns() const
{
  return myNumberOfUnderruns;
}

void SoundBuffer::ResetUnderruns()
{
  myNumberOfUnderruns = 0;
}
