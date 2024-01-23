const fs = require('fs');
const xml = require('xml2js');

// Converts the object from its current version to the newest available data version
function convert(obj) {
  // The current data version
  const CurrentVersion = 1;

  if ("version" in obj && obj.version == CurrentVersion) {
    // We are at the most recent version already
    return false;
  }

  // If the version key doesn't exist, we assume that we were handed an XML file that
  // didn't have the version information in it
  if (!("version" in obj)) {
    function rename(obj, old_key, new_key) {
      if (old_key in obj) {
        Object.defineProperty(obj, new_key, Object.getOwnPropertyDescriptor(obj, old_key));
        delete obj[old_key];
      }
    }
  
    function toString(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = String(obj[key][0]);
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toNumber(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = Number(obj[key][0]);
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toBoolean(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = (obj[key][0] === "true");
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toVec2(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = {
          x: Number(obj[key][0]["x"]),
          y: Number(obj[key][0]["y"])
        }
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toVec3(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = {
          x: Number(obj[key][0]["x"]),
          y: Number(obj[key][0]["y"]),
          z: Number(obj[key][0]["z"])
        }
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toMat4(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
  
        let values = [];
        toBoolean(obj, "transpose");
        if ("transpose" in obj) {
          values.push(Number(obj[key][0].x0));
          values.push(Number(obj[key][0].x1));
          values.push(Number(obj[key][0].x2));
          values.push(Number(obj[key][0].x3));
          values.push(Number(obj[key][0].y0));
          values.push(Number(obj[key][0].y1));
          values.push(Number(obj[key][0].y2));
          values.push(Number(obj[key][0].y3));
          values.push(Number(obj[key][0].z0));
          values.push(Number(obj[key][0].z1));
          values.push(Number(obj[key][0].z2));
          values.push(Number(obj[key][0].z3));
          values.push(Number(obj[key][0].w0));
          values.push(Number(obj[key][0].w1));
          values.push(Number(obj[key][0].w2));
          values.push(Number(obj[key][0].w3));
        }
        else {
          values.push(Number(obj[key][0].x0));
          values.push(Number(obj[key][0].y0));
          values.push(Number(obj[key][0].z0));
          values.push(Number(obj[key][0].w0));
          values.push(Number(obj[key][0].x1));
          values.push(Number(obj[key][0].y1));
          values.push(Number(obj[key][0].z1));
          values.push(Number(obj[key][0].w1));
          values.push(Number(obj[key][0].x2));
          values.push(Number(obj[key][0].y2));
          values.push(Number(obj[key][0].z2));
          values.push(Number(obj[key][0].w2));
          values.push(Number(obj[key][0].x3));
          values.push(Number(obj[key][0].y3));
          values.push(Number(obj[key][0].z3));
          values.push(Number(obj[key][0].w3));
        }
        obj[key] = values;
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }
  
    function toObject(obj, key, maybe_new_key) {
      if (key in obj) {
        if (typeof(obj[key]) !== "object")  throw `Wrong type for key ${key}`;
        if (obj[key].length !== 1)  throw `Wrong length for key ${key}`;
        obj[key] = obj[key][0];
      }
  
      if (typeof(maybe_new_key) !== "undefined") {
        rename(obj, key, maybe_new_key);
      }
    }

    obj.version = CurrentVersion;

    toString(obj, "masterAddress", "masteraddress");
    toNumber(obj, "setThreadAffinity", "threadaffinity");
    toBoolean(obj, "debugLog", "debuglog");
    toNumber(obj, "externalControlPort", "externalcontrolport");
    toBoolean(obj, "firmSync", "firmsync");

    toObject(obj, "Scene", "scene");
    if ("scene" in obj) {
      toVec3(obj.scene, "Offset", "offset");

      toObject(obj.scene, "Orientation", "orientation");
      if ("orientation" in obj.scene) {
        Object.keys(obj.scene.orientation).forEach((key, _) => {
          toNumber(obj.scene.orientation, key);
        });
      }

      rename(obj.scene, "Scale", "scale");
      if ("scale" in obj.scene) {
        obj.scene.scale = obj.scene.scale[0].value;
        toNumber(obj.scene, "scale");
      }
      toNumber(obj.scene, "Scale", "scale");
    }
    
    rename(obj, "User", "users");
    if ("users" in obj) {
      obj.users.forEach((user, _) => {
        toString(user, "name");
        toNumber(user, "eyeSeparation", "eyeseparation");
        
        toVec3(user, "Pos", "pos");

        toObject(user, "Orientation", "orientation");
        if ("orientation" in user) {
          Object.keys(user.orientation).forEach((key, _) => {
            toNumber(user.orientation, key);
          });
        }

        toMat4(user, "Matrix", "matrix");

        toObject(user, "Tracking", "tracking");
        if ("tracking" in user) {
          Object.keys(user.tracking).forEach((key, _) => {
            toString(user.tracking, key);
          });
        }
      });
    }

    toObject(obj, "Settings", "settings");
    if ("settings" in obj) {
      toBoolean(obj.settings, "DepthBufferTexture", "depthbuffertexture");
      toBoolean(obj.settings, "NormalTexture", "normaltexture");
      toBoolean(obj.settings, "PositionTexture", "positiontexture");
      toNumber(obj.settings, "Precision", "precision");

      toObject(obj.settings, "Display", "display");
      if ("display" in obj.settings) {
        toNumber(obj.settings.display, "swapInterval", "swapinterval");
        toNumber(obj.settings.display, "refreshRate", "refreshrate");
      }
    }

    rename(obj, "Capture", "capture");
    toObject(obj, "capture");
    if ("capture" in obj) {
      toString(obj.capture, "path");
      toString(obj.capture, "format");
      toNumber(obj.capture, "range-begin", "rangebegin");
      toNumber(obj.capture, "range-end", "rangeend");
    }

    rename(obj, "Tracker", "trackers");
    if ("trackers" in obj) {
      obj.trackers.forEach((tracker, _) => {
        toString(tracker, "name");

        rename(tracker, "Device", "devices");
        if ("devices" in tracker) {
          tracker.devices.forEach((device, _) => {
            toString(device, "name");

            rename(device, "Sensor", "sensors");
            if ("sensors" in device) {
              device.sensors.forEach((sensor, _) => {
                toString(sensor, "vrpnAddress", "vrpnaddress");
                toNumber(sensor, "id");
              });
            }

            rename(device, "Buttons", "buttons");
            if ("buttons" in device) {
              device.buttons.forEach((button, _) => {
                toString(button, "vrpnAddress", "vrpnaddress");
                toNumber(button, "count");
              });
            }
            
            rename(device, "Axes", "axes");
            if ("axes" in device) {
              devices.axes.forEach((axis, _) => {
                toString(axis, "vrpnAddress", "vrpnaddress");
                toNumber(axis, "count");
              });
            }

            toVec3(device, "Offset", "offset");
              
            toObject(device, "Orientation", "orientation");
            if ("orientation" in device) {
              Object.keys(device.orientation).forEach((key, _) => {
                toNumber(device.orientation, key);
              });
            }

            toMat4(device, "Matrix", "matrix");
          });
        }

        toVec3(tracker, "Offset", "offset");

        toObject(tracker, "Orientation", "orientation");
        if ("orientation" in tracker) {
          Object.keys(tracker.orientation).forEach((key, _) => {
            toNumber(tracker.orientation, key);
          });
        }

        toNumber(tracker, "Scale", "scale");
        toMat4(tracker, "Matrix", "matrix");
      });
    }


    rename(obj, "Node", "nodes");
    obj.nodes.forEach((node, _) => {
      toString(node, "address");
      toNumber(node, "port");
      toNumber(node, "dataTransferPort", "datatransferport");
      toBoolean(node, "swapLock", "swaplock");

      rename(node, "Window", "windows");
      node.windows.forEach((window, _) => {
        toNumber(window, "id");
        toString(window, "name");

        // tags change from "abc,bcd,cde" -> [ "abc", "bcd", "cde"]
        toString(window, "tags");
        if ("tags" in window) {
          window.tags = window.tags.split(",");
        }

        toString(window, "bufferBitDepth", "bufferbitdepth");
        rename(window, "fullScreen", "fullscreen");
        toBoolean(window, "fullscreen");
        toBoolean(window, "autoiconify");
        toBoolean(window, "hideMouseCursor", "hidemousecursor");
        toBoolean(window, "floating");
        toBoolean(window, "alwaysRender", "alwaysrender");
        toBoolean(window, "hidden");
        toBoolean(window, "dbuffered", "doublebuffered");

        rename(window, "numberOfSamples", "msaa");
        toNumber(window, "msaa");
        toBoolean(window, "alpha");
        toBoolean(window, "fxaa");

        rename(window, "decorated", "border");
        toBoolean(window, "border");

        toBoolean(window, "mirror");
        
        rename(window, "draw2D", "draw2d");
        toBoolean(window, "draw2d");
        
        rename(window, "draw3D", "draw3d");
        toBoolean(window, "draw3d");
        toNumber(window, "blitWindowId", "blitwindowid");
        toNumber(window, "monitor");

        // The stereo value was stored as the "type" parameter before, but we can compress
        // that value down
        rename(window, "Stereo", "stereo");
        if ("stereo" in window) {
          window.stereo = window.stereo[0].type;
          toString(window, "stereo");
        }
        
        toVec2(window, "Pos", "pos");
        toVec2(window, "Size", "size");
        toVec2(window, "Res", "res");

        rename(window, "Viewport", "viewports");
        window.viewports.forEach((viewport, _) => {
          toString(viewport, "user");
          toString(viewport, "overlay");
          rename(viewport, "mask", "blendmask")
          toString(viewport, "BlendMask", "blendmask");
          toString(viewport, "BlackLevelMask", "blacklevelmask");
          toString(viewport, "mesh");
          toBoolean(viewport, "tracked");
          toString(viewport, "eye");
          toVec2(viewport, "Pos", "pos");
          toVec2(viewport, "Size", "size");

          // This value used to be supported, but is not even support in XML anymore, so
          // we can just remove it altogether
          if ("name" in viewport) {
            delete viewport.name;
          }

          if ("PlanarProjection" in viewport) {
            toObject(viewport, "PlanarProjection", "projection");
            viewport.projection.type = "PlanarProjection";

            // FOV -> fov
            toObject(viewport.projection, "FOV", "fov");
            if ("fov" in viewport.projection) {
              Object.keys(viewport.projection.fov).forEach((key, _) => {
                toNumber(viewport.projection.fov, key);
              });

              if (viewport.projection.fov.left === viewport.projection.fov.right) {
                viewport.projection.fov.hfov =
                  viewport.projection.fov.left + viewport.projection.fov.right;

                delete viewport.projection.fov.left;
                delete viewport.projection.fov.right;
              }

              if (viewport.projection.fov.down === viewport.projection.fov.up) {
                viewport.projection.fov.vfov =
                  viewport.projection.fov.down + viewport.projection.fov.up;

                delete viewport.projection.fov.down;
                delete viewport.projection.fov.up;
              }
            }

            // Orientation -> orientation
            toObject(viewport.projection, "Orientation", "orientation");
            if ("orientation" in viewport.projection) {
              Object.keys(viewport.projection.orientation).forEach((key, _) => {
                toNumber(viewport.projection.orientation, key);
              });
            }

            // Offset -> offset
            toObject(viewport.projection, "Offset", "offset");
            if ("offset" in viewport.projection) {
              Object.keys(viewport.projection.offset).forEach((key, _) => {
                toNumber(viewport.projection.offset, key);
              });
            }
          }
          else if ("TextureMappedProjection" in viewport) {
            toObject(viewport, "TextureMappedProjection", "projection");
            viewport.projection.type = "TextureMappedProjection";

            // FOV -> fov
            toObject(viewport.projection, "FOV", "fov");
            if ("fov" in viewport.projection) {
              Object.keys(viewport.projection.fov).forEach((key, _) => {
                toNumber(viewport.projection.fov, key);
              });

              if (viewport.projection.fov.left === viewport.projection.fov.right) {
                viewport.projection.fov.hfov =
                  viewport.projection.fov.left + viewport.projection.fov.right;

                delete viewport.projection.fov.left;
                delete viewport.projection.fov.right;
              }

              if (viewport.projection.fov.down === viewport.projection.fov.up) {
                viewport.projection.fov.vfov =
                  viewport.projection.fov.down + viewport.projection.fov.up;

                delete viewport.projection.fov.down;
                delete viewport.projection.fov.up;
              }
            }

            // Orientation -> orientation
            toObject(viewport.projection, "Orientation", "orientation");
            if ("orientation" in viewport.projection) {
              Object.keys(viewport.projection.orientation).forEach((key, _) => {
                toNumber(viewport.projection.orientation, key);
              });
            }

            // Offset -> offset
            toObject(viewport.projection, "Offset", "offset");
            if ("offset" in viewport.projection) {
              Object.keys(viewport.projection.offset).forEach((key, _) => {
                toNumber(viewport.projection.offset, key);
              });
            }
          }
          else if ("FisheyeProjection" in viewport) {
            toObject(viewport, "FisheyeProjection", "projection");
            viewport.projection.type = "FisheyeProjection";
            
            toNumber(viewport.projection, "fov");
            toString(viewport.projection, "quality");
            toString(viewport.projection, "interpolation");
            toNumber(viewport.projection, "diameter");
            toNumber(viewport.projection, "tilt");

            // Crop -> copy
            toObject(viewport.projection, "Crop", "crop");
            if ("crop" in viewport.projection) {
              Object.keys(viewport.projection.crop).forEach((key, _) => {
                toNumber(viewport.projection.crop, key);
              });
            }

            toBoolean(viewport.projection, "keepAspectRatio", "keepaspectratio");

            // Offset -> offset
            toVec3(viewport.projection, "Offset", "offset");

            // Background -> background
            toObject(viewport.projection, "Background", "background");
            if ("background" in viewport.projection) {
              Object.keys(viewport.projection.background).forEach((key, _) => {
                toNumber(viewport.projection.background, key);
              });
            }
          }
          else if ("SphericalMirrorProjection" in viewport) {
            toObject(viewport, "SphericalMirrorProjection", "projection");
            viewport.projection.type = "SphericalMirrorProjection";
            
            toString(viewport.projection, "quality");
            toNumber(viewport.projection, "tilt");

            // Background -> background
            toObject(viewport.projection, "Background", "background");
            if ("background" in viewport.projection) {
              Object.keys(viewport.projection.background).forEach((key, _) => {
                toNumber(viewport.projection.background, key);
              });
            }

            // Geometry -> geometry
            toObject(viewport.projection, "Geometry", "geometry");
            if ("geometry" in viewport.projection) {
              Object.keys(viewport.projection.geometry).forEach((key, _) => {
                toString(viewport.projection.geometry, key);
              });
            }
          }
          else if ("SpoutOutputProjection" in viewport) {
            toObject(viewport, "SpoutOutputProjection", "projection");
            viewport.projection.type = "SpoutOutputProjection";
            
            toString(viewport.projection, "quality");
            toNumber(viewport.projection, "tilt");
            toString(viewport.projection, "mapping");
            toString(viewport.projection, "mappingSpoutName", "mappingspoutname");

            // Background -> background
            toObject(viewport.projection, "Background", "background");
            if ("background" in viewport.projection) {
              Object.keys(viewport.projection.background).forEach((key, _) => {
                toNumber(viewport.projection.background, key);
              });
            }

            // Channels -> channels
            toObject(viewport.projection, "Channels", "channels");
            if ("channels" in viewport.projection) {
              toBoolean(viewport.projection.channels, "Right", "right")
              toBoolean(viewport.projection.channels, "zLeft", "zleft");
              toBoolean(viewport.projection.channels, "Bottom", "bottom");
              toBoolean(viewport.projection.channels, "Top", "top");
              toBoolean(viewport.projection.channels, "Left", "left");
              toBoolean(viewport.projection.channels, "zRight", "zright");
            }

            // RigOrientation -> orientation
            toObject(viewport.projection, "RigOrientation", "orientation");
            if ("orientation" in viewport.projection) {
              Object.keys(viewport.projection.orientation).forEach((key, _) => {
                toNumber(viewport.projection.orientation, key);
              });
            }
          }
          else if ("CylindricalProjection" in viewport) {
            toObject(viewport, "CylindricalProjection", "projection");
            viewport.projection.type = "CylindricalProjection";

            toString(viewport.projection, "quality");
            toNumber(viewport.projection, "tilt");

            toNumber(viewport.projection, "rotation");
            toNumber(viewport.projection, "heightOffset", "heightoffset");
            toNumber(viewport.projection, "radius");
          }
          else if ("EquirectangularProjection" in viewport) {
            toObject(viewport, "EquirectangularProjection", "projection");
            viewport.projection.type = "EquirectangularProjection";
            
            toString(viewport.projection, "quality");
            toNumber(viewport.projection, "tilt");

            // rename and set type
            rename(viewport, "EquirectangularProjection", "projection");
            viewport.projection.type = "EquirectangularProjection";
          }
          else if ("Projectionplane" in viewport) {
            toObject(viewport, "Projectionplane", "projection");
            viewport.projection.type = "ProjectionPlane";
            
            viewport.projection.lowerleft = viewport.projection.Pos[0];
            Object.keys(viewport.projection.lowerleft).forEach((key, _) => {
              toNumber(viewport.projection.lowerleft, key);
            });

            viewport.projection.upperleft = viewport.projection.Pos[1];
            Object.keys(viewport.projection.upperleft).forEach((key, _) => {
              toNumber(viewport.projection.upperleft, key);
            });
            
            viewport.projection.upperright = viewport.projection.Pos[2];
            Object.keys(viewport.projection.upperright).forEach((key, _) => {
              toNumber(viewport.projection.upperright, key);
            });

            delete viewport.projection.Pos;
          }
        });
      });
    });
  }
  
  if (obj.version === 1) {
    // Nothing to do here since we are at the most recent version

  }

  return true;
}

if (process.argv.length !== 3) {
  console.error("Script must be started with the input configuration as input");
  process.exit();
}

const input = process.argv[2];
if (!fs.existsSync(input)) {
  console.error(`Could not find file ${input}`);
  process.exit();
}
console.log(`Loading ${input}`);

let components = input.split('.');
const extension = components.pop();

let output = components.join('.') + ".json";

if (extension == "xml") {
  var parser = new xml.Parser({
    explicitRoot: false,
    explicitArray: true,
    mergeAttrs: true
  });
  fs.readFile(input, (err, data) => {
    parser.parseString(data, (err, result) => {
      convert(result);

      console.log(`Writing ${output}`);
      fs.writeFileSync(output, JSON.stringify(result, null, 2));
    });
  });
}
else if (extension == "json") {
  fs.readFile(input, (err, data) => {
    let d = JSON.parse(data);
    let has_changed = convert(d);

    if (has_changed) {
      console.log(`Writing ${output}`);
      fs.writeFileSync(output, JSON.stringify(d, null, 2));
    }
  });
}
