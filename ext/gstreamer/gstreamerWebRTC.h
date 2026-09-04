
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// INCLUDE GSTREAMER
#define GL_GLEXT_PROTOTYPES 1
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#define GLFW_EXPOSE_NATIVE_WGL 1
#include <GLFW/glfw3native.h>

#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/gl/gstgldisplay.h>
#include <gstreamer-1.0/gst/gl/gstglmemory.h>
#include <gstreamer-1.0/gst/gl/gstglcontext.h>
#include <libsoup-2.4/libsoup/soup.h>
#include <glib-2.0/glib-object.h>
#include <json/json.hpp> //SGCT
#include <gstreamer-1.0/gst/sdp/sdp.h>
#define GST_USE_UNSTABLE_API
#include <gstreamer-1.0/gst/webrtc/webrtc.h>
#include <gstreamer-1.0/gst/app/app.h>

// Used to determine correct path for GStreamer plugins as current path differs if run from Visual Studio or
// just from the .exe file.
#include <filesystem>
namespace fs = std::filesystem;


//TODO: Change enums to OpenSpace standard camelcase
enum AppState {
    APP_STATE_UNKNOWN = 0,
    APP_STATE_ERROR = 1,
    SERVER_CONNECTING = 1000,
    SERVER_CONNECTION_ERROR,
    SERVER_CONNECTED,
    SERVER_REGISTERING = 2000,
    SERVER_REGISTRATION_ERROR,
    SERVER_REGISTERED,
    SERVER_CLOSED,
    PEER_CONNECTING = 3000,
    PEER_CONNECTION_ERROR,
    PEER_CONNECTED,
    PEER_CALL_NEGOTIATING = 4000,
    PEER_CALL_STARTED,
    PEER_CALL_STOPPING,
    PEER_CALL_STOPPED,
    PEER_CALL_ERROR,
};

// Connection ID, this is static for the host
static int ourId = 1; 

namespace {
    double currentTime = 0.0;

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint framebufferName = 0;
    GLuint renderTexture = 0;
    GLuint quad_vertexbuffer = 0;

    GLint matrixLoc = -1;
} // namespace

static GstElement* webrtc1, * source1;

struct pipelineConfig
{
    GLFWwindow* window;

    GstGLDisplay* gl_display;

    GstGLContext* gl_context;
    GstGLContext* main_context;
    GstContext* display_context;
    GstContext* app_context;

    GstGLVideoAllocationParams* allocation_params;

    int width;
    int height;

    GLuint framebuffer;
    GLuint texture;

    GstPipeline* pipeline;
    GMainLoop* loop;
    guint sourceID;
};

pipelineConfig glPipeline = { 0 };

// "A class representing a WebSocket connection"
static SoupWebsocketConnection* ws_conn = NULL;
static enum AppState app_state = APP_STATE_UNKNOWN;
static const gchar* peer_id = "2";
static const gchar* server_url = "ws://localhost:8443";
static gboolean disable_ssl = FALSE;

using namespace sgct;


// HEADER FUNCTIONS ---------------------------------------------------------------------

static gboolean quitLoop(const gchar* msg, enum AppState state);

static void sendToSocket(const char* type, const char* content);

static void sendToSocket(const char* type, nlohmann::json content);

static void onServerClosed(SoupWebsocketConnection* conn G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED);

static void allocateGstGlMemory();

void initializeGstGl();

static gboolean checkGstPlugins();

// Prepare a GstBuffer, wrap an OpenGL texture into it.
static GstBuffer* wrapGlTexture();

// Render the scene and push the buffer
static void addBufferToSource();

static gboolean runGMainLoop(gpointer data);

// Callback function for when the source buffer can receive more data
static void onNeedData(GstAppSrc* appsrc, guint size, gpointer user_data);

// Callback function for when the source buffer can not receive any more data
static void onEnoughData(GstAppSrc* appsrc, gpointer user_data);

