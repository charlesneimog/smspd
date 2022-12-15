/* 
 * Copyright (c) 2008 MUSIC TECHNOLOGY GROUP (MTG)
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

/*! \file analysis.c
 * \brief main sms analysis routines
 * 
 * the analysis routine here calls all necessary functions to perform the complete
 * SMS analysis, once the desired analysis parameters are set in SMS_AnalParams. 
 */

#include "sms.h"

/*! \brief  maximum size for magnitude spectrum */
#define SMS_MAX_SPEC 8192  

/*! \brief compute spectrum, find peaks, and fundamental of one frame
 *
 * This is the main core of analysis calls
 *
 * \param iCurrentFrame          frame number to be computed
 * \param pAnalParams     structure of analysis parameters
 * \param fRefFundamental      reference fundamental 
 */
void sms_analyzeFrame (int iCurrentFrame, SMS_AnalParams *pAnalParams, sfloat fRefFundamental )
{
	int i, iFrame;
	static int sizeWindowStatic = 0;
        static int sizeMag = 0;
	static sfloat pFMagSpectrum[SMS_MAX_SPEC];
	static sfloat pFPhaSpectrum[SMS_MAX_SPEC];
	static sfloat pFSpectrumWindow[SMS_MAX_SPEC];
        SMS_AnalFrame *pCurrentFrame = pAnalParams->ppFrames[iCurrentFrame];
        int sizeWindow =  pCurrentFrame->iFrameSize;
	int iSoundLoc = pCurrentFrame->iFrameSample -((pCurrentFrame->iFrameSize + 1) >> 1) + 1;
	sfloat *pFData =
		&(pAnalParams->soundBuffer.pFBuffer[iSoundLoc - pAnalParams->soundBuffer.iMarker]);

        /*if window size has changed, update the window and sizemag*/
	if (sizeWindowStatic != sizeWindow)
        {
                sizeMag = sms_power2(sizeWindow);
                sms_getWindow(sizeWindow, pFSpectrumWindow, pAnalParams->iWindowType);
                sms_scaleWindow(sizeWindow, pFSpectrumWindow);
                sizeWindowStatic = sizeWindow;
        }

	/* compute the magnitude and (zero-windowed) phase spectra */
        sms_spectrum(sizeWindow, pFData, pFSpectrumWindow, sizeMag,
	                   pFMagSpectrum, pFPhaSpectrum);

        /* convert magnitude spectra to dB */
        sms_arrayMagToDB(sizeMag, pFMagSpectrum);

	/* find the prominent peaks */
	//pAnalParams->ppFrames[iCurrentFrame]->nPeaks =
	pCurrentFrame->nPeaks = sms_detectPeaks (sizeMag,
                                 pFMagSpectrum,
                                 pFPhaSpectrum,
                                 pCurrentFrame->pSpectralPeaks,
                                 &pAnalParams->peakParams);

	/* find a reference harmonic */
	if (pCurrentFrame->nPeaks > 0 &&
	    (pAnalParams->iFormat == SMS_FORMAT_H ||
	    pAnalParams->iFormat == SMS_FORMAT_HP))
		sms_harmDetection (pCurrentFrame, fRefFundamental, &pAnalParams->peakParams);
        return;
}

/*! \brief re-analyze the previous frames if necessary
 *
 * \todo explain when this is necessary
 *
 * \param iCurrentFrame             current frame number
 * \param pAnalParams              structure with analysis parameters
 * \return 1 if frames are good, -1 if analysis is necessary
 * \todo is the return value info correct? Why isn't it used in sms_analyze?
 */
