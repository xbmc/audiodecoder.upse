/*
 *  Copyright (C) 2014-2020 Arne Morten Kvarving
 *  Copyright (C) 2016-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include <kodi/addon-instance/AudioDecoder.h>
#include <kodi/Filesystem.h>
#include <algorithm>
#include <iostream>

extern "C" {
#include "upse.h"
#include <upse-internal.h>
#include <stdio.h>
#include <stdint.h>


void* kodi_vfs_open(const char* path, const char* mode)
{
  kodi::vfs::CFile* file = new kodi::vfs::CFile;
  if (!file->OpenFile(path, 0))
  {
    delete file;
    return nullptr;
  }
  return file;
}

size_t kodi_vfs_read(void* ptr, size_t size, size_t nmemb, void* file)
{
  kodi::vfs::CFile* cfile = static_cast<kodi::vfs::CFile*>(file);
  return cfile->Read(ptr, size*nmemb)/size;
}

int kodi_vfs_seek(void* file, long offset, int whence)
{
  kodi::vfs::CFile* cfile = static_cast<kodi::vfs::CFile*>(file);
  return cfile->Seek(offset, whence);
}

int kodi_vfs_close(void* file)
{
  delete static_cast<kodi::vfs::CFile*>(file);
  return 0;
}

long kodi_vfs_tell(void* file)
{
  kodi::vfs::CFile* cfile = static_cast<kodi::vfs::CFile*>(file);
  return cfile->GetPosition();
}

const upse_iofuncs_t upse_io =
{
  kodi_vfs_open,
  kodi_vfs_read,
  kodi_vfs_seek,
  kodi_vfs_close,
  kodi_vfs_tell
};

struct UPSEContext
{
  upse_module_t* mod = nullptr;
  int16_t* buf = nullptr;
  int16_t* head = nullptr;
  int size = 0;
};

}

class ATTRIBUTE_HIDDEN CUPSECodec : public kodi::addon::CInstanceAudioDecoder
{
public:
  CUPSECodec(KODI_HANDLE instance) :
    CInstanceAudioDecoder(instance) {}

  virtual ~CUPSECodec()
  {
    if (ctx.mod)
    {
      upse_eventloop_stop(ctx.mod);
      if (!m_endWasReached)
        upse_eventloop_render(ctx.mod, (int16_t**)&ctx.buf);
      upse_module_close(ctx.mod);
    }
  }

  bool Init(const std::string& filename, unsigned int filecache,
            int& channels, int& samplerate,
            int& bitspersample, int64_t& totaltime,
            int& bitrate, AEDataFormat& format,
            std::vector<AEChannel>& channellist) override
  {
    upse_module_init();
    upse_module_t* upse = upse_module_open(filename.c_str(), &upse_io);
    if (!upse)
    {
      m_endWasReached = true;
      return false;
    }

    ctx.mod = upse;
    ctx.size = 0;
    ctx.head = ctx.buf;

    upse_spu_state_t * p_spu = reinterpret_cast<upse_spu_state_t *> ( upse->instance.spu );
    upse_ps1_spu_setvolume( p_spu, 32 );

    totaltime = upse->metadata->length;
    format = AE_FMT_S16NE;
    channellist = { AE_CH_FL, AE_CH_FR };
    channels = 2;
    bitspersample = 16;
    bitrate = 0.0;
    samplerate = 44100;

    return true;
  }

  int ReadPCM(uint8_t* buffer, int size, int& actualsize) override
  {
    if (ctx.size == 0)
    {
      ctx.size = 4*upse_eventloop_render(ctx.mod, (int16_t**)&ctx.buf);
      ctx.head = ctx.buf;

      // If return against 0, the end of stream is reached
      if (ctx.size == 0)
      {
        m_endWasReached = true;
        return 1;
      }
    }
#undef min
    actualsize = std::min(ctx.size, size);
    memcpy(buffer, ctx.head, actualsize);
    ctx.head += actualsize/2;
    ctx.size -= actualsize;
    return 0;
  }

  int64_t Seek(int64_t time) override
  {
    upse_eventloop_seek(ctx.mod, time);
    return time;
  }

  bool ReadTag(const std::string& file, std::string& title,
               std::string& artist, int& length) override
  {
    upse_psf_t* tag = upse_get_psf_metadata(file.c_str(), &upse_io);
    if (tag)
    {
      title = tag->title;
      artist = tag->artist;
      length = tag->length/1000;
      delete tag;
      return true;
    }

    return false;
  }

private:
  UPSEContext ctx;
  bool m_endWasReached = false;
};


class ATTRIBUTE_HIDDEN CMyAddon : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    addonInstance = new CUPSECodec(instance);
    return ADDON_STATUS_OK;
  }
  virtual ~CMyAddon() = default;
};


ADDONCREATOR(CMyAddon);
