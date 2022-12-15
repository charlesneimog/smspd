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

/* ------------------------ smssynth~ ----------------------------- */

#define SOURCE_FLOAT 1
#define SOURCE_SIGNAL 2

static t_class *smssynth_class;

typedef struct _smssynth
{
        t_object x_obj; 
        t_canvas *canvas;
        t_symbol *bufname, *envbufname;
        t_smsbuf *smsbuf, *envbuf;
        t_int i_frame, i_frameSource, synthBufPos;
        t_float *synthBuf;
        t_float f; /* dummy for signal inlet */
        t_float transpose, stocgain;
        SMS_SynthParams synthParams;
        SMS_Data smsFrame;
        t_int verbose;
        t_int ready;
} t_smssynth;

static void smssynth_verbose(t_smssynth *x, t_float flag)
{
        if(!flag) x->verbose = 0;
        else
        {
                x->verbose = 1;
                post("smsbuf: verbose messages");
        }
}

static void smssynth_buffer(t_smssynth *x, t_symbol *bufname)
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

        /* check if a file has been opened, if so re-init */
        if(x->ready)
        {
                post("smssynth_buffer: re-initializing synth");
                x->ready = 0;
                sms_freeSynth(&x->synthParams);
                sms_freeFrame(&x->smsFrame);
        }

        x->smsbuf =
        (t_smsbuf *)pd_findbyclass(x->bufname, smsbuf_class);

        if(!x->smsbuf)
        {
                pd_error(x, "smssynth~: %s was not found", x->bufname->s_name);
                return;
        }
        if(!x->smsbuf->ready)
        {
                pd_error(x, "smsbuf not ready");
                return;
        }
        else if(x->verbose)
                post("smssynth: [smsbuf %s] was successfully found ", x->bufname->s_name);

        sms_initSynth(&x->smsbuf->smsHeader, &x->synthParams);

	/* setup interpolated frame */
  	sms_allocFrameH (&x->smsbuf->smsHeader, &x->smsFrame);

        x->ready = 1;
        if(x->verbose) post("smssynth is ready for synthesis");
}

static void smssynth_sinenv(t_smssynth *x, t_symbol *bufname)
{
        if(!x->ready)
        {
                post("smssynth %s error: buffer not ready yet.", x->bufname->s_name);
                return;
        }

        if(x->verbose) post("envelope bufname: %s", bufname->s_name);
        x->envbufname = bufname;

         x->envbuf =
                 (t_smsbuf *)pd_findbyclass(x->envbufname, smsbuf_class);

        if(!x->envbuf)
        {
                pd_error(x, "smssynth~: %s (for envbuf) was not found", x->envbufname->s_name);
                return;
        }
        if(!x->envbuf->ready)
        {
                pd_error(x, "smsbuf for envelope is not ready");
                return;
        }
        else if(x->verbose)
                post("smssynth: using envelope found in [smsbuf %s]", x->envbufname->s_name);

        /* the following two checks will later be overcome with interpolation to make one fit the other */
        if(x->smsbuf->smsHeader.iMaxFreq != x->envbuf->smsHeader.iMaxFreq)
        {
                pd_error(x, "smssynth~: sms buffer max frequency (%d) does not match envelope max frequency (%d)",
                         x->smsbuf->smsHeader.iMaxFreq, x->envbuf->smsHeader.iMaxFreq);
                        return;
        }
        if(x->smsbuf->smsHeader.nEnvCoeff != x->envbuf->smsHeader.nEnvCoeff)
        {
                pd_error(x, "smssynth~: sms buffer envelope coefficients (%d) do not match target envelope coefficients (%d)",
                         x->smsbuf->smsHeader.nEnvCoeff, x->envbuf->smsHeader.nEnvCoeff);
                        return;
        }
        sms_initModify(&x->smsbuf->smsHeader, &x->synthParams.modParams);
        if(x->verbose)
        {
                post("smssynth: set envelope max frequency to %d, sizeEnv to %d",
                     x->synthParams.modParams.maxFreq, x->synthParams.modParams.sizeSinEnv);
        }
        x->synthParams.modParams.doSinEnv = 1;
}

static void smssynth_sinenvframe(t_smssynth *x, t_float frame, t_float interp)
{
        if(!x->ready)
        {
                post("smssynth %s error: buffer not ready yet.", x->bufname->s_name);
                return;
        }
        if(!x->envbuf)
        {
                pd_error(x, "smssynth~: envbuf is not set");
                return;
        }
        if(!x->envbuf->ready)
        {
                pd_error(x, "smssynth~: %s envbuf is not ready", x->envbufname->s_name);
                return;
        }
        int iFrame = (int) frame; // todo: try interpolating frame envelopes
        if(iFrame < 0) 
                iFrame = 0;
        else if(iFrame > x->envbuf->smsHeader.nFrames) 
                iFrame = x->envbuf->smsHeader.nFrames; 
        x->synthParams.modParams.sinEnvInterp = interp;
        memcpy(x->synthParams.modParams.sinEnv, x->envbuf->smsData[iFrame].pSpecEnv, 
               sizeof(sfloat) * x->synthParams.modParams.sizeSinEnv);
}