// Handles all messages from elements in the pipeline
static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data);

static void sendIceCandidateMessage(GstElement* webrtc G_GNUC_UNUSED, guint mlineindex,
    gchar* candidate, gpointer user_data G_GNUC_UNUSED);

static void sendSdpOffer(GstWebRTCSessionDescription* offer);

/* Offer created by our pipeline, to be sent to the peer */
static void onOfferCreated(GstPromise* promise, gpointer user_data);

static void onNegotiationNeeded(GstElement* element, gpointer user_data);

static gboolean startPipeline();

static gboolean setupCall(void);

static gboolean registerWithServer(void);

static void onServerMessage(SoupWebsocketConnection* conn, SoupWebsocketDataType type,
    GBytes* message, gpointer user_data);

static void onServerConnected(SoupSession* session, GAsyncResult* res, SoupMessage* msg, gpointer p);

static void connectToSignalingServer();

static void initGST();

void initOGL(GLFWwindow*);

void draw(const RenderData& data);

void postDraw();

void preSync();

std::vector<std::byte> encode();

void cleanup();

#pragma once


// FUNCTION INITIALIZATION --------------------------------------------------------------

static void initGST() {
    // Call all gstreamer functionality here
    glPipeline.texture = renderTexture;
    glPipeline.width = Engine::instance().windows()[1]->resolution().x;
    glPipeline.height = Engine::instance().windows()[1]->resolution().y;

    checkGstPlugins();
    initializeGstGl();

    // Start the WebRTC 
    connectToSignalingServer();

    // Create empty loop
    glPipeline.loop = g_main_loop_new(NULL, FALSE);

}

// TODO: Rename this function as it also fetches the necessary plugins and not just checks them
static gboolean checkGstPlugins()
{
    unsigned int i;
    gboolean ret;
    GstPlugin* plugin;
    GstRegistry* registry;
    
    registry = gst_registry_get();

    //Scans current directory (location of .exe) for available plugins.
    //Replaces the need for an ENV variable to plugin directory
    gst_registry_scan_path(registry, g_get_current_dir());


    //TODO: Perhaps hacky, can default to whatever build that is run?
    gchar* RelWithDebInfo = g_build_filename(OPENSPACE_ROOT,"bin","RelWithDebInfo", NULL);
    gchar* Debug = g_build_filename(OPENSPACE_ROOT,"bin","Debug", NULL);
    gchar* Release = g_build_filename(OPENSPACE_ROOT,"bin","Release", NULL);

    gst_registry_scan_path(registry, RelWithDebInfo);
    gst_registry_scan_path(registry, Debug);
    gst_registry_scan_path(registry, Release);


    return TRUE;
}

void initializeGstGl() {

    // Takes an existing OpenGL context, wraps into a GstGlContext.
    guintptr gl_handle = (guintptr)wglGetCurrentContext();
    glPipeline.gl_display = gst_gl_display_new();
    glPipeline.gl_context = gst_gl_context_new_wrapped(glPipeline.gl_display,
        gl_handle, GST_GL_PLATFORM_WGL, GST_GL_API_OPENGL);

    HGLRC oldContext = wglGetCurrentContext();
    HDC oldDevice = wglGetCurrentDC();

    // TODO: Right now the context is reset, can we copy it instead?
    wglMakeCurrent(NULL, NULL);

    // Create the GstGlContext main_context
    GError* err = NULL;
    gboolean isCreated = gst_gl_display_create_context(glPipeline.gl_display,
        glPipeline.gl_context, &glPipeline.main_context, &err);


    if (!isCreated) {
        g_print("Failed to create GstGLContex context: %s\n",
            err->message);
        g_clear_error(&err);
        exit(EXIT_FAILURE);
    }

    // Set up all contexts correctly 
    gst_gl_display_add_context(glPipeline.gl_display, glPipeline.main_context);
    glPipeline.display_context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
    gst_context_set_gl_display(glPipeline.display_context, glPipeline.gl_display);

    glPipeline.app_context = gst_context_new("gst.gl.app_context", TRUE);
    GstStructure* s = gst_context_writable_structure(glPipeline.app_context);
    gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, glPipeline.main_context, NULL);

    // Activate the main OpenGL context -- similar to wglMakeCurrent
    gst_gl_context_activate(glPipeline.main_context, TRUE);

    // See, we put it back
    wglMakeCurrent(oldDevice, oldContext);

    allocateGstGlMemory();

}

