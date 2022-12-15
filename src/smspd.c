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

#include "smspd.h"


#define REQUIRE_LIBSMS_VERSION 1.1

static t_class *sms_class;



// ===========================
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

// ===========================
typedef struct sms 
{
  t_object t_obj;
} t_sms;

// ===========================
static void *sms_new(void)
{
  t_sms *x = (t_sms *)pd_new(sms_class);
  return (x);
}

// ===========================
void sms_setup(void) 
{

        sms_class = class_new(gensym("sms"), sms_new, 0,
                           sizeof(t_sms), CLASS_NOINLET, 0);

        //call object setup routines
        smsbuf_setup();
        smsanal_setup();
        smssynth_tilde_setup();
        smsedit_setup();

        if(SMS_VERSION < REQUIRE_LIBSMS_VERSION)
                post("libsms is too old (version %1.1f, things might not work", SMS_VERSION);
        else
                post("smspd using libsms version %1.1f", SMS_VERSION);

}

// ===========================
/* -_-_-_-_-_-_-_-_-_-_-helper functions -_-_-_-_-_-_-_-_-_-_- */

/* method for opening file in canvas directory. */
t_symbol* getFullPathName( t_symbol *infilename,  t_canvas *canvas)
{
    char nameout[MAXPDSTRING], namebuf[MAXPDSTRING];
    char dirresult[MAXPDSTRING], *nameresult;
    char *dirname;
    char *sym = infilename->s_name;    
    dirname = canvas_getdir(canvas)->s_name;

    if((open_via_path(dirname, sym,"", dirresult, &nameresult, MAXPDSTRING, 0)) < 0) 
            return(NULL);

    namebuf[0] = 0;
    if (*dirresult) //append directory
    {
            strcat(namebuf, dirresult);
            strcat(namebuf, "/");
    }
    strcat(namebuf, nameresult); //append name
    sys_bashfilename(namebuf, nameout);    // cross-platformize slashes
    
    if (0)
    {
            post("(open_via_path):");
            post("dirname: %s", dirname);
            post("filename->s_name: %s", sym);
            post("dirresult: %s", dirresult);
            post("nameresult: %s", nameresult);
            post("namebuf: %s", namebuf);
            post("nameout: %s ", nameout);
    }
    return(gensym( nameout ));
} 

/* get the pointer to an smsbuf that has the given symbol name */
/* todo: try passing a void *x to locate errors within pd */
t_smsbuf* smspd_buffer(t_symbol *bufname)
{
        t_smsbuf *smsbuf = (t_smsbuf *)pd_findbyclass(bufname, smsbuf_class);
        return(smsbuf);
}

/*this function seems to not be copying everything... or else it would be usable
  before copying the buffer from file (it crashes if it is) */
void CopySmsHeader(SMS_Header *pFileHeader, SMS_Header *pBufHeader, char *paramString  )
{
        sms_initHeader (pBufHeader);

        pBufHeader->nFrames = pFileHeader->nFrames;
        pBufHeader->iFormat = pFileHeader->iFormat;
        pBufHeader->iFrameRate = pFileHeader->iFrameRate;
        pBufHeader->iStochasticType = pFileHeader->iStochasticType;
        pBufHeader->nTracks = pFileHeader->nTracks;
        pBufHeader->nStochasticCoeff = pFileHeader->nStochasticCoeff;
        pBufHeader->iSamplingRate = pFileHeader->iSamplingRate;
        pBufHeader->iEnvType = pFileHeader->iEnvType;
        pBufHeader->nEnvCoeff = pFileHeader->nEnvCoeff;
        pBufHeader->iMaxFreq = pFileHeader->iMaxFreq;
   
        pBufHeader->iFrameBSize = sms_frameSizeB(pBufHeader);

        // pBufHeader->nTextCharacters = pFileHeader->nTextCharacters;
        // strcpy(paramString, pFileHeader->pChTextCharacters);
        // pBufHeader->pChTextCharacters = paramString;
        
}

