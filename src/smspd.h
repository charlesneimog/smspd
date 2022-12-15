/* 
 * Copyright (c) 2008 by Richard Thomas Eakin and other members of the
 *                       MUSIC TECHNOLOGY GROUP (MTG)
 *                         UNIVERSITAT POMPEU FABRA 
 * 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */
// If PDSMS_H is not defined
#ifndef PDSMS_H
#define PDSMS_H

#include <m_pd.h>
#include "../libsms/src/sms.h"
#include <string.h>
#include <pthread.h>

t_class *smsbuf_class;

typedef struct _smsbuf
{
        t_object x_obj;
        t_canvas *canvas;
        t_symbol *filename;
        t_symbol *bufname;
        t_outlet *outlet1; /* data list */
        t_outlet  *outlet2; /* info list (key/value pair)*/
        t_atom oneTrack[10000]; // TODO dynamically allocate this
        t_atom infolist[2];
        int nframes; // TODO: use smsHeader.nFrames
        int ready;
        int verbose; 
        int allocated; // TODO see if this is necessary (also have 'ready')
	FILE *pSmsFile;
        char param_string[1024]; //TODO get rid of this
        SMS_Header smsHeader;
        SMS_Data *smsData;
        SMS_Data *smsData2; /* a backup of the data, for when editing and you want to revert */
} t_smsbuf;




/* -_-_-_-_-_-_- objects in the smspd library -_-_-_-_-_-_-_- */
void smsbuf_setup(void);
void smsanal_setup(void);
void smssynth_tilde_setup(void);
void smsedit_setup(void);

//method for opening file in canvas directory.
t_symbol* getFullPathName( t_symbol *infilename,  t_canvas *smsCanvas); 

/* get the pointer to an smsbuf that has the given symbol name */

t_smsbuf* smspd_buffer(t_symbol *bufname);

// copy one sms header (such as from a file) to another ( the buffer).
void CopySmsHeader( SMS_Header *pFileHeader, SMS_Header *pBufHeader, char *paramString  );

#endif