static void allocateGstGlMemory() {

    // Set the GL properties as frameInfo
    GstVideoInfo* frameInfo = gst_video_info_new();

    gboolean hasInfo = gst_video_info_set_format(frameInfo, GST_VIDEO_FORMAT_RGBA,
        glPipeline.width, glPipeline.height);

    if (!hasInfo) {
        g_print("Set video info ret %d\n", hasInfo);
        exit(EXIT_FAILURE);
    }

    // Allocate memory to support the mapping of GL buffers
    glPipeline.allocation_params =
        gst_gl_video_allocation_params_new_wrapped_texture(
            glPipeline.main_context, NULL, frameInfo, 0, NULL, GST_GL_TEXTURE_TARGET_2D,
            GST_GL_RGB8, glPipeline.texture, NULL, 0);
}

static void connectToSignalingServer() {
    SoupLogger* logger;   // debug logging support
    SoupMessage* message; // HTTP request and response
    SoupSession* session; // object that controls client-side HTTP
    const char* https_aliases[] = { "wss", NULL };

    // Create a session with options. In this case
    session = soup_session_new_with_options(
        SOUP_SESSION_SSL_STRICT, !disable_ssl,        // enable (?) SSL
        SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,    // use the local CA databaes
        //SOUP_SESSION_SSL_CA_FILE, "/etc/ssl/certs/ca-bundle.crt", // use this CA database (whatever is there)
        SOUP_SESSION_HTTPS_ALIASES, https_aliases,    // So we can use wss:// the same as https://, for example
        NULL);

    logger = soup_logger_new(SOUP_LOGGER_LOG_BODY, -1);  // Create unlimited size logger 
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger)); // add the logger to the session
    g_object_unref(logger);  // since the session gets a copy of the logger, deref this logger

    message = soup_message_new(SOUP_METHOD_GET, server_url); // set the message destination

    g_print("Connecting to server...\n");

    // Once connected, we will register 
    soup_session_websocket_connect_async(session, message, NULL, NULL, NULL,
        (GAsyncReadyCallback)onServerConnected, message);
    app_state = SERVER_CONNECTING;
}

static void onServerConnected(SoupSession* session, GAsyncResult* res,
    SoupMessage* msg, gpointer p)
{
    GError* error = NULL;

    // Returns a SoupWebsocketConnection that can be used to communicate with the server
    ws_conn = soup_session_websocket_connect_finish(session, res, &error);
    if (error) {
        quitLoop(error->message, SERVER_CONNECTION_ERROR); // Error handling
        g_error_free(error);
        return;
    }

    g_assert_nonnull(ws_conn);

    app_state = SERVER_CONNECTED; // Our own enums -- should not be uppercase in OpenSpace
    g_print("Connected to signalling server\n");

    // If the connection is closed - basically just quit_loop and change the app_state
    g_signal_connect(ws_conn, "closed", G_CALLBACK(onServerClosed), &p);
    // If the connection receives a message from the server
    g_signal_connect(ws_conn, "message", G_CALLBACK(onServerMessage), &p);

    // Register with the server so it knows about us and can accept commands
    registerWithServer();
}

