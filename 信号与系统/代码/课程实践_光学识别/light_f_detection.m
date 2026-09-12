% Convert the script to a function, remove default parameters at the top, and use input variables consistently
function light_f_detection(video_file, output_folder, N_frames, frame_rate, analysis_dimension, roi_selection_method, ROI_preset)

% Modify the output folder naming rule to frames_videoName_frameCount
[~, video_name, ~] = fileparts(video_file);
% Create a display name for the video (replace underscores with hyphens to avoid subscript formatting issues in MATLAB titles)
display_video_name = strrep(video_name, '_', '-');
output_folder = fullfile(fileparts(output_folder), sprintf('frames_%s_%d', video_name, N_frames));

% Create a corresponding results folder
% Extract folder number from output_folder
[~, folder_name, ~] = fileparts(output_folder);
folder_number = '';
for i = 1:length(folder_name)
    if isstrprop(folder_name(i), 'digit')
        folder_number = [folder_number, folder_name(i)];
    end
end

% Create a results folder with the corresponding number
results_folder = fullfile('results', ['results', folder_number]);
if ~exist(results_folder, 'dir')
    mkdir(results_folder);
end
fprintf('Results will be saved to folder: %s\n', results_folder);

% --- Step 1 & 2: Video frame extraction ---
% Check if the video file exists
if ~exist(video_file, 'file')
    error('Video file "%s" does not exist. Please ensure it is in the same directory as the script or provide the correct path.', video_file);
end

if ~exist(output_folder, 'dir')
    mkdir(output_folder);
end

% Check if the frame folder is empty or the frame count does not match the expected value; if so, extract frames automatically
frame_files = dir(fullfile(output_folder, 'frame_*.png'));
actual_frames_in_folder = numel(frame_files);
if actual_frames_in_folder == 0 || (actual_frames_in_folder ~= N_frames && ~isinf(N_frames))
    if actual_frames_in_folder == 0
        fprintf('Frame folder "%s" is empty, extracting frames from "%s"...\n', output_folder, video_file);
    else
        fprintf('Frame count (%d) in folder "%s" does not match the expected frame count (%d), re-extracting frames...\n', actual_frames_in_folder, output_folder, N_frames);
        % Clear existing frame files
        for i = 1:actual_frames_in_folder
            delete(fullfile(output_folder, frame_files(i).name));
        end
    end
    try
        v = VideoReader(video_file);
        % Attempt to automatically retrieve the frame rate
        if isprop(v, 'FrameRate') && ~isempty(v.FrameRate) && frame_rate == 30
            frame_rate = v.FrameRate;
            fprintf('Detected frame rate from video file: %.2f Hz\n', frame_rate);
        else
            fprintf('Using preset frame rate: %.2f Hz \n', frame_rate);
        end
        k = 1;
        max_frames_to_extract = N_frames;
        while hasFrame(v) && (isinf(max_frames_to_extract) || k <= max_frames_to_extract)
            frame = readFrame(v);
            filename = fullfile(output_folder, sprintf('frame_%03d.png', k));
            imwrite(frame, filename);
            if mod(k, 50) == 0
                fprintf('Extracted %d frames...\n', k);
            end
            k = k + 1;
        end
        N_frames = k - 1;
        if N_frames == 0
            error('No frames could be extracted from video "%s".', video_file);
        end
        disp(['Extraction complete. A total of ', num2str(N_frames), ' frames were extracted to folder: ', output_folder]);
    catch ME
        error('Error during frame extraction: %s', ME.message);
    end
    % Re-fetch the list of frame files
    frame_files = dir(fullfile(output_folder, 'frame_*.png'));
    actual_frames_in_folder = numel(frame_files);
end

if actual_frames_in_folder == 0
    error('Frame folder "%s" is empty. Please delete the folder or ensure it contains frame images.', output_folder);
end

if isinf(N_frames) % If N_frames is set to Inf, use all frames in the folder
    N_frames = actual_frames_in_folder;
    fprintf('Processing all %d frames in folder "%s".\n', N_frames, output_folder);
elseif actual_frames_in_folder < N_frames
    warning('The number of frames in the folder (%d) is less than the requested N_frames (%d). Using the actual number of frames in the folder.', actual_frames_in_folder, N_frames);
    N_frames = actual_frames_in_folder;
elseif actual_frames_in_folder > N_frames
    fprintf('The number of frames in the folder (%d) exceeds the requested N_frames (%d). Only the first %d frames will be processed.\n', actual_frames_in_folder, N_frames, N_frames);
else
    fprintf('The number of frames in the folder (%d) matches the requested N_frames.\n', actual_frames_in_folder);