/* ------------------------ smsbuf ----------------------------- */
static void smsbuf_info(t_smsbuf *x) 
{
        if(!x->ready) 
        {
                post("smsbuf_info: not ready");
                return;
        }

        /* todo: tried outlet_anything instead of list, nothing came out. would be nice to avoid the 'list' prefix */

        SETSYMBOL(x->infolist , gensym("envtype"));
        SETFLOAT(x->infolist, x->smsHeader.iEnvType);
        outlet_anything(x->outlet2, gensym("list"), 2 , x->infolist);

        SETSYMBOL(x->infolist , gensym("framerate"));
        SETFLOAT(x->infolist + 1, x->smsHeader.iFrameRate );
        outlet_list(x->outlet2, gensym("list"), 2 , x->infolist);

        SETSYMBOL(x->infolist , gensym("nframes"));
        SETFLOAT(x->infolist + 1, x->nframes );
        outlet_list(x->outlet2, gensym("list"), 2 , x->infolist);

        SETSYMBOL(x->infolist , gensym("ntracks"));
        SETFLOAT(x->infolist + 1, x->smsHeader.nTracks );
        outlet_list(x->outlet2, gensym("list"), 2 , x->infolist);

}

/* static void smsbuf_framerate(t_smsbuf *x)  */
/* { */
/*         if(x->ready)  outlet_float(x->outlet2 , x->smsHeader.iFrameRate); */
/* } */

/* function to open a file and load into the buffer:
 * 1. get fullname in a system independant manner
 * 2. read file header
 * 3. initialize the synthesizer based on header/synth params
 * 4. because we are synthesizing from file, allocate space for sms frames
 */
static void smsbuf_open(t_smsbuf *x, t_symbol *filename)
{
        SMS_Header *pSmsHeader; /* this is necessary only because sms_getHeader needs a double pointer */
        int i;
        t_symbol *fullname;
        x->filename = gensym(filename->s_name);
        fullname = getFullPathName(filename, x->canvas);

        if(fullname == NULL)
        {
                pd_error(x, "smsbuf_open: cannot find file: %s", filename->s_name);
                return;
        }
        else if(x->verbose) post("file: %s", fullname->s_name);
        //check if a file has been opened, close and init if necessary
        if(x->ready)
        {
                if(x->verbose) post("smsbuf_open: re-initializing");
                x->ready=0;
                for( i = 0; i < x->nframes; i++)
                        sms_freeFrame(&x->smsData[i]);
        }

        if (sms_getHeader (fullname->s_name, &pSmsHeader, &x->pSmsFile))
	{
                pd_error(x, "smsbuf_open: %s", sms_errorString());
                return;
        }

        /* allocate memory for nframes of SMS_Data */
        x->nframes = pSmsHeader->nFrames;
        if(x->verbose)post("nframes: %d ", x->nframes);

        /* Buffer the entire file.  For now, I'm doing this the simplest way possible.*/        
        /* will this be faster with one malloc? try once everything is setup */
        x->smsData = calloc(x->nframes, sizeof(SMS_Data));
        for( i = 0; i < x->nframes; i++ )
        {
                sms_allocFrameH (pSmsHeader,  &x->smsData[i]);
                sms_getFrame (x->pSmsFile, pSmsHeader, i, &x->smsData[i]);
        }

        /* copy header to buffer */
        CopySmsHeader( pSmsHeader, &x->smsHeader, x->param_string );

        x->ready = 1;
        if(x->verbose) post("sms file buffered: %s ", filename->s_name );
        /* output number of frames */
        //smsbuf_frames(x); 
        //smsbuf_framerate(x); 
        return;
}

/* TODO: take care of re-allocations */
static void smsbuf_backup(t_smsbuf *x)
{
        if(!x->ready)
        {
                pd_error(x, "smsbuf_backup: buffer is not ready");
                return;
        }
        int i;

        x->smsData2 = calloc(x->nframes, sizeof(SMS_Data));
        for( i = 0; i < x->nframes; i++ )
        {
                sms_allocFrameH (&x->smsHeader,  &x->smsData2[i]);
                sms_copyFrame(&x->smsData2[i],&x->smsData[i]);
        }
        if(x->verbose) post("smsbuf: make a backup of %s", x->bufname->s_name);
}