static gboolean registerWithServer(void)
{
    gchar* hello;

    if (soup_websocket_connection_get_state(ws_conn) !=
        SOUP_WEBSOCKET_STATE_OPEN)
        return FALSE;

    g_print("Registering id %i with server\n", ourId);
    app_state = SERVER_REGISTERING;

    hello = g_strdup_printf("REGISTER %i", ourId);
    sendToSocket("REGISTER", "null");
    g_free(hello);

    return TRUE;
}

// One mega message handler for our asynchronous calling mechanism 
static void onServerMessage(SoupWebsocketConnection* conn, SoupWebsocketDataType type,
    GBytes* message, gpointer user_data)
{
    gsize size;
    gchar* text, * data;

    pipelineConfig* p = (pipelineConfig*)user_data;


    // Extract the contents of the message, now we only support text, not binary
    switch (type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
        g_printerr("Received unknown binary message, ignoring\n");
        g_bytes_unref(message);
        return;
    case SOUP_WEBSOCKET_DATA_TEXT:
        data = static_cast<gchar*>(g_bytes_unref_to_data(message, &size));
        /* Convert to NULL-terminated string */
        text = g_strndup(data, size);
        g_free(data);
        break;
    default:
        g_assert_not_reached();
    }


    // Server has accepted our registration, we are ready to send commands 
    if (g_strcmp0(text, "REGISTER") == 0) {

        if (app_state != SERVER_REGISTERING) {
            quitLoop("ERROR: Received REGISTER when not registering",
                APP_STATE_ERROR);
            goto out;
        }
        app_state = SERVER_REGISTERED;
        g_print("Registered with server\n");

        // Ask signalling server to connect us with a specific peer 
        if (!setupCall()) {
            quitLoop("ERROR: Failed to setup call", PEER_CALL_ERROR);
            goto out;
        }

        // Call has been setup by the server, now we can start negotiation
    }
    else if (g_strcmp0(text, "SESSION_OK") == 0) {
        if (app_state != PEER_CONNECTING) {
            quitLoop("ERROR: Received SESSION_OK when not calling",
                PEER_CONNECTION_ERROR);
            goto out;
        }

        app_state = PEER_CONNECTED;

        // Start negotiation (exchange SDP and ICE candidates)
        if (!startPipeline())
            quitLoop("ERROR: failed to start pipeline",
                PEER_CALL_ERROR);
        
        // Handle errors
    }
    else if (g_str_has_prefix(text, "ERROR")) {
        switch (app_state) {
        case SERVER_CONNECTING:
            app_state = SERVER_CONNECTION_ERROR;
            break;
        case SERVER_REGISTERING:
            app_state = SERVER_REGISTRATION_ERROR;
            break;
        case PEER_CONNECTING:
            app_state = PEER_CONNECTION_ERROR;
            break;
        case PEER_CONNECTED:
            // break;
        case PEER_CALL_NEGOTIATING:
            app_state = PEER_CALL_ERROR;
            //break;
        default:
            app_state = APP_STATE_ERROR;
            break;
        }
        quitLoop(text, APP_STATE_UNKNOWN);
        /* Look for JSON messages containing SDP and ICE candidates */
    }
    else {

        nlohmann::json msgJSON = nlohmann::json::parse(text);
        std::string msgType = msgJSON.value("type", "ERROR");

        // Check type of JSON message 
        if (msgType == "SDP") {
            // Necessary variables from gStreamer..
            int ret;
            GstSDPMessage* sdp;
            GstWebRTCSessionDescription* answer;

            // Check that we are at the right stage in our pipeline
            g_assert_cmphex(app_state, == , PEER_CALL_NEGOTIATING);

            // Get the content of the sdp
            nlohmann::json sdpMsg = msgJSON.at("content").at("sdp"); // sdp: {sdp:"", type:""}

            // Check if the SDP is of type offer or answer
            // TODO: Now there is only functionality for answer!
            std::string sdpType = sdpMsg.value("type", "ERROR"); // offer/answer
            if (sdpType == "ERROR") {
                quitLoop("ERROR: received SDP without 'type'",
                    PEER_CALL_ERROR);
            }

            // Check if we have content in the sdp description
            std::string sdpString = sdpMsg.value("sdp", "ERROR");
            if (sdpString == "ERROR") {
                quitLoop("ERROR: received SDP without 'sdp'",
                    PEER_CALL_ERROR);
            }
            gchar* sdpDesc = const_cast<char*>(sdpString.c_str()); // sdp contents

            if (sdpType == "offer") {
                g_print("There is no support for sdp offers for this client");
            }
            else if (sdpType == "answer") {
                g_print("Received answer from %s:\n%s\n ", peer_id, sdpDesc);

                ret = gst_sdp_message_new(&sdp);
                g_assert_cmphex(ret, == , GST_SDP_OK);

                ret = gst_sdp_message_parse_buffer((guint8*)sdpDesc, strlen(sdpDesc), sdp);
                g_assert_cmphex(ret, == , GST_SDP_OK);

                answer = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER,
                    sdp);
                g_assert_nonnull(answer);

                // Set remote description on our pipeline 
                {
                    GstPromise* promise = gst_promise_new();
                    g_signal_emit_by_name(webrtc1, "set-remote-description", answer,
                        promise);
                    gst_promise_interrupt(promise);
                    gst_promise_unref(promise);
                }

                app_state = PEER_CALL_STARTED;

            }
            else {
                g_print("SDP type wasn't OFFER or ANSWER");
            }
        }
        else if (msgType == "ICE") {

            const gchar* candidate;
            gint sdpmlineindex;

            nlohmann::json iceMsg = msgJSON.at("content").at("ice");
            std::cout << "ICE MESSAGE:\n" << iceMsg << "\n";

            std::string iceCandidate = iceMsg.value("candidate", "ERROR");
            if (iceCandidate == "ERROR") {
                quitLoop("ERROR: Invalid ICE candidate.",
                    PEER_CALL_ERROR);
            }

            candidate = const_cast<char*>(iceCandidate.c_str());

            sdpmlineindex = iceMsg.value("sdpMLineIndex", -1);
            if (sdpmlineindex < 0) {
                quitLoop("ERROR: Invalid sdpMLineIndex.",
                    PEER_CALL_ERROR);
            }

            g_signal_emit_by_name(webrtc1, "add-ice-candidate", sdpmlineindex, candidate);
        }
        else {
            g_printerr("Ignoring unknown JSON message:\n%s\n", text);
        }
    }

