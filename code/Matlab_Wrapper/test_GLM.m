%  	 BROCCOLI: An open source multi-platform software for parallel analysis of fMRI data on many core CPUs and GPUS
%    Copyright (C) <2013>  Anders Eklund, andek034@gmail.com
%
%    This program is free software: you can redistribute it and/or modify
%    it under the terms of the GNU General Public License as published by
%    the Free Software Foundation, either version 3 of the License, or
%    (at your option) any later version.
%
%    This program is distributed in the hope that it will be useful,
%    but WITHOUT ANY WARRANTY; without even the implied warranty of
%    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%    GNU General Public License for more details.
%
%    You should have received a copy of the GNU General Public License
%    along with this program.  If not, see <http://www.gnu.org/licenses/>.
%-----------------------------------------------------------------------------

%---------------------------------------------------------------------------------------------------------------------
% README
% If you run this code in Windows, your graphics driver might stop working
% for large volumes / large filter sizes. This is not a bug in my code but is due to the
% fact that the Nvidia driver thinks that something is wrong if the GPU
% takes more than 2 seconds to complete a task. This link solved my problem
% https://forums.geforce.com/default/topic/503962/tdr-fix-here-for-nvidia-driver-crashing-randomly-in-firefox/
%---------------------------------------------------------------------------------------------------------------------

clear all
clc
close all

%mex GLM.cpp -lOpenCL -lBROCCOLI_LIB -IC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/include -IC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/include/CL -LC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/lib/x64 -LC:/users/wande/Documents/Visual' Studio 2010'/Projects/BROCCOLI_LIB/x64/Release/ -IC:/users/wande/Documents/Visual' Studio 2010'/Projects/BROCCOLI_LIB/BROCCOLI_LIB -IC:\Users\wande\Documents\Visual' Studio 2010'\Projects\BROCCOLI_LIB\nifticlib-2.0.0\niftilib  -IC:\Users\wande\Documents\Visual' Studio 2010'\Projects\BROCCOLI_LIB\nifticlib-2.0.0\znzlib  

mex -g GLM.cpp -lOpenCL -lBROCCOLI_LIB -IC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/include -IC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/include/CL -LC:/Program' Files'/NVIDIA' GPU Computing Toolkit'/CUDA/v5.0/lib/x64 -LC:/users/wande/Documents/Visual' Studio 2010'/Projects/BROCCOLI_LIB/x64/Debug/ -IC:/users/wande/Documents/Visual' Studio 2010'/Projects/BROCCOLI_LIB/BROCCOLI_LIB -IC:\Users\wande\Documents\Visual' Studio 2010'\Projects\BROCCOLI_LIB\nifticlib-2.0.0\niftilib  -IC:\Users\wande\Documents\Visual' Studio 2010'\Projects\BROCCOLI_LIB\nifticlib-2.0.0\znzlib  

opencl_platform = 1;
opencl_device = 0;

%sx = 63; sy = 65; sz = 31; st = 100;
%fMRI_volumes = randn(sy,sx,sz,st);

load ../../test_data/hand_movements_right.mat
fMRI_volumes = vol_exp;
[sy sx sz st] = size(fMRI_volumes);
[sy; sx; sz; st]'

% Create regressors
[sy sx sz st] = size(fMRI_volumes)
mask = randn(sy,sx,sz);

X_GLM_ = zeros(st,5);
X_GLM_ = zeros(st,1);
NN = 0;
while NN < st
    X_GLM_((NN+1):(NN+10),1) =   0;  % Activity
    X_GLM_((NN+11):(NN+20),1) =  1;  % Rest
    NN = NN + 20;
end
X_GLM(:,1) = X_GLM_(1:st) - mean(X_GLM_(1:st));
a = ones(st,1)/st;
X_GLM(:,2) = a/norm(a(:));
a = -(st-1)/2:(st-1)/2;
b = a.*a;
c = a.*a.*a;
X_GLM(:,3) = a/norm(a(:));
X_GLM(:,4) = b/norm(b(:));
X_GLM(:,5) = c/norm(c(:));

