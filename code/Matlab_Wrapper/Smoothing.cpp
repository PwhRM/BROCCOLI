/*
 * BROCCOLI: An open source multi-platform software for parallel analysis of fMRI data on many core CPUs and GPUS
 * Copyright (C) <2013>  Anders Eklund, andek034@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mex.h"
#include "help_functions.cpp"
#include "broccoli_lib.h"

void cleanUp()
{
    //cudaDeviceReset();
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    //-----------------------
    // Input pointers
    
    double		    *h_fMRI_Volumes_double;
    float           *h_fMRI_Volumes;
    
    double	        *h_Smoothing_Filter_X_double, *h_Smoothing_Filter_Y_double, *h_Smoothing_Filter_Z_double;
    float		    *h_Smoothing_Filter_X, *h_Smoothing_Filter_Y, *h_Smoothing_Filter_Z;       
    
    float           Smoothing_FWHM, EPI_VOXEL_SIZE_X, EPI_VOXEL_SIZE_Y, EPI_VOXEL_SIZE_Z;
    
    int             OPENCL_PLATFORM, OPENCL_DEVICE;
    
    //-----------------------
    // Output pointers
    
    double     		*h_Filter_Response_double;
    float    		*h_Filter_Response;
    
    double          *convolution_time;
    
    //---------------------
    
    /* Check the number of input and output arguments. */
    if(nrhs<10)
    {
        mexErrMsgTxt("Too few input arguments.");
    }
    if(nrhs>10)
    {
        mexErrMsgTxt("Too many input arguments.");
    }
    if(nlhs<1)
    {
        mexErrMsgTxt("Too few output arguments.");
    }
    if(nlhs>1)
    {
        mexErrMsgTxt("Too many output arguments.");
    }
    
    /* Input arguments */
    
    // The data
    h_fMRI_Volumes_double =  (double*)mxGetData(prhs[0]);
    h_Smoothing_Filter_X_double = (double*)mxGetData(prhs[1]);
    h_Smoothing_Filter_Y_double = (double*)mxGetData(prhs[2]);
    h_Smoothing_Filter_Z_double = (double*)mxGetData(prhs[3]);
    EPI_VOXEL_SIZE_X = (float)mxGetScalar(prhs[4]);
    EPI_VOXEL_SIZE_Y = (float)mxGetScalar(prhs[5]);
    EPI_VOXEL_SIZE_Z = (float)mxGetScalar(prhs[6]);
    Smoothing_FWHM = (float)mxGetScalar(prhs[7]);
    OPENCL_PLATFORM  = (int)mxGetScalar(prhs[8]);
    OPENCL_DEVICE  = (int)mxGetScalar(prhs[9]);
    
    int NUMBER_OF_DIMENSIONS = mxGetNumberOfDimensions(prhs[0]);
    const int *ARRAY_DIMENSIONS_DATA = mxGetDimensions(prhs[0]);
    const int *ARRAY_DIMENSIONS_FILTER = mxGetDimensions(prhs[1]);
    
    int DATA_H, DATA_W, DATA_D, DATA_T, FILTER_LENGTH;
    
    DATA_H = ARRAY_DIMENSIONS_DATA[0];
    DATA_W = ARRAY_DIMENSIONS_DATA[1];
    DATA_D = ARRAY_DIMENSIONS_DATA[2];
    DATA_T = ARRAY_DIMENSIONS_DATA[3];
    
    FILTER_LENGTH = ARRAY_DIMENSIONS_FILTER[0];
            
    int DATA_SIZE = DATA_W * DATA_H * DATA_D * DATA_T * sizeof(float);
    int FILTER_SIZE = FILTER_LENGTH * sizeof(float);
            
    mexPrintf("Data size : %i x %i x %i x %i \n",  DATA_W, DATA_H, DATA_D, DATA_T);
    
    //-------------------------------------------------
    // Output to Matlab
    
    // Create pointer for volumes to Matlab
    
    int ARRAY_DIMENSIONS_OUT[4];
    ARRAY_DIMENSIONS_OUT[0] = DATA_H;
    ARRAY_DIMENSIONS_OUT[1] = DATA_W;
    ARRAY_DIMENSIONS_OUT[2] = DATA_D;
    ARRAY_DIMENSIONS_OUT[3] = DATA_T;
    
    plhs[0] = mxCreateNumericArray(NUMBER_OF_DIMENSIONS,ARRAY_DIMENSIONS_OUT,mxDOUBLE_CLASS, mxREAL);
    h_Filter_Response_double = mxGetPr(plhs[0]);          
    
    // ------------------------------------------------
    
    // Allocate memory on the host
    h_fMRI_Volumes                 = (float *)mxMalloc(DATA_SIZE);
    h_Smoothing_Filter_X           = (float *)mxMalloc(FILTER_SIZE);
    h_Smoothing_Filter_Y           = (float *)mxMalloc(FILTER_SIZE);
    h_Smoothing_Filter_Z           = (float *)mxMalloc(FILTER_SIZE);
    h_Filter_Response              = (float *)mxMalloc(DATA_SIZE);
    
    // Reorder and cast data
    pack_double2float_volumes(h_fMRI_Volumes, h_fMRI_Volumes_double, DATA_W, DATA_H, DATA_D, DATA_T);
    pack_double2float(h_Smoothing_Filter_X, h_Smoothing_Filter_X_double, FILTER_LENGTH);
    pack_double2float(h_Smoothing_Filter_Y, h_Smoothing_Filter_Y_double, FILTER_LENGTH);
    pack_double2float(h_Smoothing_Filter_Z, h_Smoothing_Filter_Z_double, FILTER_LENGTH);
    
    //------------------------
    
    BROCCOLI_LIB BROCCOLI(OPENCL_PLATFORM,OPENCL_DEVICE);
    
    BROCCOLI.SetEPIWidth(DATA_W);
    BROCCOLI.SetEPIHeight(DATA_H);
    BROCCOLI.SetEPIDepth(DATA_D);
    BROCCOLI.SetEPITimepoints(DATA_T);
    BROCCOLI.SetEPIVoxelSizeX(EPI_VOXEL_SIZE_X);
    BROCCOLI.SetEPIVoxelSizeY(EPI_VOXEL_SIZE_Y);
    BROCCOLI.SetEPIVoxelSizeZ(EPI_VOXEL_SIZE_Z);
    BROCCOLI.SetInputfMRIVolumes(h_fMRI_Volumes);
    BROCCOLI.SetOutputSmoothedfMRIVolumes(h_Filter_Response);
    BROCCOLI.SetSmoothingFilters(h_Smoothing_Filter_X, h_Smoothing_Filter_Y, h_Smoothing_Filter_Z);
    BROCCOLI.SetSmoothingAmount(Smoothing_FWHM);
    
    /*
     * Error checking     
     */

    int getPlatformIDsError = BROCCOLI.GetOpenCLPlatformIDsError();
	int getDeviceIDsError = BROCCOLI.GetOpenCLDeviceIDsError();		
	int createContextError = BROCCOLI.GetOpenCLCreateContextError();
	int getContextInfoError = BROCCOLI.GetOpenCLContextInfoError();
	int createCommandQueueError = BROCCOLI.GetOpenCLCreateCommandQueueError();
	int createProgramError = BROCCOLI.GetOpenCLCreateProgramError();
	int buildProgramError = BROCCOLI.GetOpenCLBuildProgramError();
	int getProgramBuildInfoError = BROCCOLI.GetOpenCLProgramBuildInfoError();
          
    mexPrintf("Get platform IDs error is %d \n",getPlatformIDsError);
    mexPrintf("Get device IDs error is %d \n",getDeviceIDsError);
    mexPrintf("Create context error is %d \n",createContextError);
    mexPrintf("Get create context info error is %d \n",getContextInfoError);
    mexPrintf("Create command queue error is %d \n",createCommandQueueError);
    mexPrintf("Create program error is %d \n",createProgramError);
    mexPrintf("Build program error is %d \n",buildProgramError);
    mexPrintf("Get program build info error is %d \n",getProgramBuildInfoError);
    
    int* createKernelErrors = BROCCOLI.GetOpenCLCreateKernelErrors();
    for (int i = 0; i < 34; i++)
    {
        if (createKernelErrors[i] != 0)
        {
            mexPrintf("Create kernel error %i is %d \n",i,createKernelErrors[i]);
        }
    }
    
    int* createBufferErrors = BROCCOLI.GetOpenCLCreateBufferErrors();
    for (int i = 0; i < 30; i++)
    {
        if (createBufferErrors[i] != 0)
        {
            mexPrintf("Create buffer error %i is %d \n",i,createBufferErrors[i]);
        }
    }
        
    int* runKernelErrors = BROCCOLI.GetOpenCLRunKernelErrors();
    for (int i = 0; i < 20; i++)
    {
        if (runKernelErrors[i] != 0)
        {
            mexPrintf("Run kernel error %i is %d \n",i,runKernelErrors[i]);
        }
    } 
    
    mexPrintf("Build info \n \n %s \n", BROCCOLI.GetOpenCLBuildInfoChar());  
    
    if ( (getPlatformIDsError + getDeviceIDsError + createContextError + getContextInfoError + createCommandQueueError + createProgramError + buildProgramError + getProgramBuildInfoError) == 0)
    {        
        BROCCOLI.PerformSmoothingWrapper();
    }
    
    
    unpack_float2double_volumes(h_Filter_Response_double, h_Filter_Response, DATA_W, DATA_H, DATA_D, DATA_T);
        
    
    
    // Free all the allocated memory on the host
    mxFree(h_fMRI_Volumes);
    mxFree(h_Smoothing_Filter_X);
    mxFree(h_Smoothing_Filter_Y);
    mxFree(h_Smoothing_Filter_Z);
    mxFree(h_Filter_Response);
    
    //mexAtExit(cleanUp);
    
    return;
}


