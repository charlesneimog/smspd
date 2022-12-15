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
#include "sms.h"
#include "smspd.h"

/* ------------------------ smsedit ----------------------------- */

static t_class *smsedit_class;

typedef struct _smsedit
{
        t_object x_obj; 
        t_smsbuf *smsbuf;
        t_symbol *bufname;
        int verbose;
} t_smsedit;

static void smsedit_buffer(t_smsedit *x, t_symbol *bufname)
{
        if(!*bufname->s_name)
        {
                if(!*x->bufname->s_name)
                {
                        post("... don't have a bufname");
                        return;
                }
                else if(x->verbose) post("using initial bufname: %s", x->bufname->s_name);
        }
        else
        {
                if(x->verbose) post("new bufname: %s", bufname->s_name);
                x->bufname = bufname;
        }
        
        x->smsbuf = smspd_buffer(x->bufname);
        if(!x->smsbuf)
                pd_error(x, "smsedit_new: smsbuf %s was not found", x->bufname->s_name);
        else if(x->verbose)post("smsedit is using buffer: %s", x->bufname->s_name);
}

/* just using this function to check if I have the right pointer to data  */
static void smsedit_printframe(t_smsedit *x, float f)
{
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }

        if(f >= x->smsbuf->nframes || f < 0) return;
               
        int i;
        int frame = (int) f;
                
        post("----- smsbuf (%s):: frame: %d, timetag: %f -----", x->smsbuf->bufname->s_name, frame, f / x->smsbuf->smsHeader.iFrameRate);
        for(i = 0; i < x->smsbuf->smsHeader.nTracks; i++)
                if(x->smsbuf->smsData[frame].pFSinAmp[i] > 0.00000001 )
                        post("harmonic %d : %f[%f]", i, x->smsbuf->smsData[frame].pFSinFreq[i],
                             x->smsbuf->smsData[frame].pFSinAmp[i]);

}

/* individually edit a breakpoint (a deterministic sinusodal track at one point in time)
 * op: type of operation (set, add, scale)
 * co: component (freq, amp)
 * frame: desired frame number
 * track: desired track number
 * value: a value to edit the breakpoint based on the desired operation
 */
static void smsedit_bp( t_smsedit *x, t_symbol *op, t_symbol *co, t_float frame, t_float track, t_float value)
{
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }
        int iFrame = (int) frame;
        int iTrack = (int) track;

        if(iFrame >= x->smsbuf->nframes || iFrame < 0)
        {
                post("smsedit_bp: frame out of range");
                return;
        }
        if(iTrack >= x->smsbuf->smsData[iFrame].nTracks || iTrack < 0)
        {
                post("smsedit_bp: track out of range");
                return;
        }

        SMS_Data *pFrame = &x->smsbuf->smsData[iFrame];
        
        if(!strcmp(op->s_name,"set"))
        {
                if(!strcmp(co->s_name,"freq"))
                {
                        pFrame->pFSinFreq[iTrack] = value;
                        if(x->verbose)post("smsedit: setting track %d:%d to %.3f hertz", iFrame, iTrack, value);
                } 
                else if(!strcmp(co->s_name,"amp"))
                {
                        pFrame->pFSinAmp[iTrack] = value;
                        if(x->verbose)post("smsedit: setting track %d:%d to %.3f dB", iFrame, iTrack, value);
                } 
                else if(x->verbose) post("smsbuf_bp: unknown component: %s", op->s_name);

        }
        if(!strcmp(op->s_name,"add"))
        {
                if(!strcmp(co->s_name,"freq"))
                {
                        pFrame->pFSinFreq[iTrack] += value;
                        if(x->verbose)post("smsedit: adding to track %d:%d to %.3f hertz", iFrame, iTrack, value);
                } 
                else if(!strcmp(co->s_name,"amp"))
                {
                        pFrame->pFSinAmp[iTrack] += value;
                        if(x->verbose)post("smsedit: adding to %d:%d to %.3f dB", iFrame, iTrack, value);
                } 
                else if(x->verbose) post("smsbuf_bp: unknown component: %s", op->s_name);

        }
        if(!strcmp(op->s_name,"add"))
        {
                if(!strcmp(co->s_name,"freq"))
                {
                        pFrame->pFSinFreq[iTrack] += value;
                        if(x->verbose)post("smsedit: adding %.3f hertz to track %d:%d", value, iFrame, iTrack);
                } 
                else if(!strcmp(co->s_name,"amp"))
                {
                        pFrame->pFSinAmp[iTrack] += value;
                        if(x->verbose)post("smsedit: adding %.3f dB to %d:%d", value, iFrame, iTrack);
                } 
                else if(x->verbose) post("smsbuf_bp: unknown component: %s", op->s_name);

        }
        if(!strcmp(op->s_name,"scale"))
        {
                if(!strcmp(co->s_name,"freq"))
                {
                        pFrame->pFSinFreq[iTrack] *= value;
                        if(x->verbose)post("smsedit: scaling track %d:%d by %.3f hertz", iFrame, iTrack, value);
                } 
                else if(!strcmp(co->s_name,"amp"))
                {
                        pFrame->pFSinAmp[iTrack] *= value;
                        if(x->verbose)post("smsedit: scaling %d:%d by %.3f dB", iFrame, iTrack, value);
                } 
                else if(x->verbose) post("smsbuf_bp: unknown component: %s", op->s_name);

        }
        else if(x->verbose) post("smsbuf_bp: unknown operation: %s", op->s_name);
}