end
% Attempt to read video properties to retrieve the frame rate (if the extraction step was skipped)
try
    v_info = VideoReader(video_file);
    if isprop(v_info, 'FrameRate') && ~isempty(v_info.FrameRate) && frame_rate == 30 % Only override if the default value was not modified
        frame_rate = v_info.FrameRate;
        fprintf('Retrieved frame rate from video file properties: %.2f Hz\n', frame_rate);
    else
        fprintf('Using preset frame rate: %.2f Hz (please confirm this value matches the video specifications)\n', frame_rate);
    end
catch ME_info
    warning('Unable to retrieve properties of video "%s" to get frame rate: %s. Continuing with preset value %.2f Hz.', video_file, ME_info.message, frame_rate);
end

% --- Step 3: ROI selection and preprocessing ---
fprintf('Step 3: ROI selection and preprocessing\n');
filename_first = fullfile(output_folder, sprintf('frame_%03d.png', 1));
if ~exist(filename_first, 'file')
    error('First frame image not found: %s. Please ensure frames were extracted correctly.', filename_first);
end
img_first = imread(filename_first);
img_gray_first = im2gray(img_first); % Convert to grayscale

if strcmpi(roi_selection_method, 'manual')
    figure;
    imshow(img_gray_first);
    title(['Please manually select ROI (double-click to confirm) - ', display_video_name]);
    h_roi = imrect; % Allow user to draw a rectangle
    ROI = wait(h_roi); % Wait for user to double-click to confirm
    ROI = round(ROI); % Round coordinates [x, y, width, height]
    close(gcf); % Close the selection window
    fprintf('Manually selected ROI: [%d, %d, %d, %d]\n', ROI(1), ROI(2), ROI(3), ROI(4));
elseif strcmpi(roi_selection_method, 'preset')
    ROI = ROI_preset;
    fprintf('Using preset ROI: [%d, %d, %d, %d]\n', ROI(1), ROI(2), ROI(3), ROI(4));
    % Optional: Display the preset ROI
    figure;
    imshow(img_gray_first);
    set(gca, 'NextPlot', 'add'); % hold on
    rectangle('Position', ROI, 'EdgeColor', 'r', 'LineWidth', 1);
    title(['Preset ROI - ', display_video_name]);
    set(gca, 'NextPlot', 'replace'); % hold off
    pause(1); % Briefly display
    % close(gcf);
else
    error('Invalid roi_selection_method. Choose ''manual'' or ''preset''.');
end

% Check if the ROI is valid
if ROI(3) <= 0 || ROI(4) <= 0
    error('The width and height of the ROI must be greater than 0.');
end

% --- Step 4: Stack into "Time-Space" matrix ---
fprintf('Step 4: Data stacking\n');
stack = []; % Used to store 2D data (space x time)
brightness_signal_1d = zeros(1, N_frames); % Used to store 1D average brightness data

for i = 1:N_frames
    filename = fullfile(output_folder, sprintf('frame_%03d.png', i));
    if ~exist(filename, 'file')
        warning('Frame file not found: %s, skipping this frame.', filename);
        continue;
    end
    img = imread(filename);
    img_gray = im2gray(img); % Grayscale processing

    % Crop ROI
    try
        block = imcrop(img_gray, ROI);
    catch ME
        error('Error cropping ROI (frame %d): %s\nPlease check if the ROI coordinates are within the image range. ROI=[%d,%d,%d,%d], ImgSize=[%d,%d]', ...
              i, ME.message, ROI(1), ROI(2), ROI(3), ROI(4), size(img_gray,2), size(img_gray,1));
    end

    if isempty(block)
        warning('The ROI cropping result of frame %d is empty, skipping this frame. ROI=[%d,%d,%d,%d]', i, ROI(1), ROI(2), ROI(3), ROI(4));
        continue;
    end

    if strcmpi(analysis_dimension, '1D')
            % Calculate the average brightness of the entire ROI area
            brightness_signal_1d(i) = mean(block(:));
        elseif strcmpi(analysis_dimension, '2D')
        % Take the average value of each row in the ROI area as the strip profile (space dimension = height)
        strip_profile = mean(block, 2); % Average by row, get column vector
        % Or take the average value of each column (space dimension = width)
        % strip_profile = mean(block, 1)'; % Average by column, transpose to column vector
        stack = [stack, strip_profile]; % Stack column vectors by time
    else
        error('Invalid analysis_dimension. Choose ''1D'' or ''2D''.');
    end
end

% If it is 1D analysis, ensure the signal length is correct
if strcmpi(analysis_dimension, '1D') && length(brightness_signal_1d) ~= N_frames
    brightness_signal_1d = brightness_signal_1d(1:N_frames); % Adjust length just in case
end

% --- Step 5: Fourier analysis ---
fprintf('Step 5: Fourier analysis (%s)\n', analysis_dimension);

% Create an empty structure to store all FFT results for unified display later
fft_results = struct();