out:
    g_free(text);
}

static void onServerClosed(SoupWebsocketConnection* conn G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED)
{
    app_state = SERVER_CLOSED;
    quitLoop("Server connection closed", APP_STATE_UNKNOWN);
}

static gboolean setupCall(void)
{
    gchar* msg;
    gchar* peer_id_c;

    if (soup_websocket_connection_get_state(ws_conn) !=
        SOUP_WEBSOCKET_STATE_OPEN)
        return FALSE;

    if (!peer_id)
        return FALSE;

    g_print("Setting up signalling server call with %s\n", peer_id);
    app_state = PEER_CONNECTING;

    sendToSocket("SESSION", peer_id);
    return TRUE;
}

static void onNegotiationNeeded(GstElement* element, gpointer user_data)
{
    g_print("On Negotiation Created");

    GstPromise* promise;

    app_state = PEER_CALL_NEGOTIATING;

    promise = gst_promise_new_with_change_func(onOfferCreated, user_data, NULL);;
    g_signal_emit_by_name(webrtc1, "create-offer", NULL, promise);
}

static void onOfferCreated(GstPromise* promise, gpointer user_data)
{
    g_print("On Offer Created");
    GstWebRTCSessionDescription* offer = NULL;
    const GstStructure* reply;

    // TODO: This is a glib compare function.
    g_assert_cmphex(app_state, == , PEER_CALL_NEGOTIATING);
    g_assert_cmphex(gst_promise_wait(promise), == , GST_PROMISE_RESULT_REPLIED);

    reply = gst_promise_get_reply(promise);
    gst_structure_get(reply,
        "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer,
        NULL);
    gst_promise_unref(promise);

    promise = gst_promise_new();
    // Emits a signal to webrtc element
    g_signal_emit_by_name(webrtc1, "set-local-description", offer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);

    // Send offer to peer 
    sendSdpOffer(offer);
    gst_webrtc_session_description_free(offer);
}