static int ReAnalyzeFrame (int iCurrentFrame, SMS_AnalParams *pAnalParams)
{
        sfloat fFund, fLastFund, fDev;
	int iNewFrameSize, i;
	sfloat fAvgDeviation = sms_fundDeviation(pAnalParams, iCurrentFrame);
        int iFirstFrame = iCurrentFrame - SMS_MIN_GOOD_FRAMES;

/*         fprintf(stdout, "Frame %d reAnalyze: Freq. deviation %f\n", */
/*                 pAnalParams->ppFrames[iCurrentFrame]->iFrameNum, fAvgDeviation); */

        /*! \todo mae this a < 0 check, but first make sure sms_fundDeviation does not
          return values below zero */
	if (fAvgDeviation == -1)
		return (-1);
  
	/* if the last SMS_MIN_GOOD_FRAMES are stable look before them */
	/*  and recompute the frames that are not stable */
	if (fAvgDeviation <= SMS_MAX_DEVIATION)
		for (i = 0; i < SMS_ANAL_DELAY; i++)
		{
			if (pAnalParams->ppFrames[iFirstFrame - i]->iFrameNum <= 0 ||
			    pAnalParams->ppFrames[iFirstFrame - i]->iStatus == SMS_FRAME_RECOMPUTED)
				return(-1);
			fFund = pAnalParams->ppFrames[iFirstFrame - i]->fFundamental;
			fLastFund = pAnalParams->ppFrames[iFirstFrame - i + 1]->fFundamental;
			fDev = fabs (fFund - fLastFund) / fLastFund;
			iNewFrameSize = ((pAnalParams->iSamplingRate / fLastFund) *
				pAnalParams->fSizeWindow/2) * 2 + 1;
	
			if (fFund <= 0 || fDev > .2 ||
			    fabs ((double)(pAnalParams->ppFrames[iFirstFrame - i]->iFrameSize -
			          iNewFrameSize)) /
			    iNewFrameSize >= .2)
			{
				pAnalParams->ppFrames[iFirstFrame - i]->iFrameSize = iNewFrameSize;
				pAnalParams->ppFrames[iFirstFrame - i]->iStatus = SMS_FRAME_READY;
	    
/*                                 fprintf(stdout, "re-analyzing frame %d\n", */
/*                                         pAnalParams->ppFrames[iFirstFrame - i]->iFrameNum); */
	    
				/* recompute frame */
				sms_analyzeFrame (iFirstFrame - i, pAnalParams, fLastFund);
				pAnalParams->ppFrames[iFirstFrame - i]->iStatus = SMS_FRAME_RECOMPUTED;
	    
				if (fabs(pAnalParams->ppFrames[iFirstFrame - i]->fFundamental - fLastFund) /
				    fLastFund >= .2)
				return(-1);
			}
		}
	return (1);
}

/*! \brief main function to perform the SMS analysis on a single frame
 *
 * The input is a section of the sound, the output is the SMS data
 *
 * \param sizeWaveform	     size of input waveform data
 * \param pWaveform	     pointer to input waveform data
 * \param pSmsData          pointer to output SMS data
 * \param pAnalParams   pointer to analysis parameters
 * \return \todo sort out return meanings
 */