if strcmpi(analysis_dimension, '1D')
    % --- 1D FFT analysis ---
    if N_frames < 2
        error('At least 2 frames are required for 1D FFT analysis.');
    end
    % Remove DC component
    brightness_signal_1d = brightness_signal_1d - mean(brightness_signal_1d);

    L = N_frames; % Signal length
    Y = fft(brightness_signal_1d);
    P2 = abs(Y/L); % Two-sided spectrum
    P1 = P2(1:floor(L/2)+1); % Single-sided spectrum
    P1(2:end-1) = 2*P1(2:end-1);

    % Calculate frequency axis (Hz)
    f_axis_1d = frame_rate*(0:(L/2))/L;

    % Only find the maximum value for frequencies greater than or equal to 25Hz
    freq_mask = f_axis_1d >= 25;
    if any(freq_mask)
        [max_amp, idx_masked] = max(P1(freq_mask));
        idx_candidates = find(freq_mask);
        idx = idx_candidates(idx_masked);
        peak_freq_1d = f_axis_1d(idx);
    else
        [max_amp, idx] = max(P1);
        peak_freq_1d = f_axis_1d(idx);
    end
    fprintf('1D FFT peak frequency (>=25Hz): %.2f Hz (amplitude: %.2f)\n', peak_freq_1d, max_amp);
    
    % Save 1D FFT results to structure
    fft_results.type_1d = '1D';
    fft_results.freq_1d = f_axis_1d;
    fft_results.amp_1d = P1;
    fft_results.peak_freq_1d = peak_freq_1d;
    fft_results.peak_amp_1d = max_amp;
    
    % Display the original time-domain signal (optional, not part of the FFT results)
    figure('Name', 'Original Time-Domain Signal', 'Position', [100, 100, 800, 400]);
    plot(1:N_frames, brightness_signal_1d);
    title(['ROI Average Brightness Over Time (1D Signal) - ', display_video_name]);
    xlabel('Frame Number');
    ylabel('Average Brightness');
    grid on;
    saveas(gcf, fullfile(results_folder, '1D_time_domain.png'));
    saveas(gcf, fullfile(results_folder, '1D_time_domain.fig'));

elseif strcmpi(analysis_dimension, '2D')
    % --- 2D FFT analysis ---
    if isempty(stack) || size(stack, 2) < 2
        error('At least 2 frames and a valid ROI are required for 2D FFT analysis.');
    end
    [space_dim, time_dim] = size(stack); % space_dim is ROI height, time_dim is the number of frames

    % Remove DC component from each row
    stack = stack - mean(stack, 2);

    F_raw = fft2(stack);
    F_shifted = fftshift(F_raw); % Shift zero-frequency component to the center
    F_magnitude = abs(F_shifted);

    % Estimate the main flicker frequency (extracted from the horizontal centerline of the 2D spectrum)
    center_row = floor(space_dim / 2) + 1;
    fft_line_time = F_magnitude(center_row, :); % Extract the center horizontal line
    L_time = time_dim;
    P2_time = fft_line_time / L_time; % Normalize (approximate)
    P1_time = P2_time(floor(L_time/2)+1:end); % Take the single-sided spectrum (positive frequency part)

    % Calculate the time frequency axis (Hz)
    f_axis_time_2d = frame_rate * (0:(ceil(L_time/2)-1)) / L_time;

    % Find the time frequency peak (only find the maximum value for frequencies greater than or equal to 25Hz)
    freq_mask_2d = f_axis_time_2d >= 25;
    if any(freq_mask_2d)
        [max_amp_time, idx_masked_2d] = max(P1_time(freq_mask_2d));
        idx_candidates_2d = find(freq_mask_2d);
        idx_time = idx_candidates_2d(idx_masked_2d);
    else
        [max_amp_time, idx_time] = max(P1_time);
    end
    peak_freq_time_2d = f_axis_time_2d(idx_time);

    fprintf('Estimated main flicker frequency from 2D FFT horizontal centerline: %.2f Hz\n', peak_freq_time_2d);
    
    % Save 2D FFT results to structure
    fft_results.type_2d = '2D';
    fft_results.F_magnitude = F_magnitude;
    fft_results.freq_2d = f_axis_time_2d;
    fft_results.amp_2d = P1_time;
    fft_results.peak_freq_2d = peak_freq_time_2d;
    fft_results.peak_amp_2d = max_amp_time;
    
    % Display the original space-time image (optional, not part of the FFT results)
    figure('Name', 'Original Space-Time Image', 'Position', [100, 100, 800, 600]);
    imagesc(stack);
    colormap('gray');
    colorbar;
    title(['Stacked Strip Image (Space vs. Time) - ', display_video_name]);
    xlabel('Time (Frame Number)');
    ylabel('Space (ROI Rows)');
    axis xy;
    saveas(gcf, fullfile(results_folder, '2D_space_time.png'));
    saveas(gcf, fullfile(results_folder, '2D_space_time.fig'));