static void sendSdpOffer(GstWebRTCSessionDescription* offer)
{
    gchar* text;

    if (app_state < PEER_CALL_NEGOTIATING) {
        quitLoop("Can't send offer, not in call", APP_STATE_ERROR);
        return;
    }

    // Convert sdp to a string
    text = gst_sdp_message_as_text(offer->sdp);
    g_print("Sending offer:\n%s\n", text);

    nlohmann::json sdpMsg = {
        { "content",
            {{"sdp",
                {
                    {"type", "offer"},
                    {"sdp", text}
                }
            }}
        }
    };

    // Send our offer to the server
    sendToSocket("SDP", sdpMsg);
    g_free(text);
}

static void sendIceCandidateMessage(GstElement* webrtc G_GNUC_UNUSED, guint mlineindex,
    gchar* candidate, gpointer user_data G_GNUC_UNUSED)
{

    if (app_state < PEER_CALL_NEGOTIATING) {
        quitLoop("Can't send ICE, not in call", APP_STATE_ERROR);
        return;
    }

    nlohmann::json iceMsg = {
        { "content",
            {{ "ice",
                {
                    {"candidate", candidate},
                    {"sdpMLineIndex", mlineindex}
                }
            }}
        }
    };

    std::string msgDump = iceMsg.dump();
    char* msgString = const_cast<char*>(msgDump.c_str());
    sendToSocket("ICE", iceMsg);
}

static void sendToSocket(const char* type, const char* content) {

    nlohmann::json msgJSON = {
        {"type", type},
        {"content", content},
        {"caller_id", ourId}
    };
    std::string msgDump = msgJSON.dump();
    char* msgString = const_cast<char*>(msgDump.c_str());
    soup_websocket_connection_send_text(ws_conn, msgString);
};

static void sendToSocket(const char* type, nlohmann::json content) {

    nlohmann::json msgJSON = {
        {"type", type},
        {"caller_id", ourId}
    };
    msgJSON.insert(content.begin(), content.end());
    std::string msgDump = msgJSON.dump();
    char* msgString = const_cast<char*>(msgDump.c_str());
    soup_websocket_connection_send_text(ws_conn, msgString);
};

