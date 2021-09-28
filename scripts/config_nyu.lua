local _M = {}

_M = {
    --Wall dimensions
    displayWall_nColumns = 5,
    displayWall_nRows = 4,
    --Measurements for individual monitor
    monitorFrameWidth = 0.7571,
    monitorFrameHeight = 0.4400,
    monitorBezelWidth = 0.024,
    monitorBezelHeight = 0.022,
    --Measure the average gap of empty space between monitors
    displayWall_monitorGapAvgHorizontal = 0.001,
    displayWall_monitorGapAvgVertical = 0.001,
    --Resolution of individual monitor in the display wall
    monitorWallResolution = {3840, 2160},
    --Resolution of master node display
    masterResolution = {1920, 1080},
    --Use scale to stretch the FOV (default is 1.0, 1.0)
    scale = {1.0, 1.0},
    --Network name (or IP) of master node, and each column node
    masterHostname = "127.24.113.129",
    nodeHostnames = {"172.24.113.130",
                     "172.24.113.131",
                     "172.24.113.132",
                     "172.24.113.133",
                     "172.24.113.134"}
}
return _M