xtxxt_GLM = inv(X_GLM'*X_GLM)*X_GLM';

% Create contrasts
%contrasts = zeros(size(X_GLM,2),3);
contrasts = [1 0 0 0 0]';
%contrasts(:,1) = [1 0 0 0 0 0 0 0]';
%contrasts(:,2) = [0 1 0 0 0 0 0 0]';
%contrasts(:,3) = [0 0 0 0 1 0 0 0]';
for i = 1:size(contrasts,2)
    contrast = contrasts(:,i);
    ctxtxc_GLM(i) = contrast'*inv(X_GLM'*X_GLM)*contrast;
end
ctxtxc_GLM

mask = ones(sy,sx,sz,st);
statistical_maps_cpu = zeros(sy,sx,sz,size(contrasts,2));
betas_cpu = zeros(sy,sx,sz,size(X_GLM,2));
residuals_cpu = zeros(sy,sx,sz,st);
residual_variances_cpu = zeros(sy,sx,sz);


for x = 1:sx
    for y = 1:sy
        for z = 1:sz
            timeseries = squeeze(fMRI_volumes(y,x,z,:));
            timeseries = timeseries - mean(timeseries);
            fMRI_volumes(y,x,z,:) = timeseries;
            beta = xtxxt_GLM*timeseries;
            betas_cpu(y,x,z,:) = beta;
            eps = timeseries - X_GLM*beta;
            residuals_cpu(y,x,z,:) = eps;
            %residual_variances_cpu(y,x,z) = sum((eps-mean(eps)).^2)/(st-size(X_GLM,2));
            residual_variances_cpu(y,x,z) = var(eps);
            for i = 1:size(contrasts,2)
                contrast = contrasts(:,i);
                statistical_maps_cpu(y,x,z,i) = contrast'*beta / sqrt( residual_variances_cpu(y,x,z) * ctxtxc_GLM(i));
            end
        end
    end
end

% Create smoothing filters
smoothing_filter_x = fspecial('gaussian',9,1);
smoothing_filter_x = smoothing_filter_x(:,5);
smoothing_filter_x = smoothing_filter_x / sum(abs(smoothing_filter_x));
smoothing_filter_y = smoothing_filter_x;
smoothing_filter_z = smoothing_filter_x;

tic
[betas_opencl, residuals_opencl, residual_variances_opencl, statistical_maps_opencl, ar1_estimates_opencl, ar2_estimates_opencl, ar3_estimates_opencl, ar4_estimates_opencl] = GLM(fMRI_volumes,mask,X_GLM,xtxxt_GLM',contrasts,ctxtxc_GLM,smoothing_filter_x,smoothing_filter_y,smoothing_filter_z,opencl_platform,opencl_device);
toc

slice = 17;

figure
imagesc([betas_cpu(:,:,slice,1) betas_opencl(:,:,slice,1)]); colorbar

figure
imagesc([residual_variances_cpu(:,:,slice) residual_variances_opencl(:,:,slice)]); colorbar

figure
imagesc([statistical_maps_cpu(:,:,slice,1) statistical_maps_opencl(:,:,slice,1)]); colorbar

figure
imagesc([ar1_estimates_opencl(:,:,slice) ]); colorbar

figure
imagesc([ar2_estimates_opencl(:,:,slice) ]); colorbar

figure
imagesc([ar3_estimates_opencl(:,:,slice) ]); colorbar

figure
imagesc([ar4_estimates_opencl(:,:,slice) ]); colorbar

beta_tot_error = sum(abs(betas_cpu(:) - betas_opencl(:)))
beta_max_error = max(abs(betas_cpu(:) - betas_opencl(:)))

%for slice = 1:sz
%    slice
%    a = betas_cpu(:,:,slice,1);
%    b = betas_opencl(:,:,slice,1);
%    max(a(:) - b(:))
%end

residual_tot_error = sum(abs(residuals_cpu(:) - residuals_opencl(:)))
residual_max_error = max(abs(residuals_cpu(:) - residuals_opencl(:)))

residual_variances_tot_error = sum(abs(residual_variances_cpu(:) - residual_variances_opencl(:)))
residual_variances_max_error = max(abs(residual_variances_cpu(:) - residual_variances_opencl(:)))

stat_tot_error = sum(abs(statistical_maps_cpu(:) - statistical_maps_opencl(:)))
stat_max_error = max(abs(statistical_maps_cpu(:) - statistical_maps_opencl(:)))

% for slice = 1:sz
%     slice
%     a = statistical_map_cpu(:,:,slice,1);
%     b = statistical_map_opencl(:,:,slice,1);
%     max(a(:) - b(:))
% end