static gboolean startPipeline()
{
    GstStateChangeReturn ret;
    GError* error = NULL;

    gchar* pWidth = g_strdup_printf("%i", glPipeline.width);
    gchar* pHeight = g_strdup_printf("%i", glPipeline.height);

    // Pipeline using the x264 encoder. Use for non-Nvidia graphics cards.
    //gchar* pipelineDescription = g_strconcat(
    //    " appsrc stream-type=0 do-timestamp=1 "
    //    " format=GST_FORMAT_TIME is-live=1 name=source "
    //    " caps=\"video/x-raw(memory:GLMemory), width=", pWidth, ",height=", pHeight,
    //    ", format=(string)RGBA, texture-target=(string)2D\" ! "
    //    " glvideoflip method=vertical-flip ! glcolorconvert !  gldownload ! "
    //    " x264enc tune=zerolatency speed-preset=veryfast ! "
    //    " rtph264pay ! "
    //    " application/x-rtp, profile=high, media=video, encoding-name=H264, payload=96 ! "
    //    " webrtcbin name=sendrecv", NULL);

    gchar* pipelineDescription = g_strconcat(
        " appsrc stream-type=0 do-timestamp=1 "
        " format=GST_FORMAT_TIME is-live=1 name=source "
        " caps=\"video/x-raw(memory:GLMemory), width=", pWidth, ",height=", pHeight,
        ", framerate=50/1, format=(string)RGBA, texture-target=(string)2D\" ! "
        " glvideoflip method=vertical-flip ! glcolorconvert !  gldownload ! "
        " nvh264enc gop-size=-1 qp-const=5 preset=low-latency-hq rc-mode=cbr bitrate=6000 ! "
        " rtph264pay ! "
        " application/x-rtp, framerate=50/1, profile=main, media=video, encoding-name=H264, payload=96 ! "
        " webrtcbin name=sendrecv", NULL);

    g_print("Pipeline desc: %s", pipelineDescription);

    glPipeline.pipeline = GST_PIPELINE(gst_parse_launch(pipelineDescription, &error));

    if (error) {
        g_printerr("Failed to parse launch: %s\n", error->message);
        g_error_free(error);
        //TODO: Look into why 'goto err' gives error when uncommented
        //goto err;
    }

    // Check that our source has been added properly
    source1 = gst_bin_get_by_name(GST_BIN(glPipeline.pipeline), "source");
    g_assert(source1);
    g_assert(GST_IS_APP_SRC(source1));

    // OPENGL-related functionality -------------------------------------------------------------------
    // Connects the necessary callback functions to the pipeline
    g_signal_connect(source1, "need-data", G_CALLBACK(onNeedData), &glPipeline);
    g_signal_connect(source1, "enough-data", G_CALLBACK(onEnoughData), &glPipeline);

    // Connect the pipeline to a GstBus which will handle all messages from the pipeline
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(glPipeline.pipeline));
    gst_bus_add_watch(bus, onBusMessage, &glPipeline);
    gst_object_unref(bus);

    // Set our contexts
    gst_element_set_context(GST_ELEMENT(glPipeline.pipeline), glPipeline.app_context);
    gst_element_set_context(GST_ELEMENT(glPipeline.pipeline), glPipeline.display_context);

    // WEBRTC-related functionality -------------------------------------------------------------------

    // Check valid webrtc-bin
    webrtc1 = gst_bin_get_by_name(GST_BIN(glPipeline.pipeline), "sendrecv");
    g_object_set(webrtc1, "latency", 5, NULL);
    g_assert_nonnull(webrtc1);

    g_signal_connect(webrtc1, "on-negotiation-needed",
        G_CALLBACK(onNegotiationNeeded), NULL);

    g_signal_connect(webrtc1, "on-ice-candidate",
        G_CALLBACK(sendIceCandidateMessage), NULL);

    gst_object_unref(webrtc1);


    // Finalize the pipeline -------------------------------------------------------------------

    gst_element_set_state(GST_ELEMENT(glPipeline.pipeline), GST_STATE_PAUSED);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(glPipeline.pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

    g_print("Starting pipeline\n");
    ret = gst_element_set_state(GST_ELEMENT(glPipeline.pipeline), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        //TODO: Look into why 'goto err' gives error when uncommented
        //goto err;

    g_print("Started pipeline\n");
    return TRUE;

err:
    if (glPipeline.pipeline)
        g_clear_object(&glPipeline.pipeline);
    if (webrtc1)
        webrtc1 = NULL;
    return FALSE;
}

static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer data) {

    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_EOS:
        g_print("End of stream\n");
        g_main_loop_quit(glPipeline.loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(glPipeline.loop);
        break;
    }

    case GST_MESSAGE_NEED_CONTEXT:
    {
        const gchar* context_type;
        gst_message_parse_context_type(msg, &context_type);
        gst_element_get_name(GST_ELEMENT(msg->src));

        if (g_strcmp0(context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
            gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(msg)), glPipeline.display_context);
        }
        else if (g_strcmp0(context_type, "gst.gl.app_context") == 0) {
            GST_ELEMENT(msg->src);
            gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(msg)), glPipeline.app_context);
        }
        break;
    }
    case GST_MESSAGE_HAVE_CONTEXT:
    {
        GstContext* context;
        const gchar* context_type;
        gchar* context_str;

        gst_message_parse_have_context(msg, &context);

        context_type = gst_context_get_context_type(context);
        context_str =
            gst_structure_to_string(gst_context_get_structure(context));
        g_print("Got context from element '%s': %s=%s\n",
            GST_ELEMENT_NAME(GST_MESSAGE_SRC(msg)), context_type, context_str);
        g_free(context_str);
        gst_context_unref(context);
    }
    break;

    default:
        break;
    }

    return TRUE;
}

