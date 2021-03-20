# Python API

API for `cvvidproc` Python module.



## Background Image

Purpose: get the background of a video (or cropped view of video)

Function calls:
- `GetVideoBackground(VidBgPack pack)`
	- Inputs:
		- `pack`: a package of input variables
	- Returns: A `numpy` array representation of the background image (convertible to an OpenCV `Mat`)

Structures/Classes:
- `VidBgPack`
	- Parameters (with default values):
		- `vid_path`: *String*, Full system path to video that should be analyzed
		- `bg_algo` = "hist": *String*, Algorithm for obtaining background image; available algorithms:
			- `hist`: Histogram-based median of pixel values (per-channel median).
		- `max_threads` = -1: *Int*, Maximum number of threads to use while computing background image
		- `frame_limit` = -1: *Int*, Maximum number of frames in video to use while computing background image
		- `grayscale` = false: *Bool*, Whether to interpret the video has grayscale
		- `vid_is_grayscale` = false: *Bool*, Whether the video should be treated as already grayscale (optimization)
		- `crop_x` = 0: *Int*, Horizontal position of upper left corner of crop-view
		- `crop_y` = 0: *Int*, Vertical position of upper left corner of crop-view
		- `crop_width` = 0: *Int*, Width of crop-view
		- `crop_height` = 0: *Int*, Height of crop-view
		- `token_storage_limit` = 10: *Int*, Maximum number of frames to store at a time (use lower values if program is using too much RAM, otherwise ignore)
		- `print_timing_report` = false: *Bool*, Whether to print a timing report about the algorithm's performance


### Example Use

```
import cvvidproc

def compute_bkgd_med_thread(vid_path, vid_is_grayscale, num_frames=100, 
			crop_x=0, crop_y=0, crop_width=0, crop_height=0):
    """
    Calls multithreaded bkgd algo.
    """

    # computes the median
    vidpack = cvvidproc.VidBgPack(
        vid_path = vid_path,
        frame_limit = num_frames, #(default = -1 -> all frames),
        vid_is_grayscale = vid_is_grayscale,
        crop_x = crop_x,
        crop_y = crop_y,
        crop_width = crop_width, #(default = 0)
        crop_height = crop_height, #(default = 0)
        print_timing_report = True)

    print('getting video background')
    start_time = time.time()

    bkgd_med = cvvidproc.GetVideoBackground(vidpack)

    end_time = time.time()
    print('video background obtained ({0:f} s)'.format(end_time - start_time))

    # takes value channel if color image provided
    if len(bkgd_med.shape) == 3:
        bkgd_med = get_val_channel(bkgd_med)

    return bkgd_med
```



## Object Tracking

Purpose: track objects in a video with user-defined object tracking method

Function calls:
- `TrackObjects(VidObjectTrackPack pack)`
	- Inputs:
		- `pack`: a package of input variables
	- Returns: A Python Dictionary containing all objects tracked in video

Structures/Classes:
- `VidObjectTrackPack`
	- Parameters (with default values):
		- `vid_path`: *String*, Full system path to video that should be analyzed
		- `highlight_objects_pack`: *HighlightObjectsPack*, Variable pack for highlighting objects
		- `assign_objects_pack`: *AssignObjectsPack*, Variable pack for assigning objects
		- `max_threads` = -1: *Int*, Maximum number of threads to use
		- `frame_limit` = -1: *Int*, Maximum number of frames in video to use
		- `grayscale` = false: *Bool*, Whether to interpret the video has grayscale
		- `vid_is_grayscale` = false: *Bool*, Whether the video should be treated as already grayscale (optimization)
		- `crop_x` = 0: *Int*, Horizontal position of upper left corner of crop-view
		- `crop_y` = 0: *Int*, Vertical position of upper left corner of crop-view
		- `crop_width` = 0: *Int*, Width of crop-view
		- `crop_height` = 0: *Int*, Height of crop-view
		`token_storage_limit` = 10: *Int*, Maximum number of frames to store at a time (use lower values if program is using too - much RAM, otherwise ignore)
		- `print_timing_report` = false: *Bool*, Whether to print a timing report about the algorithm's performance

- `HighlightObjectsPack`
	- Parameters (no defaults):
		- `background`: *Image*, Background of video to highlight objects in
		- `struct_element`: *Image*
		- `threshold`: *Int*
		- `threshold_lo`: *Int*
		- `threshold_hi`: *Int*
		- `min_size_hyst`: *Int*
		- `min_size_threshold`: *Int*
		- `width_border`: *Int*

- `AssignObjectsPack`
	- Parameters (no defaults):
		- `function`: *functor*, Python function object for assigning objects in highlighted frames; expected signature/behavior:
			- `next_ID = func(bkgd_frame, frames_processed, objects_prev, objects_archive, next_ID, kwargs)`
		- `args`: *Dictionary*, Dictionary of args to forward to function

### Example Use

```
import cvvidproc


def assign_bubbles_cvvidproc(bkgd_frame, frames_processed, objects_prev, objects_archive, next_ID, kwargs):
    # wrapper on assign_bubbles() to meet cvvidproc API expectation

    return assign_bubbles(bkgd_frame, frames_processed, objects_prev, objects_archive, next_ID, **kwargs)


def track_bubble_cvvidproc(track_kwargs, highlight_kwargs, assignbubbles_kwargs):
    highlightpack = cvvidproc.HighlightObjectsPack(
        background=track_kwargs['bkgd'],
        struct_element=highlight_kwargs['selem'],
        threshold=highlight_kwargs['th'],
        threshold_lo=highlight_kwargs['th_lo'],
        threshold_hi=highlight_kwargs['th_hi'],
        min_size_hyst=highlight_kwargs['min_size_hyst'],
        min_size_threshold=highlight_kwargs['min_size_th'],
        width_border=highlight_kwargs['width_border'])

    assignpack = cvvidproc.AssignObjectsPack(
        assign_bubbles_cvvidproc,     # pass in function name as functor (not string)
        assignbubbles_kwargs)

    # fields not defined will be defaulted
    trackpack = cvvidproc.VidObjectTrackPack(
        vid_path=track_kwargs['vid_path'],
        highlight_objects_pack=highlightpack,
        assign_objects_pack=assignpack,
        frame_limit=track_kwargs['end']-track_kwargs['start'],
        vid_is_grayscale=True,
        crop_y=assignbubbles_kwargs['row_lo'],
        crop_height=assignbubbles_kwargs['row_hi']-assignbubbles_kwargs['row_lo'],
        print_timing_report=True)

    objects_archive = cvvidproc.TrackObjects(trackpack)

    return objects_archive
```



