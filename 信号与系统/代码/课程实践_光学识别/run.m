%% Call light_f_detection function in batch
% video_list = {
%     'light.mp4',...
%     'laptop60hz_off.MP4','laptop60hz_on.MP4','laptop144hz_off.mp4',...
%     'pad60hz_off.mp4','pad60hz_on.mp4','pad120hz_off.MP4','pad120hz_on.mp4',...
%     'king60hz.mp4','king90hz.mp4','king120hz.mp4',...%honor of king on pad with pins 60 90 120
%     'library_laptop60hz_on.mp4'};
video_list = {
    'pad120hz_off.MP4'};
for idx = 1:numel(video_list)
    video_file      = video_list{idx};
    output_folder   = fullfile(pwd, ['frames' num2str(idx)]);
    N_frames        = 600;
    frame_rate      = 240;
    analysis_dimension  = '1D';
    roi_selection_method = 'manual';
    ROI_preset      = [400, 200, 500, 300];
    
    light_f_detection(video_file, output_folder, N_frames, frame_rate, analysis_dimension, roi_selection_method, ROI_preset);
end
%   1 light 100Hz       || Measured 100Hz
%   2 laptop 60Hz off   || Measured 60Hz
%   3 laptop 60Hz on    || Measured 60Hz+100Hz not obvious (increased to 600 frames, smaller ROI, prominent taskbar and specular reflection at bottom-right)
%   4 laptop 144Hzoff   || Not measured, chaotic

%   5 pad 60Hz  off     || Measured 60Hz (main) + 120Hz (secondary)
%   6 pad 60Hz  on      || Measured 60Hz (main) + 120Hz (secondary)
%   7 pad 120Hz off     || Measured 118.6Hz
%   8 pad 120Hz on      || Measured 119Hz
%% honor of king test: 60, 90, and 120 Hz results are 60, 120, 120; brightness change at 90Hz is weaker than PWM-driven change
%% Explanation: The pad uses 1440Hz PWM dimming, so compared to the laptop's 60Hz, an additional 12th harmonic peak appears at 120Hz
%% Explanation: Under lit conditions, the pad cannot detect the 100Hz indoor lighting because its 1000 nit brightness is higher than the laptop's, causing aliasing of the 100Hz component
%camera dji action4 1080P 240Hz
% Experiment
% #1 baseline ceiling light,
% #2 laptop 60Hz light off, showing 60Hz result
% #3 laptop 60Hz light on, showing 60Hz+100Hz (both displayed on the same slide for comparison)
% #4 laptop 144Hz, which does not meet the Nyquist sampling theorem (additional slide with formula explanation)
% #5 pad 60Hz off and 60Hz on displayed simultaneously, 
% with no 100Hz result observed due to OLED screen and peak brightness diminishing ceiling light effect
% #6 the secondary 120Hz peak in the 60Hz image indicates it is caused by the pad's PWM dimming
% #7 display the pad's 120Hz measurement with a 240fps camera, showing a successful measurement with slight error
% #8  display three sets of Honor of Kings videos and processing results at 60, 90, and 120Hz, 
% indicating that the pad's 1440Hz 12th harmonic plays a dominant role,
% leading to a 90Hz refresh rate (OLED refresh adjusts brightness differently than LCD, resulting in subtle visual changes), resulting in 120Hz