/* the signal in is not currently used; is there a benifit to control the synthesis by signal rate? */
static t_int *smssynth_perform(t_int *w)
{
        t_smssynth *x = (t_smssynth *)(w[1]);
        //t_sample *in = (t_float *)(w[2]);
        t_sample *out = (t_float *)(w[3]);
        int n = (int)(w[4]);
        
        if(x->ready && x->smsbuf->ready)        
        {
                int i, iLeftFrame, iRightFrame;
                int nFrames = x->smsbuf->nframes;
                if(x->synthBufPos >= x->synthParams.sizeHop)
                {
                        if(x->f >= nFrames)
                                x->f = nFrames -1;
                        if(x->f < 0) x->f = 0;
                
                        iLeftFrame = MIN (nFrames - 1, floor (x->f)); 
                        iRightFrame = (iLeftFrame < nFrames - 2)
                                ? (1+ iLeftFrame) : iLeftFrame;

                        sms_interpolateFrames (&x->smsbuf->smsData[iLeftFrame], &x->smsbuf->smsData[iRightFrame],
                                                &x->smsFrame, x->f - iLeftFrame);
                        sms_modify(&x->smsFrame, &x->synthParams.modParams); 
                        sms_synthesize (&x->smsFrame, x->synthBuf, &x->synthParams);
                        x->synthBufPos = 0; // samples are loaded
                }
                // send out samples in pd blocks
                //todo: check when blocksize is larger than hopsize... will probably crash
                for (i = 0; i < n; i++, x->synthBufPos++)
                        out[i] = x->synthBuf[x->synthBufPos];
        }
        else
        {
                while(n--) *out++ = 0;
                /* if the buffer is turned off for some reason, turn off the synth too. It will need to be re-initialized. */
                if(x->ready && !x->smsbuf->ready)
                {
                        if(x->verbose)
                        {
                                post("smssynth_perform: x->ready: %d, x->smsbuf->ready: %d",
                                     x->ready, x->smsbuf->ready);
                                post("smssynth_perform: re-initializing synth");

                        }
                        x->ready = 0;
                        sms_freeSynth(&x->synthParams);
                        sms_freeFrame(&x->smsFrame);
                }
        }

        return (w+5);
}

static void smssynth_sizehop(t_smssynth *x, t_float f)
{
        post("doesn't work yet.");
        return;
        //pthread_t childthread;
        int sizehop = sms_power2((int) f);
        if(x->verbose) post("smssynth: setting sizehop to %d", sizehop);
        /* check if a file has been opened, if so re-init */
        if(x->ready)
        {
                if(x->verbose)post("smssynth_sizehop: re-initializing synth");
                x->ready = 0;
                x->synthParams.sizeHop = sizehop; /* should set synthBufPos here too? otherwise might go out of bounds */
        }
        else x->synthParams.sizeHop = x->synthBufPos = sizehop;

}

static void smssynth_transpose(t_smssynth *x, t_float f)
{
        x->synthParams.modParams.transpose = f;
        if(x->verbose) post("transposition: %f", x->synthParams.modParams.transpose);
}

/* static void smssynth_keepenv(t_smssynth *x, t_float f) */
/* { */
/*         int iKeepEnv = (int) f; */
/*         if(!x->ready) */
/*         { */
/*                 post("smssynth %s error: buffer not ready yet.", x->bufname->s_name); */
/*                 return; */
/*         } */

/*         if(iKeepEnv && x->smsbuf->smsHeader.iEnvType != SMS_ENV_FBINS) */
/*         { */
/*                 post("smssynth %s error: buffer does not contain a spectral envelope.", x->bufname->s_name); */
/*                 post("smsHeader.iEnvType: %d", x->smsbuf->smsHeader.iEnvType); */
/*                 x->synthParams.modParams.envType = SMS_MTYPE_NONE; */
/*         } */
/*         else if(iKeepEnv) */
/*                         x->synthParams.modParams.envType = SMS_MTYPE_KEEP_ENV; */
/*         if(x->verbose) */
/*                 post("smssynth %s modify type: %d", x->bufname->s_name, x->synthParams.modParams.envType); */
/* } */