static void smsbuf_switch(t_smsbuf *x)
{
        if(!x->ready)
        {
                pd_error(x, "smsbuf_switch: buffer is not ready");
                return;
        }
        if(!x->smsData2)
        {
                pd_error(x, "smsbuf_switch: backup buffer is not ready");
                return;
        }
        int i;

        for( i = 0; i < x->nframes; i++ )
                sms_copyFrame(&x->smsData[i],&x->smsData2[i]);

}
static void smsbuf_save(t_smsbuf *x, t_symbol *filename)
{
        if(!x->ready)
        {
                pd_error(x, "smsbuf_save: buffer is not ready");
                return;
        }
        
        int i;
        int iError = 0;
        FILE *pOutputSmsFile;

        /* open file for writing */
        iError = sms_writeHeader(filename->s_name, &x->smsHeader, &pOutputSmsFile);
        if(iError < 0) 
        {
                pd_error(x, "smsbuf_save: %s", sms_errorString());
                return;
        }
        /* write the frames */
        for(i = 0; i < x->nframes; i++)
                sms_writeFrame (pOutputSmsFile, &x->smsHeader, &x->smsData[i]);

        /* save and close file */
        sms_writeFile (pOutputSmsFile, &x->smsHeader);
        if(x->verbose)
                post("smsbuf: wrote %d frames from buffer %s to file %s ", x->nframes,
                     x->bufname->s_name, filename->s_name);
}
static void smsbuf_print(t_smsbuf *x)
{
        if(x->ready)
        {
                post("sms buffer name : %s ", x->bufname->s_name );
                post("sms file : %s ", x->filename->s_name );
                post("original file length: %f seconds ", (float)  x->smsHeader.nFrames /
                     x->smsHeader.iFrameRate);
                post("__header contents__");
                post("Number of Frames: %d", x->smsHeader.nFrames);
                post("Frame rate (Hz) = %d", x->smsHeader.iFrameRate);
                post("Number of tracks = %d", x->smsHeader.nTracks);
                post("Number of stochastic coefficients = %d",
                     x->smsHeader.nStochasticCoeff);
                if(x->smsHeader.iFormat == SMS_FORMAT_H) 
                        post("Format = harmonic");
                else if(x->smsHeader.iFormat == SMS_FORMAT_IH) 
                        post("Format = inharmonic");
                else if(x->smsHeader.iFormat == SMS_FORMAT_HP)
                        post("Format = harmonic with phase");
                else if(x->smsHeader.iFormat == SMS_FORMAT_IHP)
                        post("Format = inharmonic with phase");
                if(x->smsHeader.iStochasticType == SMS_STOC_IFFT) post("Stochastic type = IFFT");
                else if(x->smsHeader.iStochasticType == SMS_STOC_APPROX)
                        post("Stochastic type = line segment magnitude spectrum approximation ");
                else if(x->smsHeader.iStochasticType == SMS_STOC_NONE) post("Stochastic type = none");
                post("Max Frequency: %d", x->smsHeader.iMaxFreq);
                if(x->smsHeader.iEnvType == SMS_ENV_CEP) 
                        post("envelope type = cepstrum");
                else if(x->smsHeader.iEnvType == SMS_ENV_FBINS) 
                        post("envelope type = frequency bins");
                else if(x->smsHeader.iEnvType == SMS_ENV_NONE) 
                        post("envelope type = NONE");
                post("Envelope Coefficients: %d", x->smsHeader.nEnvCoeff);
                post("Original Signal Sampling Rate = %d", x->smsHeader.iSamplingRate);  

                // if (x->smsHeader.nTextCharacters > 0)
                //         post("ANALISIS ARGUMENTS: %s", x->smsHeader.pChTextCharacters);
        }
        else pd_error(x, "smsbuf (%s) not ready", x->bufname->s_name);
}

