local _M = {}

_M = {
    --Wall dimensions
    displayWall_nColumns = 9,
    displayWall_nRows = 4,
    --Measurements for individual monitor
    monitorFrameWidth = 0.637,
    monitorFrameHeight = 0.378,
    monitorBezelWidth = 0.020,
    monitorBezelHeight = 0.021,
    --Measure the average gap of empty space between monitors
    displayWall_monitorGapAvgHorizontal = 0.002,
    displayWall_monitorGapAvgVertical = 0.001,
    --Resolution of individual monitor in the display wall
    monitorWallResolution = {2560, 1440},
    --Resolution of master node display
    masterResolution = {1920, 1080},
    --Use scale to stretch the FOV (default is 1.0, 1.0)
    scale = {1.0, 1.0},
    --Network name (or IP) of master node, and each column node
    masterHostname = "InsideExplorer",
    nodeHostnames = {"powerwall01",
                     "powerwall02",
                     "powerwall03",
                     "powerwall04",
                     "powerwall05",
                     "powerwall06",
                     "powerwall07",
                     "powerwall08",
                     "powerwall09"},
}
return _M
