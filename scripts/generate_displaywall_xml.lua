--This is a commandline script used to generate an .xml configuration file that can 
--be used in the config/ directory. The characteristics, resolution, and dimensions
--of the display panels in the wall are provided in a separate lua configuration file.

--USAGE
-- Arg 1 is a .lua file containing the configuration settings of the display wall.
--       Example files are located in the sgct scripts/ directory.
-- Arg 2 is the output filename (can be provided with or without the .xml extension)
--       which will be used in the sgct config/ directory.

--CONVENTIONS
-- Arrays, columns, and row numbers are 1-based
-- Each monitor is defined by 3 corner positions, in order of:
--   1) lower-left
--   2) upper-left
--   3) upper-right
-- Display wall has n vertical columns of m rows of monitors
--   Columns are numbered from left (#1) to right (from observer facing the wall).
--   Rows are numbered from top (#1) to bottom (from observer facing the wall).
-- The PHYSICAL coordinate system origin is at the lower-left corner of the wall.
-- The DISPLAY coordinate system origin is at the upper-left corner of the wall,
--   and the display origin of each monitor is at its own upper-left corner.
-- All units are in meters

function doesFileExist(filename)
    local f = io.open(filename, "r")
    if (f == nil) then
        return false
    else
        io.close(f)
	return true
    end
end

--Reference a lua configuration file here with display wall dimensions & setup
configFileInput = arg[1]
if (configFileInput == nil) then
    print("Error: A .lua file is required for display wall configuration as arg 1")
    os.exit(-1)
end
extension = string.sub(configFileInput, -4, -1)
sansExtension = string.sub(configFileInput, 1, -5)
if (extension ~= ".lua") then
    print("Error: A .lua file is required for display wall configuration as arg 1")
    os.exit(-1)
end
local cfg = require(sansExtension)

outputFile = arg[2]
if (outputFile == nil) then
    print("Error: An output .xml filename is required as arg 2")
    os.exit(-1)
end
if (string.sub(outputFile, -4) ~= ".xml") then
    outputFile = outputFile .. ".xml"
end
if (doesFileExist(outputFile)) then
    print("Error: An .xml configuration file of the same name already exists")
    os.exit(-1)
end

local totalGapHoriz = (cfg.displayWall_monitorGapAvgHorizontal
        * (cfg.displayWall_nColumns - 1))
local totalGapVert = (cfg.displayWall_monitorGapAvgVertical
        * (cfg.displayWall_nRows - 1)) 
local displayWallWidth = (cfg.monitorFrameWidth * cfg.displayWall_nColumns)
        + totalGapHoriz
local displayWallHeight = (cfg.monitorFrameHeight * cfg.displayWall_nRows)
        + totalGapVert
-- If even number of monitors then we don't want origin to be at bezel between monitors
if (cfg.displayWall_nRows / 2) == math.floor(cfg.displayWall_nRows / 2) then
    originVerticalOffsetToAvoidBezel = cfg.monitorFrameHeight / 2
        + cfg.displayWall_monitorGapAvgVertical / 2
else
    originVerticalOffsetToAvoidBezel = 0
end
local displayOrigin = {(displayWallWidth / 2),
                       (displayWallHeight / 2 + originVerticalOffsetToAvoidBezel)}
local borderLineHoriz = {}
local borderLineVert = {}
local displayWallPixelsTotal = {}
displayWallPixelsTotal[1] = cfg.displayWall_nColumns * cfg.monitorWallResolution[1]
displayWallPixelsTotal[2] = cfg.displayWall_nRows    * cfg.monitorWallResolution[2]
local masterAspectRatio = {(displayWallPixelsTotal[1] / displayWallPixelsTotal[2]), 1.0}

function computeBorderLines()
    for x = 1,(cfg.displayWall_nColumns + 1) do
        fractionOfWallWidth = cfg.scale[1] * (x - 1) / cfg.displayWall_nColumns
        distanceFromLeftSide = displayWallWidth * fractionOfWallWidth
        borderLineVert[x] = round(distanceFromLeftSide - displayOrigin[1], 4)
    end
    for y = 1,(cfg.displayWall_nRows + 1) do
        fractionOfWallHeight = cfg.scale[2] * (cfg.displayWall_nRows + 1 - y)
            / cfg.displayWall_nRows
        distanceFromBottom = displayWallHeight * fractionOfWallHeight
        borderLineHoriz[y] = round(distanceFromBottom - displayOrigin[2], 4)
    end
end

function computeWindowCoordinates(row, column, cornerName)
    if cornerName == "LL" then
        x = borderLineVert[column]   + cfg.monitorBezelWidth
        y = borderLineHoriz[row + 1] + cfg.monitorBezelHeight
    end
    if cornerName == "UL" then
        x = borderLineVert[column] + cfg.monitorBezelWidth
        y = borderLineHoriz[row]   - cfg.monitorBezelHeight
    end
    if cornerName == "UR" then
        x = borderLineVert[column + 1] - cfg.monitorBezelWidth
        y = borderLineHoriz[row]       - cfg.monitorBezelHeight
    end
    return {x, y}
end

function generateXmlForMonitorCorner(row, column, cornerName)
    local xy = computeWindowCoordinates(row, column, cornerName)
    resString = "<Pos x=\""
    if xy[1] >= 0.0 then
        resString = resString .. " "
    end
    resString = resString .. "" .. string.format("%.4f", xy[1]) .. "\" y=\""
    if xy[2] >= 0.0 then
        resString = resString .. " "
    end
    resString = resString .. ""..string.format("%.4f", xy[2]) .. "\" z=\"0.0\" />"
    return resString
end

function indent(level)
    line = ""
    spaces = 4
    for l = 1, level do
        for s=1,spaces do
            line = line .. " "
        end
    end
    return line
end

function wall_generateViewplane(row, column, lines, indentLevel)
    table.insert(lines,
                 indent(indentLevel) .. "<Viewplane>")
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(row, column, "LL"))
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(row, column, "UL"))
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(row, column, "UR"))
    table.insert(lines, indent(indentLevel) .. "</Viewplane>")
