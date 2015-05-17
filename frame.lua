--[[ frame

The 'frame' package has two public functions. The first is the 'init' function
that takes as arguments the application options table and a second source
definitions table. The second public function is a 'forward' function that is
created by the the 'init' function using the options/source tables to select an
appropriate method. It is this 'forward' function that is called by the
applications 'main' loop to grab an image frame.

--]]
local frame = {}
torch.setdefaulttensortype('torch.FloatTensor')

local sys = assert(require('sys'))

local pf = function(...) print(string.format(...)) end
local Cr = sys.COLORS.red
local Cb = sys.COLORS.blue
local Cg = sys.COLORS.green
local Cn = sys.COLORS.none
local THIS = sys.COLORS.blue .. 'THIS' .. Cn


local function prep_lua_camera(opt, source)
   assert(require('camera'))
   local vid = assert(require('libvideo_decoder'))

   local cam = image.Camera {
      idx      = opt.cam,
      width    = source.w,
      height   = source.h,
   }

   -- set frame forward function
   frame.forward = function(img)
      return cam:forward(img)
   end

   -- load postfile routine from libvideo_decoder library:
   cam.postfile = vid.postfile

   source.cam = cam
end


local function prep_libcamera_tensor(opt, source)
   local cam = assert(require("libcamera_tensor"))

   if not cam.init(opt.cam, source.w, source.h, opt.fps, 1) then
      error("No camera")
   end

   -- set frame forward function
   frame.forward = function(img)
      if not cam.frame_rgb(opt.cam, img) then
         error('frame grab failed')
      end

      return img
   end

   source.cam = cam
end


local function prep_libvideo_decoder_camera(opt, source)
   local cam = assert(require('libvideo_decoder'))
   cam.loglevel(opt.loglevel)

   local status = false
   if opt.wthumbs ~= 2 then
      -- buffers default == 1
      status = cam.capture('/dev/video'..opt.cam, source.w, source.h, source.fps)
      if not status then
         error('cam.capture failed')
      end
   else
      -- if we want to stream detections to server:
      status = cam.capture('/dev/video'..opt.cam, source.w, source.h, source.fps, 16, 'auto', 25)
      if not status then
         error('cam.capture failed')
      end

      -- remux video if we want to save video segments
      local strstr = '|url=' .. userdata.serverUrl .. '/api/upload' .. '||'
      strstr = strstr .. 'username=' .. userdata.login
      strstr = strstr .. '|password=' .. userdata.pw
      print('uploading video to: ', strstr)

      status = cam.startremux(strstr, 'mp4')
      if status == 0 then
         error('cam.startremux failed')
      end
   end

   -- video library only handles byte tensors
   local img_tmp = torch.FloatTensor(3, source.h, source.w)

   -- set frame forward function
   frame.forward = function(img)
      if not cam.frame_rgb(img_tmp) then
         return false
      end
      img = img_tmp:clone()

      return img
   end

   source.cam = cam
end


function frame:init(opt, source)
   if (opt.input == 'usbcam') and (sys.OS == 'macos') then

      prep_lua_camera(opt, source)
      
   elseif (opt.input == 'usbcam') then

      prep_libvideo_decoder_camera(opt, source)

   else
      error("<ERROR frame> Unsupported platform and frame source combination")
   end
end


return frame