static void smssynth_stocgain(t_smssynth *x, t_float f)
{
        x->synthParams.modParams.resGain = f;
        if(x->verbose) post("stochastic gain: %fx", x->synthParams.modParams.resGain);
}

static void smssynth_synthtype(t_smssynth *x, t_float f)
{
        x->synthParams.iSynthesisType = (int) f;
        if(x->verbose)
        {
                switch (x->synthParams.iSynthesisType)
                {
                case SMS_STYPE_ALL:
                        post("synthesis type: %d, all", x->synthParams.iSynthesisType);
                        break;
                case SMS_STYPE_DET:
                        post("synthesis type: %d, only deterministic", x->synthParams.iSynthesisType);
                        break;
                case SMS_STYPE_STOC:
                        post("synthesis type: %d, only stochastic", x->synthParams.iSynthesisType);
                        break;
                }
        }
}


static void smssynth_info(t_smssynth *x)
{
        post("smssynth~ %s info:", x->bufname->s_name);
        post("samplingrate: %d  ", x->synthParams.iSamplingRate);
        if(x->synthParams.iSynthesisType == SMS_STYPE_ALL) 
                post("synthesis type: all ");
        else if(x->synthParams.iSynthesisType == SMS_STYPE_DET) 
                post("synthesis type: deterministic only ");
        else if(x->synthParams.iSynthesisType == SMS_STYPE_STOC) 
                post("synthesis type: stochastic only ");
        if(x->synthParams.iDetSynthType == SMS_DET_IFFT) 
                post("deteministic synthesis method: ifft ");
        else if(x->synthParams.iDetSynthType == SMS_DET_SIN) 
                post("deteministic synthesis method: oscillator bank ");
        post("sizeHop: %d ", x->synthParams.sizeHop);
        /* print modParams below */
        SMS_ModifyParams *p = &x->synthParams.modParams;
        post("---- modParams ----");
        post("ready: %d", p->ready);
        // blah don't have the time right now
}

static void smssynth_dsp(t_smssynth *x, t_signal **sp)
{
        x->synthParams.iSamplingRate =  sp[0]->s_sr;
        //need x and 2 vectors (in/out), and lastly the vector size:
        dsp_add(smssynth_perform, 4, x,  sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void *smssynth_new(t_symbol *bufname)
{
        t_smssynth *x = (t_smssynth *)pd_new(smssynth_class);
        outlet_new(&x->x_obj, gensym("signal"));
        
        x->bufname = bufname;
        x->verbose = 0;
        x->smsbuf = NULL;
        x->ready = 0;
 
        sms_init(); /* probably already accomplished by smsbuf, but if not.. */
        sms_initSynthParams(&x->synthParams); /* set sane synthesis values */
        x->synthParams.modParams.doResGain = 1; /* turn residual gain scaling back on */
        x->synthParams.modParams.doTranspose = 1; /* turn transpose back on */
        x->synthParams.modParams.maxFreq = 0; /* updated in sms_initSynth() */
        //x->synthParams.modParams.envInterp = 0.; /* updated in sms_initSynth() */

        x->synthBuf = (t_float *) calloc(x->synthParams.sizeHop, sizeof(t_float));
        
        return (x);
}

static void smssynth_free(t_smssynth *x)
{
        if(x->smsbuf && x->smsbuf->ready) 
        {
                free(x->synthBuf);
                sms_freeSynth(&x->synthParams);
                sms_freeFrame(&x->smsFrame);
        }
}
void smssynth_tilde_setup(void)
{
        smssynth_class = class_new(gensym("smssynth~"), (t_newmethod)smssynth_new, 
                                       (t_method)smssynth_free, sizeof(t_smssynth), 0, A_DEFSYM, 0);
        CLASS_MAINSIGNALIN(smssynth_class, t_smssynth, f);
        class_addmethod(smssynth_class, (t_method)smssynth_dsp, gensym("dsp"), 0);
        class_addmethod(smssynth_class, (t_method)smssynth_buffer, gensym("buffer"), A_DEFSYM, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_info, gensym("info"),  0);
        class_addmethod(smssynth_class, (t_method)smssynth_verbose, gensym("verbose"), A_DEFFLOAT, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_sizehop, gensym("sizehop"), A_DEFFLOAT, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_transpose, gensym("transpose"), A_DEFFLOAT, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_stocgain, gensym("stocgain"), A_DEFFLOAT, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_synthtype, gensym("synthtype"), A_DEFFLOAT, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_sinenv, gensym("sinenv"), A_DEFSYM, 0);
        class_addmethod(smssynth_class, (t_method)smssynth_sinenvframe, gensym("sinenvframe"),
                        A_DEFFLOAT, A_DEFFLOAT, 0);
}