int sms_analyze (int sizeWaveform, sfloat *pWaveform, SMS_Data *pSmsData, SMS_AnalParams *pAnalParams)
{
	static int sizeWindow = 0;      /* size of current analysis window */ //RTE ?: shouldn't this just be initilalized outside?

	int iCurrentFrame = pAnalParams->iMaxDelayFrames - 1;  /* frame # of current frame */
	int i, iError, iExtraSamples;              /* samples used for next analysis frame */
	sfloat fRefFundamental = 0;   /* reference fundamental for current frame */
        SMS_AnalFrame *pTmpAnalFrame;

	/* clear SMS output */
	sms_clearFrame (pSmsData);
  
	/* set initial analysis-window size */
	if (sizeWindow == 0)
		sizeWindow = pAnalParams->iDefaultSizeWindow;
  
	/* fill the input sound buffer and perform pre-emphasis */
	if (sizeWaveform > 0)
		sms_fillSoundBuffer (sizeWaveform, pWaveform, pAnalParams);
    
	/* move analysis data one frame back */
	pTmpAnalFrame = pAnalParams->ppFrames[0];
	for(i = 1; i < pAnalParams->iMaxDelayFrames; i++)
		pAnalParams->ppFrames[i-1] = pAnalParams->ppFrames[i];
	pAnalParams->ppFrames[pAnalParams->iMaxDelayFrames-1] = pTmpAnalFrame;


	/* initialize the current frame */
	sms_initFrame (iCurrentFrame, pAnalParams, sizeWindow);
        if(sms_errorCheck())
        {
                printf("error in init frame: %s \n", sms_errorString());
                return(-1);
        }
  
	/* if right data in the sound buffer do analysis */
	if (pAnalParams->ppFrames[iCurrentFrame]->iStatus == SMS_FRAME_READY)
	{
		sfloat fAvgDev = sms_fundDeviation( pAnalParams, iCurrentFrame - 1);

		/* if single note use the default fundamental as reference */
		if (pAnalParams->iSoundType == SMS_SOUND_TYPE_NOTE)
			fRefFundamental = pAnalParams->fDefaultFundamental;
		/* if sound is stable use the last fundamental as a reference */
		else if (fAvgDev != -1 && fAvgDev <= SMS_MAX_DEVIATION)
			fRefFundamental = pAnalParams->ppFrames[iCurrentFrame - 1]->fFundamental;
		else
			fRefFundamental = 0;

		/* compute spectrum, find peaks, and find fundamental of frame */
		sms_analyzeFrame (iCurrentFrame, pAnalParams, fRefFundamental);

		/* set the size of the next analysis window */
		if (pAnalParams->ppFrames[iCurrentFrame]->fFundamental > 0 &&
		    pAnalParams->iSoundType != SMS_SOUND_TYPE_NOTE)
			sizeWindow = sms_sizeNextWindow (iCurrentFrame, pAnalParams);
      
		/* figure out how much needs to be read next time */
		iExtraSamples =
			(pAnalParams->soundBuffer.iMarker + pAnalParams->soundBuffer.sizeBuffer) -
			(pAnalParams->ppFrames[iCurrentFrame]->iFrameSample + pAnalParams->sizeHop);
/*                 printf("iMarker: %d, sizeBuffer: %d, iFrameSample %d, sizeHop: %d \n",  */
/*                        pAnalParams->soundBuffer.iMarker, pAnalParams->soundBuffer.sizeBuffer, */
/*                        pAnalParams->ppFrames[iCurrentFrame]->iFrameSample, pAnalParams->sizeHop); */

		pAnalParams->sizeNextRead = MAX (0, (sizeWindow+1)/2 - iExtraSamples);
/*                 printf("pAnalParams -> sizeNextRead: %d, sizeWindow: %d, iExtraSamples: %d \n", */
/*                        pAnalParams->sizeNextRead, sizeWindow, iExtraSamples); */
		/* check again the previous frames and recompute if necessary */
                /*! \todo when deviation is really off, this function returns -1, yet it
                  isn't used.. is it being recomputed ?? */
		ReAnalyzeFrame (iCurrentFrame, pAnalParams);
	}
  
	/* incorporate the peaks into the corresponding tracks */
	/* This is done after a SMS_DELAY_FRAMES delay  */
	if (pAnalParams->ppFrames[iCurrentFrame - SMS_DELAY_FRAMES]->fFundamental > 0 ||
	    ((pAnalParams->iFormat == SMS_FORMAT_IH ||
	      pAnalParams->iFormat == SMS_FORMAT_IHP) &&
	     pAnalParams->ppFrames[iCurrentFrame - SMS_DELAY_FRAMES]->nPeaks > 0))
		sms_peakContinuation (iCurrentFrame - SMS_DELAY_FRAMES, pAnalParams);
    
	/* fill gaps and delete short tracks */
	if (pAnalParams->iCleanTracks > 0 &&
	    pAnalParams->ppFrames[iCurrentFrame - SMS_DELAY_FRAMES]->iStatus != SMS_FRAME_EMPTY)
		sms_cleanTracks (iCurrentFrame - SMS_DELAY_FRAMES, pAnalParams);

	/* do stochastic analysis */
	if (pAnalParams->iStochasticType != SMS_STOC_NONE)
	{
		/* synthesize deterministic signal */
		if (pAnalParams->ppFrames[1]->iStatus != SMS_FRAME_EMPTY &&
		    pAnalParams->ppFrames[1]->iStatus != SMS_FRAME_END)
		{
			/* shift synthesis buffer */
			memcpy ( pAnalParams->synthBuffer.pFBuffer,
                                 pAnalParams->synthBuffer.pFBuffer+pAnalParams->sizeHop,
			        sizeof(sfloat) * pAnalParams->sizeHop);
			memset (pAnalParams->synthBuffer.pFBuffer+pAnalParams->sizeHop,
                                0, sizeof(sfloat) * pAnalParams->sizeHop);
      
			/* get deterministic signal with phase  */
			sms_sineSynthFrame (&pAnalParams->ppFrames[1]->deterministic,
			                pAnalParams->synthBuffer.pFBuffer+pAnalParams->sizeHop,
			                pAnalParams->sizeHop, &pAnalParams->prevFrame,
			                pAnalParams->iSamplingRate);
		}
  
		/* perform stochastic analysis after 1 frame of the     */
		/* deterministic synthesis because it needs two frames  */
		if (pAnalParams->ppFrames[0]->iStatus != SMS_FRAME_EMPTY &&
		    pAnalParams->ppFrames[0]->iStatus != SMS_FRAME_END)
		
                {
			int sizeResidual = pAnalParams->sizeHop * 2;
			int iSoundLoc = pAnalParams->ppFrames[0]->iFrameSample - pAnalParams->sizeHop;
			sfloat *pOriginal = &(pAnalParams->soundBuffer.pFBuffer[iSoundLoc -
			                                       pAnalParams->soundBuffer.iMarker]);
			sfloat *pFResidual;

                        static sfloat *pWindow;
                        static int sizeWindowArray = 0;

			int sizeData =
				MIN (pAnalParams->soundBuffer.sizeBuffer -
				      (iSoundLoc - pAnalParams->soundBuffer.iMarker),
				     sizeResidual);
			if ((pFResidual = (sfloat *) calloc (sizeResidual, sizeof(float)))
			    == NULL)
			{
                                sms_error("sms_analyze: error allocating memory for pFResidual");
                                return -1;
                        }
                        if (sizeWindowArray != sizeData)
                        {
                                if(sizeWindowArray != 0) free(pWindow);
                                if((pWindow = (sfloat *) calloc(sizeData, sizeof(float))) == NULL)
                                {
                                        sms_error("sms_analyze: error allocating memory for pWindow");
                                        return -1;
                                }
                                sms_getWindow( sizeData, pWindow, SMS_WIN_HAMMING);
                                sms_scaleWindow( sizeData, pWindow);
                                sizeWindowArray = sizeData;
                        }

			/* obtain residual sound from original and synthesized sounds.  accumulate the residual percentage.*/
                        pAnalParams->fResidualAccumPerc += sms_residual (sizeData,
                                      pAnalParams->synthBuffer.pFBuffer,
                                      pOriginal,
                                      pFResidual,
                                      pWindow);


                        if (pAnalParams->iStochasticType == SMS_STOC_APPROX)
                        {
                                /* filter residual with a high pass filter (it solves some problems) */
                                sms_filterHighPass (sizeData, pFResidual, pAnalParams->iSamplingRate);
                                
                                /* approximate residual */
                                sms_stocAnalysis (sizeData, pFResidual, pWindow, pSmsData);
                        }
                        else if  (pAnalParams->iStochasticType == SMS_STOC_IFFT)
                        {
                                int sizeMag = sms_power2(sizeData >> 1);
                                sms_spectrum (sizeData, pFResidual, pWindow, sizeMag, pSmsData->pFStocCoeff, 
                                        pSmsData->pResPhase);
                        }

			/* get sharper transitions in deterministic representation */
                        /* \todo why is this done in the stochastic analysis space? */
                        sms_scaleDet (pAnalParams->synthBuffer.pFBuffer, pOriginal,
                                      pAnalParams->ppFrames[0]->deterministic.pFSinAmp,
                                      pAnalParams, pSmsData->nTracks);
      
			pAnalParams->ppFrames[0]->iStatus = SMS_FRAME_DONE;

			free ((char *) pFResidual); /* \todo get rid of this free, manage memory the same as spectrum functions */
		}
	}
	else if (pAnalParams->ppFrames[0]->iStatus != SMS_FRAME_EMPTY &&
	         pAnalParams->ppFrames[0]->iStatus != SMS_FRAME_END)
		pAnalParams->ppFrames[0]->iStatus = SMS_FRAME_DONE;

	/* get the result */
	if (pAnalParams->ppFrames[0]->iStatus == SMS_FRAME_EMPTY)
        {
		return (0);
        }
	/* return analysis data */
	else if (pAnalParams->ppFrames[0]->iStatus == SMS_FRAME_DONE)
	{
		/* put data into output */
		int length = sizeof(sfloat) * pSmsData->nTracks;
		memcpy ((char *) pSmsData->pFSinFreq, (char *)
		        pAnalParams->ppFrames[0]->deterministic.pFSinFreq, length);
		memcpy ((char *) pSmsData->pFSinAmp, (char *)
		         pAnalParams->ppFrames[0]->deterministic.pFSinAmp, length);
                /* convert mags back to linear */
                sms_arrayDBToMag(pSmsData->nTracks, pSmsData->pFSinAmp);
		if (pAnalParams->iFormat == SMS_FORMAT_HP ||
		    pAnalParams->iFormat == SMS_FORMAT_IHP)
			memcpy ((char *) pSmsData->pFSinPha, (char *)
			        pAnalParams->ppFrames[0]->deterministic.pFSinPha, length);

                /* do post-processing (for now, spectral envelope calculation and storage) */
                if(pAnalParams->specEnvParams.iType != SMS_ENV_NONE)
                {
                        //printf("computing spectral envelope \n");
                        sms_spectralEnvelope( pSmsData, &pAnalParams->specEnvParams);
                        
                }
		return (1);
	}
	/* done, end of sound */
	else if (pAnalParams->ppFrames[0]->iStatus == SMS_FRAME_END)
		return (-1);
	else
	{
		sms_error ("sms_analyze error: wrong status of frame.");
                return(-1);
	}
	return (1);
}
