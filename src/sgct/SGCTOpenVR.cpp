/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTOpenVR.h>
#ifdef __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#undef __gl_h_
#endif
#include <sgct/SGCTWindow.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/OffScreenBuffer.h>

namespace {
    struct FBODesc {
        unsigned int fboID;
        unsigned int texID;
    };

    bool isOpenVRInitalized = false;
    vr::IVRSystem* HMD = nullptr;

    FBODesc leftEyeFBODesc;
    FBODesc rightEyeFBODesc;

    // Matries updated every rendering cycle
    glm::mat4 poseHMDMat;

    // Matrices updated on statup
    glm::mat4 eyeLeftProjectionMat;
    glm::mat4 eyeLeftToHeadMat;
    glm::mat4 eyeRightProjectionMat;
    glm::mat4 eyeRightToHeadMat;

    bool createHMDFrameBuffer(int width, int height, FBODesc& fboDesc) {
        glGenFramebuffers(1, &fboDesc.fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, fboDesc.fboID);

        glGenTextures(1, &fboDesc.texID);
        glBindTexture(GL_TEXTURE_2D, fboDesc.texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            fboDesc.texID,
            0
        );

        // check FBO status
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return true;
    }

    glm::mat4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose) {
        glm::mat4 matrixObj(
            matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
            matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
            matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
            matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
        );
        return glm::inverse(matrixObj);
    }
} // namespace

namespace sgct::openvr {

void initialize(float nearClip, float farClip) {
    if (isOpenVRInitalized) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "OpenVR has already been initialized\n"
        );
        return;
    }
        
    const bool isHMDconnected = vr::VR_IsHmdPresent();

    if (!isHMDconnected) {
        return;
    }

    // Loading the SteamVR Runtime
    vr::EVRInitError eError = vr::VRInitError_None;
    HMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None) {
        shutdown();
        std::string err = "Unable to init VR runtime: ";
        err += vr::VR_GetVRInitErrorAsEnglishDescription(eError);
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "VR_Init Failed", err.c_str()
        );
    }
    else {
        isOpenVRInitalized = true;

        unsigned int renderWidth;
        unsigned int renderHeight;
        HMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "OpenVR render dimensions per eye: %d x %d\n", renderWidth, renderHeight
        );

        // Create FBO and Texture used for sending data top HMD
        createHMDFrameBuffer(renderWidth, renderHeight, leftEyeFBODesc);
        createHMDFrameBuffer(renderWidth, renderHeight, rightEyeFBODesc);

        std::string HMDDevice = getTrackedDeviceString(
            HMD,
            vr::k_unTrackedDeviceIndex_Hmd,
            vr::Prop_TrackingSystemName_String,
            nullptr
        );
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "OpenVR Device Name : %s\n", HMDDevice.c_str()
        );

        std::string HMDNumber = getTrackedDeviceString(
            HMD,
            vr::k_unTrackedDeviceIndex_Hmd,
            vr::Prop_SerialNumber_String,
            nullptr
        );
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "OpenVR Device Number : %s\n", HMDNumber.c_str()
        );

        vr::IVRRenderModels* renderModels = reinterpret_cast<vr::IVRRenderModels*>(
            vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError)
        );
        if (!renderModels) {
            shutdown();
            std::string err = "Unable to get render model interface: ";
            err += vr::VR_GetVRInitErrorAsEnglishDescription(eError);
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "VR_Init Failed: %s\n", err.c_str()
            );
        }

        updateHMDMatrices(nearClip, farClip);
    }
}

void shutdown() {
    if (isOpenVRInitalized) {
        vr::VR_Shutdown();

        glDeleteTextures(1, &leftEyeFBODesc.texID);
        glDeleteFramebuffers(1, &leftEyeFBODesc.fboID);

        glDeleteTextures(1, &rightEyeFBODesc.texID);
        glDeleteFramebuffers(1, &rightEyeFBODesc.fboID);
    }
    else if (HMD) {
        vr::VR_Shutdown();
    }
    HMD = nullptr;
}

bool isHMDActive() {
    return (vr::VR_IsHmdPresent() && isOpenVRInitalized);
}

// Assuming side-by-side stereo, i.e. one FBO, one texture for both eyes
void copyWindowToHMD(SGCTWindow* win) {
    if (!isHMDActive()) {
        return;
    }

    glm::ivec2 dim = win->getFinalFBODimensions();
    const int windowWidth = dim.x;
    const int windowHeight = dim.y;
    const int renderWidth = dim.x / 2;
    const int renderHeight = dim.y;

    // HMD Left Eye
    glBindFramebuffer(GL_READ_FRAMEBUFFER, win->getFBO()->getBufferID());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeFBODesc.fboID);

    glBlitFramebuffer(
        0,
        0,
        renderWidth,
        renderHeight,
        0,
        0,
        renderWidth,
        renderHeight,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
    );

    vr::Texture_t leftEyeTexture = {
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(leftEyeFBODesc.texID)),
        vr::TextureType_OpenGL,
        vr::ColorSpace_Gamma
    };
    vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

    // HMD Right Eye
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeFBODesc.fboID);

    glBlitFramebuffer(
        renderWidth,
        0,
        windowWidth,
        renderHeight,
        0,
        0,
        renderWidth,
        renderHeight,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
    );

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    vr::Texture_t rightEyeTexture = {
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(rightEyeFBODesc.texID)),
        vr::TextureType_OpenGL,
        vr::ColorSpace_Gamma
    };
    vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
}