static void smsbuf_printframe(t_smsbuf *x, float f)
{
        if(f >= x->nframes|| f < 0) return;
               
        int i;
        int frame = (int) f;

        if(x->ready)
        {
                post("----- smsbuf (%s):: frame: %d, timetag: %f -----", x->bufname->s_name, frame, f / x->smsHeader.iFrameRate);
                for(i = 0; i < x->smsHeader.nTracks; i++)
                        if(x->smsData[frame].pFSinAmp[i] > 0.00000001 )
                                post("harmonic %d : %f[%f]", i, x->smsData[frame].pFSinFreq[i],
                                     x->smsData[frame].pFSinAmp[i]);
        }
        else pd_error(x, "smsbuf (%s) not ready", x->bufname->s_name);
}

static void smsbuf_gettrack(t_smsbuf *x, float f)
{
        if(!x->ready)
        {
                pd_error(x, "smsbuf (%s) not ready", x->bufname->s_name);
                return;
        }

        if(f >= x->smsHeader.nTracks || f < 0) return;

        int i;
        float val;
        int notZero = 0;
        int track = (int) f;
//        if(x->oneTrack == NULL) // allocate memory, will have to free and re-init also
        // going to have to do a memset here
        SETSYMBOL(x->oneTrack , gensym("freq"));
        for(i = 0; i < x->smsHeader.nFrames; i++)
        {
                val = x->smsData[i].pFSinFreq[track];
                SETFLOAT(x->oneTrack + i +1, val);
                if(val > 1.) notZero = 1;
        }

        if(notZero)
                outlet_list(x->outlet1, gensym("list"), x->smsHeader.nFrames+1, x->oneTrack);
}


static void smsbuf_verbose(t_smsbuf *x, t_float flag)
{
        if(!flag) x->verbose = 0;
        else
        {
                x->verbose = 1;
                post("smsbuf: verbose messages");
        }
}

static void *smsbuf_new(t_symbol *bufname)
{
        t_smsbuf *x = (t_smsbuf *)pd_new(smsbuf_class);

        x->canvas = canvas_getcurrent();
        x->filename = NULL;
        x->nframes= 0;
        x->ready= 0;
        x->verbose = 0;
        x->smsData2 = NULL;
        //x->oneTrack = NULL;
        x->outlet1 = outlet_new(&x->x_obj,  gensym("list"));
        x->outlet2 = outlet_new(&x->x_obj,  gensym("list"));

        //todo: make a default name if none is given:
        //if (!*s->s_name) s = gensym("delwrite~");
        // ?? do in need to check if bufname already exists? ??
        pd_bind(&x->x_obj.ob_pd, bufname);
        x->bufname = bufname;
        //post("smsbuf using bufname: %s", bufname->s_name);

        sms_init();
    
        return (x);
}

static void smsbuf_free(t_smsbuf *x)
{
        int i;
        //sms_free(); This is a problem in pd, if there is more than one smsbuf

        //smsbuf_dealloc(x);
        if(x->allocated)
        {
                for( i = 0; i < x->nframes; i++)
                        sms_freeFrame(&x->smsData[i]);
                free(x->smsData);
        }

        if(x->smsData2)
        {
                for( i = 0; i < x->nframes; i++)
                        sms_freeFrame(&x->smsData2[i]);
                free(x->smsData2);
        }
        pd_unbind(&x->x_obj.ob_pd, x->bufname);
}

void smsbuf_setup(void)
{       
        smsbuf_class = class_new(gensym("smsbuf"), (t_newmethod)smsbuf_new, (t_method)smsbuf_free, sizeof(t_smsbuf), 0, A_DEFSYM, 0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_open, gensym("open"), A_DEFSYM, 0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_save, gensym("save"), A_DEFSYM, 0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_info, gensym("info"),  0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_print, gensym("print"),  0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_printframe, gensym("printframe"), A_DEFFLOAT, 0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_gettrack, gensym("gettrack"), A_DEFFLOAT, 0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_backup, gensym("backup"),  0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_switch, gensym("switch"),  0);
        class_addmethod(smsbuf_class, (t_method)smsbuf_verbose, gensym("verbose"), A_DEFFLOAT, 0);
}