/* edit all even breakpoints in one frame arguments:
 * op: type of operation 
 *     - transpose: value to transpose the harmonics by
 *     - ampmod:  add this value to the harmonic's amplitude
 *     - ampdev:  add this random value to the harmonic's frequencies
 *     - freqdev:  add this random value to the harmonic's amplitudes
 * frame: desired frame number
 * value: a value to edit the breakpoint based on the desired operation
 */
static void smsedit_evens( t_smsedit *x, t_symbol *op, t_float frame, t_float value)
{
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }
        int iFrame = (int) frame;
        int i;
        float transpose;

        if(iFrame >= x->smsbuf->nframes || iFrame < 0)
        {
                post("smsedit_bp: frame out of range");
                return;
        }
        
        SMS_Data *pFrame = &x->smsbuf->smsData[iFrame];
        int nTracks = pFrame->nTracks;

        if(!strcmp(op->s_name,"transpose"))
        {
                transpose = sms_scalarTempered( value );
                for( i = 0; (i*2) <  nTracks; i++)
                        if(pFrame->pFSinAmp[i*2] <= 0.0000001) pFrame->pFSinFreq[i*2] = pFrame->pFSinFreq[i*2] * transpose;
                
                if(x->verbose)post("smsedit: scaling even tracks in frame %d by a factor of %.3f (midi)",
                                   iFrame, value);
        }
        else if(!strcmp(op->s_name,"ampmod"))
        {
                for( i = 0; (i*2) <  nTracks; i++)
                        if(pFrame->pFSinAmp[i*2] <= 0.0000001) pFrame->pFSinAmp[i*2] += value;
                
                if(x->verbose)post("smsedit: scaling the amplitude of even tracks in frame %d by a factor of %.3f",
                                   iFrame, value);
        }
        else if(!strcmp(op->s_name,"ampdev"))
        {
                for( i = 0; (i*2) <  nTracks; i++)
                        if(pFrame->pFSinAmp[i*2] <= 0.0000001) pFrame->pFSinAmp[i*2] += value * sms_random();

                if(x->verbose)post("smsedit: adding random amplitudes to even tracks in frame %d of factor +/-%.3f",
                                   iFrame, value);
        }
        else if(x->verbose) post("smsbuf_evens: unknown operation: %s", op->s_name);

}

/* edit every other breakpoint in one frame arguments:
 * op: type of operation 
 *     - transpose: value to transpose the harmonics by
 *     - ampmod:  add this value to the harmonic's amplitude
 *     - ampdev:  add this random value to the harmonic's frequencies
 *     - freqdev:  add this random value to the harmonic's amplitudes
 * frame: desired frame number
 * track: first track to edit
 * value: a value to edit the breakpoint based on the desired operation
 */
static void smsedit_everyother( t_smsedit *x, t_symbol *op, t_float frame, t_float track, t_float value)
{
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }
        int iFrame = (int) frame;
        int iTrack = (int) track;
        int i;
        float transpose;

        if(iFrame >= x->smsbuf->nframes || iFrame < 0)
        {
                post("smsedit_everyother: frame out of range");
                return;
        }
        if(iTrack >= x->smsbuf->smsData[iFrame].nTracks || iTrack < 0)
        {
                post("smsedit_everyother: track out of range");
                return;
        }
        
        SMS_Data *pFrame = &x->smsbuf->smsData[iFrame];
        int nTracks = pFrame->nTracks;

        if(!strcmp(op->s_name,"transpose"))
        {
                transpose = sms_scalarTempered( value );
                for( i = iTrack; (i*2) <  nTracks; i++)
                        pFrame->pFSinFreq[i*2] = pFrame->pFSinFreq[i*2] * transpose;
                
                if(x->verbose)post("smsedit: scaling every other track (starting at %d) in frame %d by a factor of %.3f (midi)",
                                   iTrack, iFrame, value);
        }
        else if(!strcmp(op->s_name,"ampmod"))
        {
                for( i = iTrack; (i*2) <  nTracks; i++)
                        pFrame->pFSinAmp[i*2] += value;
                
                if(x->verbose)post("smsedit: scaling the amplitude of every other track (starting at %d) in frame %d by a factor of %.3f",
                                   iTrack, iFrame, value);
        }
        else if(!strcmp(op->s_name,"ampdev"))
        {
                for( i = iTrack; (i*2) <  nTracks; i++)
                        pFrame->pFSinAmp[i*2] += value * sms_random();

                if(x->verbose)post("smsedit: adding random amplitudes to every other track (starting at %d) in frame %d of factor +/-%.3f",
                                   iTrack, iFrame, value);
        }
        else if(x->verbose) post("smsbuf_evens: unknown operation: %s", op->s_name);

}