end

function wall_generateViewport(row, column, lines, indentLevel)
    table.insert(lines, indent(indentLevel) .. "<Viewport>")
    table.insert(lines, indent(indentLevel + 1) .. "<Pos  x=\"0.0\" y=\"0.0\" />")
    table.insert(lines, indent(indentLevel + 1) .. "<Size x=\"1.0\" y=\"1.0\" />")
    wall_generateViewplane(row, column, lines, indentLevel + 1)
    table.insert(lines, indent(indentLevel) .. "</Viewport>")
end

function wall_generateWindow(row, column, lines, indentLevel)
    table.insert(lines, indent(indentLevel)
                 .. "<Window fullScreen=\"false\" border=\"false\">")
    table.insert(lines, indent(indentLevel + 1)
                 .. "<Pos  x=\"0\" y=\""..cfg.monitorWallResolution[2]*(row - 1) .. "\" />")
    table.insert(lines, indent(indentLevel + 1) .. "<Size x=\""
                 .. cfg.monitorWallResolution[1]
                 .. "\" y=\""..cfg.monitorWallResolution[2] .. "\" />")
    wall_generateViewport(row, column, lines, indentLevel + 1)
    table.insert(lines, indent(indentLevel) .. "</Window>")
end

function master_generateViewplane(lines, indentLevel)
    table.insert(lines, indent(indentLevel) .. "<Viewplane>")
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(cfg.displayWall_nRows, 1, "LL"))
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(1, 1, "UL"))
    table.insert(lines, indent(indentLevel + 1)
                 .. generateXmlForMonitorCorner(1, cfg.displayWall_nColumns, "UR"))
    table.insert(lines, indent(indentLevel) .. "</Viewplane>")
end

function master_generateViewport(lines, indentLevel)
    table.insert(lines, indent(indentLevel) .. "<Viewport>")
    table.insert(lines, indent(indentLevel + 1) .. "<Pos  x=\"0.0\" y=\"0.0\" />")
    table.insert(lines, indent(indentLevel + 1) .. "<Size x=\"1.0\" y=\"1.0\" />")
    master_generateViewplane(lines, indentLevel + 1)
    table.insert(lines, indent(indentLevel) .. "</Viewport>")
end

function master_generateWindow(lines, indentLevel)
    table.insert(lines, indent(indentLevel) .. "<Window fullScreen=\"true\">")
    table.insert(lines, indent(indentLevel + 1) .. "<Pos  x=\"0.0\" y=\"0.0\" />")
    table.insert(lines, indent(indentLevel + 1) .. "<Size x=\""..cfg.masterResolution[1]
                 .. "\" y=\""..cfg.masterResolution[2] .. "\" />")
    master_generateViewport(lines, indentLevel + 1)
    table.insert(lines, indent(indentLevel) .. "</Window>")
end

function round(num, places)
  return tonumber(string.format("%." .. (places or 0) .. "f", num))
end

computeBorderLines()

local f = io.open(outputFile, "w")
f:write("<?xml version=\"1.0\" ?>\n")
f:write("<Cluster masterAddress=\""..cfg.masterHostname .. "\" firmSync=\"true\">\n")
f:write(indent(1) .. "<Settings>\n")
f:write(indent(2) .. "<Display swapInterval=\"0\" />\n")
f:write(indent(1) .. "</Settings>\n")
portNum = 20401
f:write(indent(1) .. "<Node address=\""..cfg.masterHostname .. "\" port=\""..portNum
      .. "\" swapLock=\"false\">\n")
masterChunk = {}
master_generateWindow(masterChunk, 2)
for _,l in ipairs(masterChunk) do
    f:write(l .. "\n")
end
f:write(indent(1).."</Node>\n")
portNum = portNum + 1
for c = 1, cfg.displayWall_nColumns do
    f:write(indent(1) .. "<!-- Node for column " .. c .. " -->\n")
    f:write(indent(1) .. "<Node address=\""..cfg.nodeHostnames[c] .. "\" port=\""
          .. portNum .. "\" swapLock=\"false\">\n")
    for r = 1, cfg.displayWall_nRows do
        chunk = {}
        wall_generateWindow(r, c, chunk, 2)
        rowHeader = "<!-- Window/Viewport/Viewplane for row " .. r .. " "
        if r == 1 then
            rowHeader = rowHeader .. "(top)"
        elseif r == cfg.displayWall_nRows then
            rowHeader = rowHeader .. "(bottom)"
        end
        rowHeader = rowHeader .. " -->"
        f:write(indent(2)..rowHeader .. "\n")
        for _,n in ipairs(chunk) do
            f:write(n .. "\n")
        end
    end
    f:write(indent(1) .. "</Node>\n")
    portNum = portNum + 1
end
f:write(indent(1) .. "<User eyeSeparation=\"0.065\">\n")
f:write(indent(2) .. "<Pos x=\"0.0\" y=\"0.0\" z=\"3.5\" />\n")
f:write(indent(1) .. "</User>\n")
f:write("</Cluster>\n")
f:close()