glm::mat4 getHMDCurrentViewProjectionMatrix(sgct_core::Frustum::FrustumMode nEye) {
    if (nEye == sgct_core::Frustum::StereoLeftEye) {
        return eyeLeftProjectionMat * eyeLeftToHeadMat * poseHMDMat;
    }
    else if (nEye == sgct_core::Frustum::StereoRightEye) {
        return eyeRightProjectionMat * eyeRightToHeadMat *  poseHMDMat;
    }
    else {
        // Mono
        return eyeLeftProjectionMat * poseHMDMat;
    }
}

std::string getTrackedDeviceString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, 
                                   vr::TrackedDeviceProperty prop,
                                   vr::TrackedPropertyError* peError)
{
    if (!isHMDActive()) {
        return "";
    }

    const uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(
        unDevice,
        prop,
        nullptr,
        0,
        peError
    );
    if (unRequiredBufferLen == 0) {
        return "";
    }

    std::vector<char> pchBuffer(unRequiredBufferLen);
    pHmd->GetStringTrackedDeviceProperty(
        unDevice,
        prop,
        pchBuffer.data(),
        unRequiredBufferLen,
        peError
    );
    return pchBuffer.data();
}

void updatePoses() {
    if (!HMD) {
        return;
    }

    char deviceClassChar[vr::k_unMaxTrackedDeviceCount];
    glm::mat4 devicePoseMat[vr::k_unMaxTrackedDeviceCount];

    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    vr::VRCompositor()->WaitGetPoses(
        trackedDevicePose,
        vr::k_unMaxTrackedDeviceCount,
        nullptr,
        0
    );

    for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice) {
        if (!trackedDevicePose[nDevice].bPoseIsValid) {
            continue;
        }
        devicePoseMat[nDevice] = convertSteamVRMatrixToMatrix4(
            trackedDevicePose[nDevice].mDeviceToAbsoluteTracking
        );

        // @TODO (abock, 2019-09-11) This deviceClassCharalue is not actually used
        // anywhere, but I didn't feel like I wanted to remove it, but I think it could be
        if (deviceClassChar[nDevice] == 0) {
            switch (HMD->GetTrackedDeviceClass(nDevice)) {
                case vr::TrackedDeviceClass_Controller:
                    deviceClassChar[nDevice] = 'C';
                    break;
                case vr::TrackedDeviceClass_HMD:
                    deviceClassChar[nDevice] = 'H';
                    break;
                case vr::TrackedDeviceClass_Invalid:
                    deviceClassChar[nDevice] = 'I';
                    break;
                case vr::TrackedDeviceClass_GenericTracker:
                    deviceClassChar[nDevice] = 'G';
                    break;
                case vr::TrackedDeviceClass_TrackingReference:
                    deviceClassChar[nDevice] = 'T';
                    break;
                default:
                    deviceClassChar[nDevice] = '?';
                    break;
            }
        }
    }

    if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid) {
        poseHMDMat = devicePoseMat[vr::k_unTrackedDeviceIndex_Hmd];
        glm::inverse(poseHMDMat);
    }
}

void updateHMDMatrices(float nearClip, float farClip) {
    eyeLeftProjectionMat = getHMDEyeProjectionMatrix(vr::Eye_Left, nearClip, farClip);
    eyeLeftToHeadMat = getHMDEyeToHeadTransform(vr::Eye_Left);
    eyeRightProjectionMat = getHMDEyeProjectionMatrix(vr::Eye_Right, nearClip, farClip);
    eyeRightToHeadMat = getHMDEyeToHeadTransform(vr::Eye_Right);
}

glm::mat4 getHMDEyeProjectionMatrix(vr::Hmd_Eye nEye, float nearClip, float farClip) {
    if (!HMD) {
        return glm::mat4();
    }

    vr::HmdMatrix44_t mat = HMD->GetProjectionMatrix(nEye, nearClip, farClip);

    return glm::mat4(
        mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
        mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
    );
}

glm::mat4 getHMDEyeToHeadTransform(vr::Hmd_Eye nEye) {
    if (!HMD) {
        return glm::mat4();
    }

    vr::HmdMatrix34_t mat = HMD->GetEyeToHeadTransform(nEye);
    glm::mat4 matrixObj(
        mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
        mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
        mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
        mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
    );

    return glm::inverse(matrixObj);
}

glm::mat4 getHMDPoseMatrix() {
    return poseHMDMat;
}

glm::quat getInverseRotation(glm::mat4 matPose) {
    glm::quat q;
    q.w = sqrt(fmaxf(0, 1 + matPose[0][0] + matPose[1][1] + matPose[2][2])) / 2;
    q.x = sqrt(fmaxf(0, 1 + matPose[0][0] - matPose[1][1] - matPose[2][2])) / 2;
    q.y = sqrt(fmaxf(0, 1 - matPose[0][0] + matPose[1][1] - matPose[2][2])) / 2;
    q.z = sqrt(fmaxf(0, 1 - matPose[0][0] - matPose[1][1] + matPose[2][2])) / 2;
    q.x = copysign(q.x, matPose[2][1] - matPose[1][2]);
    q.y = copysign(q.y, matPose[0][2] - matPose[2][0]);
    q.z = copysign(q.z, matPose[1][0] - matPose[0][1]);
    return q;
}

} // namespace sgct::openvr