/* edit a range of breakpoints in one frame arguments:
 * op: type of operation 
 *     - transpose: value to transpose the harmonics by
 *     - ampmod:  add this value to the harmonic's amplitude
 *     - ampdev:  add this random value to the harmonic's frequencies
 *     - freqdev:  add this random value to the harmonic's amplitudes
 * frame: desired frame number
 * firstTrack: first track to edit
 * lastTrack: last track to edit
 * value: a value to edit the breakpoint based on the desired operation
 */
static void smsedit_range( t_smsedit *x, t_symbol *op, t_float frame, t_float firstTrack, t_float lastTrack, t_float value)
{
        int iFrame = frame;
        int iFirstTrack = (int) firstTrack;
        int iLastTrack = (int) lastTrack;
        int i;
        float transpose;
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }
       if(iFirstTrack > iLastTrack || iLastTrack >= x->smsbuf->smsData[iFrame].nTracks || 
           iLastTrack < 0)
        {
                pd_error(x, "smsedit_range: track selection out of range");
                return;
        }
        
        SMS_Data *pFrame = &x->smsbuf->smsData[iFrame];
        //int nTracks = pFrame->nTracks;

        if(!strcmp(op->s_name,"transpose"))
        {
                transpose = sms_scalarTempered( value );
                for( i = iFirstTrack; i <=  iLastTrack; i++)
                        pFrame->pFSinFreq[i] = pFrame->pFSinFreq[i] * transpose;
        }
        else if(!strcmp(op->s_name,"ampmod"))
        {
                for( i = iFirstTrack; i <=  iLastTrack; i++)
                        pFrame->pFSinAmp[i] += value;
        }
        else if(!strcmp(op->s_name,"ampdev"))
        {
                for( i = iFirstTrack; i <=  iLastTrack; i++)
                        pFrame->pFSinAmp[i] += value * sms_random();
        }
        else if(x->verbose) post("smsbuf_evens: unknown operation: %s", op->s_name);

}

static void smsedit_converge( t_smsedit *x, t_float frame, t_float w)
{
        if(!x->smsbuf)
        {
                pd_error(x, "smsedit: no pointer to an smsbuf");
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsedit: smsbuf is not ready");
                return;
        }
        if(!x->smsbuf->smsData2)
        {
                pd_error(x, "smsedit: smsbuf does not have a backup");
                return;
        }
        if(w <= 0) w =0;
        if(w >= 1) w = 1;

        int iFrame = (int) frame;
        int i;

        if(iFrame >= x->smsbuf->nframes || iFrame < 0)
        {
                post("smsedit_converge: frame out of range");
                return;
        }
        
        SMS_Data *pFrame = &x->smsbuf->smsData[iFrame];
        SMS_Data *pFrameBackup = &x->smsbuf->smsData2[iFrame];
        int nTracks = pFrame->nTracks;
        float u = 1 - w;

        for(i = 0; i < nTracks; i++)
        {
                pFrame->pFSinFreq[i] = pFrame->pFSinFreq[i] * u + pFrameBackup->pFSinFreq[i] * w;
                pFrame->pFSinAmp[i] = pFrame->pFSinAmp[i] * u + pFrameBackup->pFSinAmp[i] * w;
        }
}

static void smsedit_verbose(t_smsedit *x, t_float flag)
{
        if(!flag) x->verbose = 0;
        else
        {
                x->verbose = 1;
                post("smsedit: verbose messages");
        }
}
/* creator function */
static void *smsedit_new(t_symbol *s, int argcount, t_atom *argvec)
{
        (void)*s;
        t_smsedit *x = (t_smsedit *)pd_new(smsedit_class);
        int i;
        x->smsbuf = NULL;
        x->verbose = 0;

        for (i = 0; i < argcount; i++)
        {
                if (argvec[i].a_type == A_SYMBOL)
                {
                        x->bufname = argvec[i].a_w.w_symbol;
                        x->smsbuf = smspd_buffer(x->bufname);
                        if(!x->smsbuf)
                                pd_error(x, "smsedit_new: smsbuf %s was not found", x->bufname->s_name);
                }
        }

        return (x);
}

static void smsedit_free(t_smsedit *x)
{

}

void smsedit_setup(void)
{
        smsedit_class = class_new(gensym("smsedit"), (t_newmethod)smsedit_new, 
                                       (t_method)smsedit_free, sizeof(t_smsedit), 0, A_GIMME, 0);

        class_addmethod(smsedit_class, (t_method)smsedit_buffer, gensym("buffer"), A_DEFSYM, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_printframe, gensym("printframe"), A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_bp, gensym("bp"), A_DEFSYM, A_DEFSYM,
                        A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_evens, gensym("evens"), A_DEFSYM, A_DEFFLOAT,
                        A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_everyother, gensym("everyother"), A_DEFSYM,
                        A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_range, gensym("range"), A_DEFSYM,
                        A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_converge, gensym("converge"), A_DEFFLOAT,
                        A_DEFFLOAT, 0);
        class_addmethod(smsedit_class, (t_method)smsedit_verbose, gensym("verbose"), A_DEFFLOAT, 0);
}