// Callback function for when the source buffer can receive more data
static void onNeedData(GstAppSrc* appsrc, guint size, gpointer user_data)
{
    if (glPipeline.sourceID == 0) {
        glPipeline.sourceID = g_idle_add((GSourceFunc)runGMainLoop, &glPipeline);
    }
}

// Callback function for when the source buffer can not receive any more data
static void onEnoughData(GstAppSrc* appsrc, gpointer user_data)
{
    if (glPipeline.sourceID != 0) {
        g_source_remove(glPipeline.sourceID);
        glPipeline.sourceID = 0;
    }
}

static gboolean runGMainLoop(gpointer data)
{

    addBufferToSource();

    return TRUE;
}

// Render the scene and push the buffer
static void addBufferToSource() {

    // Wrap the rendered texture into a GstBuffer.
    GstBuffer* buffer = wrapGlTexture();

    // Push the buffer.
    // TODO: Try to fix the enum/enum class warning
    GstFlowReturn bufferStatus = gst_app_src_push_buffer(GST_APP_SRC(source1), buffer);
    if (bufferStatus != GST_FLOW_OK) {
        g_print("Failed to add the GstBuffer to appsrc, return value is %d\n",
            bufferStatus);
    }
}

// Prepare a GstBuffer, wrap an OpenGL texture into it.
static GstBuffer* wrapGlTexture()
{
    glActiveTexture(GL_TEXTURE0);
    GstGLMemoryAllocator* allocator;
    gpointer wrappedTextures[1];
    GstGLFormat textureFormats[1];
    GstBuffer* buffer;

    allocator = gst_gl_memory_allocator_get_default(glPipeline.main_context);

    buffer = gst_buffer_new();

    if (!buffer) {
        g_error("Failed to create new buffer\n");
        return NULL;
    }

    textureFormats[0] = GST_GL_RGBA8;

    // Connect GStreamer to the second window of SGCT. Configuration of this windows is done in the SGCT-config file for GStreamer
    wrappedTextures[0] = (gpointer)Engine::instance().windows()[1]->frameBufferTexture(Window::TextureIndex::LeftEye);

    // Wrap the texture into GLMemory
    gboolean isSetUp = gst_gl_memory_setup_buffer(allocator, buffer,
        glPipeline.allocation_params, textureFormats, wrappedTextures, 1);

    if (!isSetUp) {
        g_error("Failed to setup gl memory\n");
        return NULL;
    }

    gst_object_unref(allocator);

    return buffer;
}

static gboolean quitLoop(const gchar* msg, enum AppState state)
{
    if (msg)
        g_printerr("%s\n", msg);
    if (state > 0)
        app_state = state;

    if (ws_conn) {
        if (soup_websocket_connection_get_state(ws_conn) ==
            SOUP_WEBSOCKET_STATE_OPEN)
            soup_websocket_connection_close(ws_conn, 1000, "");
        else
            g_object_unref(ws_conn);
    }

    if (glPipeline.loop) {
        g_main_loop_quit(glPipeline.loop);
        glPipeline.loop = NULL;
    }

    return G_SOURCE_REMOVE;
}
