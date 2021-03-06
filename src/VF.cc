////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                //
// $Id: VF.cc,v 2.5 1999/07/25 13:24:23 achim Exp $
//                                                                                                                //
// BeDVI                                                                                                          //
// by Achim Blumensath                                                                                            //
// blume@corona.oche.de                                                                                           //
//                                                                                                                //
// This program is free software! It may be distributed according to the GNU Public License (see COPYING).        //
// Some of the code in this file is based on xdvi. The original copyright follows:                                //
//                                                                                                                //
// Copyright (c) 1994 Paul Vojta.  All rights reserved.                                                           //
//                                                                                                                //
// Redistribution and use in source and binary forms, with or without                                             //
// modification, are permitted provided that the following conditions                                             //
// are met:                                                                                                       //
// 1. Redistributions of source code must retain the above copyright                                              //
//    notice, this list of conditions and the following disclaimer.                                               //
// 2. Redistributions in binary form must reproduce the above copyright                                           //
//    notice, this list of conditions and the following disclaimer in the                                         //
//    documentation and/or other materials provided with the distribution.                                        //
//                                                                                                                //
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND                                         //
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE                                          //
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE                                     //
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE                                        //
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL                                     //
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS                                        //
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)                                          //
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT                                     //
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY                                      //
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF                                         //
// SUCH DAMAGE.                                                                                                   //
//                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "BeDVI.h"
#include "FontList.h"
#include "TeXFont.h"
#include "DVI.h"
#include "DVI-View.h"
#include "log.h"

static const uchar LongChar = 242;

static const uint VF_Param_1 = 20;
static const uint VF_Param_2 = 256;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                //
// bool ReadVFIndex(const DVI *doc, cosnt DrawSettings *Settings, Font *f)                                        //
//                                                                                                                //
// Reads general information from a font-file.                                                                    //
//                                                                                                                //
// DVI          *doc                    document the font appears in                                              //
// DrawSettings *Settings               settings                                                                  //
// Font         *f                      font                                                                      //
//                                                                                                                //
// Result:                              `true' if successful, otherwise `false'                                   //
//                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ReadVFIndex(const DVI *doc, const DrawSettings *Settings, Font *f)
{
  uchar Cmd;
  uchar *Avail;
  uchar *AvailEnd;
  long  CheckSum;
  ulong MaxCC = 0;

  f->ReadChar = NULL;
  f->Virtual  = true;

  f->File->Seek(ReadInt(f->File, 1), SEEK_CUR);

  CheckSum = ReadInt(f->File, 4);

  f->File->Seek(4, SEEK_CUR);

  f->FirstFont = NULL;

  for (Cmd = ReadInt(f->File, 1); Cmd >= DVI::FontDef1 && Cmd <= DVI::FontDef4; Cmd = ReadInt(f->File, 1))
  {
    Font *NewFont;

    if (!(NewFont = f->VFTable.LoadFont(doc, Settings, f->File, f, Cmd)))
      return false;

    if (f->FirstFont == NULL)
      f->FirstFont = NewFont;
  }
  f->MaxChar = ~0;

  if (!(f->Macros = new Macro[f->MaxChar + 1]))
    return false;

  memset(f->Macros, 0, (f->MaxChar + 1) * sizeof(Macro));

  Avail    = NULL;
  AvailEnd = NULL;

  for (; Cmd <= LongChar; Cmd = ReadInt(f->File, 1))
  {
    Macro *m;
    int   len;
    ulong cc;
    long  Width;

    if (Cmd == LongChar)
    {
      len   = ReadInt(f->File, 4);
      cc    = ReadInt(f->File, 4);
      Width = ReadInt(f->File, 4);

      if (cc > f->MaxChar)
      {
        f->File->Seek(len, SEEK_CUR);
        continue;
      }
    }
    else
    {
      len   = Cmd;
      cc    = ReadInt(f->File, 1);
      Width = ReadInt(f->File, 3);
    }
    if (cc > MaxCC)  MaxCC = cc;

    m = &f->Macros[cc];

    m->Advance = Width * f->DimConvert;

    if (len > 0)
    {
      if (len <= AvailEnd - Avail)
      {
        m->Position = Avail;
        Avail      += len;
      }
      else
      {
        m->FreeMe = true;

        if (len <= VF_Param_1)
        {
          if (!(m->Position = new uchar[VF_Param_2]))
            return false;
          Avail    = m->Position;
          AvailEnd = Avail + VF_Param_2;
          Avail   += len;
        }
        else
          if (!(m->Position = new uchar[len]))
            return false;
      }
      f->File->Read(m->Position, len);

      m->End = m->Position + len;
    }
  }

  if (Cmd != DVI::Postamble)
    return false;

  f->MaxChar = MaxCC;

  Macro *NewMacros = new Macro[f->MaxChar + 1];

  memcpy(NewMacros, f->Macros, (f->MaxChar + 1) * sizeof(Macro));

  delete [] f->Macros;

  f->Macros = NewMacros;

  return true;
}