else
    error('Invalid analysis_dimension. Choose ''1D'' or ''2D''.');
end

% --- Unified display of all FFT results ---
% Create a figure to display all FFT spectrum results
h_fig_fft = figure('Name', 'FFT Spectrum Analysis Results', 'Position', [100, 100, 1000, 800]);

% Determine the number and layout of subplots based on the analysis dimension
if strcmpi(analysis_dimension, '1D')
    % Only display 1D FFT results
    plot(fft_results.freq_1d, fft_results.amp_1d, 'b-', 'LineWidth', 1.5);
    title(['1D FFT Spectrum Analysis - ', display_video_name]);
    xlabel('Frequency (Hz)');
    ylabel('|FFT(Brightness)|');
    xlim([0 150]); % Limit the frequency range to 0-150Hz
    grid on;
    set(gca, 'NextPlot', 'add'); % hold on
    % Mark the peak
    plot(fft_results.peak_freq_1d, fft_results.peak_amp_1d, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    text(fft_results.peak_freq_1d, fft_results.peak_amp_1d, sprintf(' %.2f Hz', fft_results.peak_freq_1d), ...
         'VerticalAlignment', 'bottom', 'HorizontalAlignment', 'left', 'FontSize', 24);
    set(gca, 'NextPlot', 'replace'); % hold off
    
elseif strcmpi(analysis_dimension, '2D')
    % Create a 2x2 layout
    % 1. 2D FFT magnitude spectrum
    subplot(2, 2, 1);
    imagesc(log(fft_results.F_magnitude + 1));
    colormap(gca, 'jet');
    colorbar;
    title(['2D FFT Magnitude Spectrum (Log Scale) - ', display_video_name]);
    xlabel('Time Frequency Component');
    ylabel('Space Frequency Component');
    axis xy;
    
    % 2. 1D frequency spectrum extracted from 2D FFT
    subplot(2, 2, 2);
    plot(fft_results.freq_2d, fft_results.amp_2d, 'b-', 'LineWidth', 1.5);
    title(['Frequency Spectrum Extracted from 2D FFT Center Horizontal Line - ', display_video_name]);
    xlabel('Frequency (Hz)');
    ylabel('|FFT Component|');
    xlim([0 120]);
    grid on;
    set(gca, 'NextPlot', 'add'); % hold on
    % Mark the peak
    plot(fft_results.peak_freq_2d, fft_results.peak_amp_2d, 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    text(fft_results.peak_freq_2d, fft_results.peak_amp_2d, sprintf(' %.2f Hz', fft_results.peak_freq_2d), ...
         'VerticalAlignment', 'bottom', 'HorizontalAlignment', 'left', 'FontSize', 12);
    set(gca, 'NextPlot', 'replace'); % hold off
    
    % 3. Visualization of the horizontal centerline of the 2D FFT
    subplot(2, 2, 3);
    center_row = floor(size(fft_results.F_magnitude, 1) / 2) + 1;
    plot(fft_results.F_magnitude(center_row, :));
    title(['2D FFT Horizontal Centerline Profile - ', display_video_name]);
    xlabel('Frequency Index');
    ylabel('Magnitude');
    grid on;
    
    % 4. Visualization of the vertical centerline of the 2D FFT
    subplot(2, 2, 4);
    center_col = floor(size(fft_results.F_magnitude, 2) / 2) + 1;
    plot(fft_results.F_magnitude(:, center_col));
    title(['2D FFT Vertical Centerline Profile - ', display_video_name]);
    xlabel('Space Index');
    ylabel('Magnitude');
    grid on;
end

% Save the FFT analysis result figure
saveas(h_fig_fft, fullfile(results_folder, 'FFT_analysis.png'));
saveas(h_fig_fft, fullfile(results_folder, 'FFT_analysis.fig'));

% --- Step 6: Interpretation ---
fprintf('\n--- Result Interpretation ---\n');
fprintf('Video frame rate: %.2f Hz\n', frame_rate);
fprintf('Analysis dimension: %s\n', analysis_dimension);
if strcmpi(analysis_dimension, '1D')
    fprintf('Detected main frequency: %.2f Hz\n', peak_freq_1d);
elseif strcmpi(analysis_dimension, '2D')
    fprintf('Estimated main flicker frequency from 2D FFT: %.2f Hz\n', peak_freq_time_2d);
    fprintf('The peak on the horizontal axis of the 2D FFT spectrum reflects the flicker frequency over time.\n');
    fprintf('The peak on the vertical axis (if any) reflects the spatial periodicity within the ROI.\n');
end

fprintf('Analysis complete. Results have been saved to: %s\n', results_folder);
