/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__GLM__H__
#define __SGCT__GLM__H__

// All glm includes go through this header so that every translation unit, in SGCT and in
// applications that consume its headers, sees glm configured the same way

#ifndef GLM_META_PROG_HELPERS
#define GLM_META_PROG_HELPERS
#endif // GLM_META_PROG_HELPERS

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif // GLM_ENABLE_EXPERIMENTAL

#ifndef GLM_FORCE_CTOR_INIT
#define GLM_FORCE_CTOR_INIT
#endif // GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#endif // __SGCT__GLM__H__
