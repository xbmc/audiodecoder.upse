/*
 *      Copyright (C) 2014 Arne Morten Kvarving
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "libXBMC_addon.h"
#include <iostream>

extern "C" {
#include "upse.h"
#include <upse-internal.h>
#include <stdio.h>
#include <stdint.h>

#include "kodi_audiodec_dll.h"

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

void* xbmc_vfs_open(const char* path, const char* mode)
{
  return XBMC->OpenFile(path, 0);
}

size_t xbmc_vfs_read(void* ptr, size_t size, size_t nmemb, void* file)
{
  return XBMC->ReadFile(file, ptr, size*nmemb)/size;
}

int xbmc_vfs_seek(void* file, long offset, int whence)
{
  return XBMC->SeekFile(file, offset, whence);
}

int xbmc_vfs_close(void* file)
{
  XBMC->CloseFile(file);
  return 0;
}

long xbmc_vfs_tell(void* file)
{
  return XBMC->GetFilePosition(file);
}

const upse_iofuncs_t upse_io =
{
  xbmc_vfs_open,
  xbmc_vfs_read,
  xbmc_vfs_seek,
  xbmc_vfs_close,
  xbmc_vfs_tell
};

struct UPSEContext
{
  upse_module_t* mod;
  int16_t* buf;
  int16_t* head;
  int size;
};
             
void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  upse_module_init();
  upse_module_t* upse = upse_module_open(strFile, &upse_io);
  if (!upse)
    return NULL;

  UPSEContext* result = new UPSEContext;
  result->mod = upse;
  result->size = 0;
  result->head = result->buf;

  upse_spu_state_t * p_spu = reinterpret_cast<upse_spu_state_t *> ( upse->instance.spu );
  upse_ps1_spu_setvolume( p_spu, 32 );

  *totaltime = upse->metadata->length;
  static enum AEChannel map[3] = {
    AE_CH_FL, AE_CH_FR, AE_CH_NULL
  };
  *format = AE_FMT_S16NE;
  *channelinfo = map;
  *channels = 2;
  *bitspersample = 16;
  *bitrate = 0.0;
  *samplerate = 44100;

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  UPSEContext* upse = (UPSEContext*)context;

  if (upse->size == 0)
  {
    upse->size = 4*upse_eventloop_render(upse->mod, (int16_t**)&upse->buf);
    upse->head = upse->buf;
  }
#undef min
  *actualsize = std::min(upse->size, size);
  memcpy(pBuffer, upse->head, *actualsize);
  upse->head += *actualsize/2;
  upse->size -= *actualsize;
  return 0;
}

int64_t Seek(void* context, int64_t time)
{
  UPSEContext* upse = (UPSEContext*)context;
  upse_eventloop_seek(upse->mod, time);
  return time;
}

bool DeInit(void* context)
{
  UPSEContext* upse = (UPSEContext*)context;
  upse_eventloop_stop(upse->mod);
  upse_eventloop_render(upse->mod, (int16_t**)&upse->buf);
  upse_module_close(upse->mod);
  delete upse;

  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist, int* length)
{
  upse_psf_t* tag = upse_get_psf_metadata(strFile, &upse_io);
  if (tag)
  {
    strcpy(title, tag->title);
    strcpy(artist, tag->artist);
    *length = tag->length/1000;
    return true;
  }
  return false;
}

int TrackCount(const char* strFile)
{
  return 1;
}
}
