#include "Webserver.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h> // websocket lib

tthread::mutex gSendMutex;
Webserver * Webserver::mInstance = NULL;

char gBuffer[4096];//may corrupt the stack if too small

const char * get_mimetype(const char *file)
{
    size_t n = strlen(file);

    if (n < 5)
        return NULL;

    if (!strcmp(&file[n - 4], ".ico"))
        return "image/x-icon";

    if (!strcmp(&file[n - 4], ".png"))
        return "image/png";
    
    if (!strcmp(&file[n - 4], ".jpg"))
        return "image/png";

    if (!strcmp(&file[n - 5], ".html"))
        return "text/html";

    return NULL;
}

//callbacks
static int nullHttp(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    char buf[1024];
    memset(buf, 0, 1024);
    const char *mimetype;

    switch (reason)
    {
        case LWS_CALLBACK_HTTP:
        {
            if (len < 1)
            {
                lws_return_http_status(wsi,
                    HTTP_STATUS_BAD_REQUEST, NULL);
                return -1;
            }

            const char * data = reinterpret_cast<const char *>(in);

            /* this server has no concept of directories */
            if (strchr(data + 1, '/'))
            {
                lws_return_http_status(wsi,
                    HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }

            /* if a legal POST URL, let it continue and accept data */
            if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
                return 0;

            /* if not, send a file the easy way */
            //fprintf(stderr, "data: %s\n", data);
            if (strcmp(data, "/"))
            {
                strncat(buf, data+1, sizeof(buf) - strlen(buf) - 2);
            }
            else /* default file to serve */
            {
                strcat(buf, "index.html");
            }
            buf[sizeof(buf)-1] = '\0';

            /* refuse to serve files we don't understand */
            mimetype = get_mimetype(buf);
            if (!mimetype)
            {
                lwsl_err("Unknown mimetype for %s\n", buf);
                lws_return_http_status(wsi,
                    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
                return -1;
            }

            if (lws_serve_http_file(wsi, buf,
                mimetype, NULL, 0))
                return -1; /* through completion or error, close the socket */
        }
    break;
    }
    
    
    return 0;
}

static int echoCallback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *user, void *in, size_t len)
{
    // reason for callback
    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        {
            int n;
            unsigned int id = Webserver::instance()->generateSessionIndex();

            gSendMutex.lock();
#if (_MSC_VER >= 1400)
            n = sprintf_s(gBuffer, sizeof(gBuffer), "%u\n", id);
#else
            n = sprintf(gBuffer, "%u\n", id);
#endif
            lws_write(wsi, reinterpret_cast<unsigned char *>(gBuffer), n, LWS_WRITE_TEXT);
            gSendMutex.unlock();

            fprintf(stderr, "connection %u established\n", id);
        }
        break;

    case LWS_CALLBACK_RECEIVE:
        {
            // log what we recieved and what we're going to send as a response.
            if (Webserver::instance()->mWebMessageCallbackFn)
                Webserver::instance()->mWebMessageCallbackFn(reinterpret_cast<const char *>(in), len);
        }
        break;
    }

    return 0;
}

// protocol types for websockets
static struct lws_protocols protocols[] =
{
    {
        "http-only",
        nullHttp,
        0
    },
    {
        "sgct",
        echoCallback,
        0
    },
    {
        NULL, NULL, 0
    }
};

Webserver::Webserver()
{
    mMainThreadPtr = NULL;
    mWebMessageCallbackFn = NULL;
    mRunning = false;
    mSessionIndex = 0;
}

Webserver::~Webserver()
{
    mMutex.lock();
        mRunning = false;
        mWebMessageCallbackFn = NULL;
    mMutex.unlock();
    
    if (mMainThreadPtr && mMainThreadPtr->joinable())
    {
        fprintf(stderr, "Waiting for websocket thread to finish...\n");

        mMainThreadPtr->join();
        delete mMainThreadPtr;
        mMainThreadPtr = NULL;
    }
}

void Webserver::start(int port, int timeout_ms)
{
    mRunning = true;
    mPort = port;
    mTimeout = timeout_ms;
    mMainThreadPtr = new (std::nothrow) tthread::thread(Webserver::worker, NULL);
}

void Webserver::setCallback(Webserver::WebMessageCallbackFn cb)
{
    mMutex.lock();
    mWebMessageCallbackFn = cb;
    mMutex.unlock();
}

unsigned int Webserver::generateSessionIndex()
{
    //session index 0 is invalid all clients having an index > 0 is ok
    
    unsigned int tmpUi;
    mMutex.lock();
    mSessionIndex++;
    tmpUi = mSessionIndex;
    mMutex.unlock();
    return tmpUi;
}

void Webserver::worker(void *)
{
    // server url will be ws://localhost:9000
    const char *interface = NULL;
    struct lws_context *context = NULL;

    // we're not using ssl
    const char *cert_path = NULL;
    const char *key_path = NULL;

    //lws_set_log_level(7, lwsl_emit_syslog);
    //lws_set_log_level(1, lwsl_emit_syslog);

    // no special options
    int opts = 0;

    // create connection struct
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.port = Webserver::instance()->getPort();
    info.iface = interface;
    info.protocols = protocols;
    info.extensions = NULL;
    info.ssl_cert_filepath = cert_path;
    info.ssl_private_key_filepath = key_path;
    info.options = opts;
    info.gid = -1;
    info.uid = -1;

    // create libwebsocket context representing this server
    context = lws_create_context(&info);

    // make sure it starts
    if (context == NULL)
    {
        Webserver::instance()->mRunning = false;
        fprintf(stderr, "libwebsocket init failed\n");
        return;
    }
    
    printf("starting server...\n");
    // infinite loop, to end this server send SIGTERM. (CTRL+C)
    while (Webserver::instance()->mRunning.load())
    {
        lws_service(context, Webserver::instance()->getTimeout()); //5 ms -> 200 samples / s
        //lws_service(context, 50);
    }
    
    lws_context_destroy(context);
    return;
}