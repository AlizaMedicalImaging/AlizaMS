/*********************************************************
 *
 * MDCM
 *
 * Modifications github.com/issakomi
 *
 *********************************************************/

/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "mdcmUUIDGenerator.h"
#include "mdcmTrace.h"
#include "mdcmSystem.h"
#include <cstring>

// FIXME
#if defined(_WIN32) || defined(__CYGWIN__)
#define HAVE_UUIDCREATE
#else
#define HAVE_UUID_GENERATE
#endif

#ifdef HAVE_UUID_GENERATE
#include "mdcm_uuid.h"
#endif

#ifdef MDCM_HAVE_RPC_H
#include <rpc.h>
#endif

namespace mdcm
{

const char* UUIDGenerator::Generate()
{
  Unique.resize(36);
  char * uuid_data = &Unique[0];
#if defined(HAVE_UUID_GENERATE)
  assert(sizeof(uuid_t) == 16);
  uuid_t g;
  uuid_generate(g);
  uuid_unparse(g, uuid_data);
#elif defined(HAVE_UUID_CREATE)
  uint32_t rv;
  uuid_t g;
  uuid_create(&g, &rv);
  if (rv != uuid_s_ok) return NULL;
  uuid_to_string(&g, &uuid_data, &rv);
  if (rv != uuid_s_ok) return NULL;
#elif defined(HAVE_UUIDCREATE)
  UUID uuid;
  UuidCreate(&uuid);
  BYTE * str = 0;
  UuidToString(&uuid, &str);
  Unique = (char*)str;
  RpcStringFree(&str);
#else
#error should not happen
#endif
  assert(IsValid(Unique.c_str()));
  return Unique.c_str();
}

bool UUIDGenerator::IsValid(const char * suid)
{
  if(!suid) return false;
#if defined(HAVE_UUID_GENERATE)
  uuid_t uu;
  int res = uuid_parse((const char *)suid, uu);
  if(res) return false;
#elif defined(HAVE_UUID_CREATE)
  uint32_t status;
  uuid_t uuid;
  uuid_from_string(suid, &uuid, &status);
  if(status != uuid_s_ok) return false;
#elif defined(HAVE_UUIDCREATE)
  UUID uuid;
  if (FAILED(UuidFromString((unsigned char *)suid, &uuid)))
  {
    return false;
  }
#else
#error should not happen
#endif
  return true;
}


} // end namespace mdcm
