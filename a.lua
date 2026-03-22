local luapython = require("luapython")

luapython.load()

local mediapipe = luapython.import("mediapipe")
local cv2 = luapython.import("cv2")
local python = mediapipe.python
local vision = mediapipe.vision

local base_options = python.BaseOptions({
	model_asset_path = "pose_landmarker.tasks",
})
