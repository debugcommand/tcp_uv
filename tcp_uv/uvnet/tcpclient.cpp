#include "tcpclient.h"

#define MAXLISTSIZE 20

namespace UVNet
{
TcpClientCtx* AllocTcpClientCtx(void* parentserver)
{
    TcpClientCtx* ctx = (TcpClientCtx*)malloc(sizeof(*ctx));
    ctx->packet_ = new PacketSync;
    ctx->read_buf_.base = (char*)malloc(MAX_BUFFER_SIZE);
    ctx->read_buf_.len = MAX_BUFFER_SIZE;
    ctx->write_req.data = ctx;//store self
    ctx->parent_server = parentserver;//store TCPClient
    return ctx;
}

void FreeTcpClientCtx(TcpClientCtx* ctx)
{
    delete ctx->packet_;
    free(ctx->read_buf_.base);
    free(ctx);
}


cli_write_param* AllocCliWriteParam(void)
{
    cli_write_param* param = (cli_write_param*)malloc(sizeof(*param));
    param->buf_.base = (char*)malloc(MAX_BUFFER_SIZE);
    param->buf_.len = MAX_BUFFER_SIZE;
    param->buf_truelen_ = MAX_BUFFER_SIZE;
    return param;
}

void FreeCliWriteParam(cli_write_param* param)
{
    free(param->buf_.base);
    free(param);
}

/*****************************************TCP Client*************************************************************/
TCPClient::TCPClient()
    : recvcb_(nullptr), recvcb_userdata_(nullptr), closedcb_(nullptr), closedcb_userdata_(nullptr)
    , connectstatus_(CONNECT_DIS), write_circularbuf_(MAX_BUFFER_SIZE)
    , isclosed_(true), isuseraskforclosed_(false)
    , reconnectcb_(nullptr), reconnect_userdata_(nullptr)
    , isIPv6_(false), isreconnecting_(false)
    , conncb_userdata_(nullptr), conncb_(nullptr)
{
    client_handle_ = AllocTcpClientCtx(this);
    int iret = uv_loop_init(&loop_);
    if (iret) {
        errmsg_ = GetUVError(iret);
        fprintf(stdout, "init loop error: %s\n", errmsg_.c_str());
#if 0
        LOGE(errmsg_);        
#endif // 0
     
    }
    iret = uv_mutex_init(&mutex_writebuf_);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
    }
    connect_req_.data = this;
    ishangup = false;
}


TCPClient::~TCPClient()
{
    Close();
    uv_thread_join(&connect_threadhandle_);
    FreeTcpClientCtx(client_handle_);
    uv_loop_close(&loop_);
    uv_mutex_destroy(&mutex_writebuf_);
    for (auto it = writeparam_list_.begin(); it != writeparam_list_.end(); ++it) {
        FreeCliWriteParam(*it);
    }
    writeparam_list_.clear();
#if 0
    LOGI("client(" << this << ")exit");
#endif // 0    
}

bool TCPClient::init()
{
    if (!isclosed_) {
        return true;
    }
    int iret = uv_async_init(&loop_, &async_handle_, AsyncCB);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    async_handle_.data = this;

    iret = uv_tcp_init(&loop_, &client_handle_->tcphandle);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    client_handle_->tcphandle.data = client_handle_;
    client_handle_->parent_server = this;

    client_handle_->packet_->SetPacketCB(GetPacket, CBClose, client_handle_);
    iret = uv_timer_init(&loop_, &reconnect_timer_);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    reconnect_timer_.data = this;
#if 0
    LOGI("client(" << this << ")Init");
#endif // 0
    isclosed_ = false;
    return true;
}

void TCPClient::closeinl()
{
    if (isclosed_) {
        return;
    }
    StopReconnect();
    uv_walk(&loop_, CloseWalkCB, this);
#if 0
    LOGI("client(" << this << ")close");
#endif // 0
}

bool TCPClient::run(int status)
{
    int iret = uv_run(&loop_, (uv_run_mode)status);
    isclosed_ = true;
#if 0
    LOGI("client had closed.");
#endif // 0
    if (closedcb_) {//trigger close cb to user
        closedcb_(-1, closedcb_userdata_); //client id is -1.
    }
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    return true;
}

bool TCPClient::SetNoDelay(bool enable)
{
    //TCP_NODELAY详解:http://blog.csdn.net/u011133100/article/details/21485983
    int iret = uv_tcp_nodelay(&client_handle_->tcphandle, enable ? 1 : 0);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    return true;
}

bool TCPClient::SetKeepAlive(int enable, unsigned int delay)
{
    int iret = uv_tcp_keepalive(&client_handle_->tcphandle, enable , delay);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0        
        return false;
    }
    return true;
}

bool TCPClient::Connect(const char* ip, int port)
{
    closeinl();
    if (!init()) {
        return false;
    }
    connectip_ = ip;
    connectport_ = port;
    isIPv6_ = false;
    struct sockaddr_in bind_addr;
    int iret = uv_ip4_addr(connectip_.c_str(), connectport_, &bind_addr);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0        
        return false;
    }
    iret = uv_tcp_connect(&connect_req_, &client_handle_->tcphandle, (const sockaddr*)&bind_addr, AfterConnect);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0        
        return false;
    }
#if 0
    LOGI("client(" << this << ")start connect to server(" << ip << ":" << port << ")");
#endif // 0
    iret = uv_thread_create(&connect_threadhandle_, ConnectThread, this);//thread to wait for succeed connect.
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0        
        return false;
    }
    int wait_count = 0;
    while (connectstatus_ == CONNECT_DIS) {
        ThreadSleep(100);
        if (++wait_count > 100) {
            connectstatus_ = CONNECT_TIMEOUT;
            break;
        }
    }
    if (CONNECT_FINISH != connectstatus_) {
        errmsg_ = "connect time out";
        return false;
    } else {
        return true;
    }
}

bool TCPClient::Connect6(const char* ip, int port)
{
    closeinl();
    if (!init()) {
        return false;
    }
    connectip_ = ip;
    connectport_ = port;
    isIPv6_ = true;
    struct sockaddr_in6 bind_addr;
    int iret = uv_ip6_addr(connectip_.c_str(), connectport_, &bind_addr);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
    iret = uv_tcp_connect(&connect_req_, &client_handle_->tcphandle, (const sockaddr*)&bind_addr, AfterConnect);
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0
        return false;
    }
#if 0
    LOGI("client(" << this << ")start connect to server(" << ip << ":" << port << ")");
#endif // 0    
    iret = uv_thread_create(&connect_threadhandle_, ConnectThread, this);//thread to wait for succeed connect.
    if (iret) {
        errmsg_ = GetUVError(iret);
#if 0
        LOGE(errmsg_);
#endif // 0           
        return false;
    }
    int wait_count = 0;
    while (connectstatus_ == CONNECT_DIS) {
        ThreadSleep(100);
        if (++wait_count > 100) {
            connectstatus_ = CONNECT_TIMEOUT;
            break;
        }
    }
    if (CONNECT_FINISH != connectstatus_) {
        errmsg_ = "connect time out";
        return false;
    } else {
        return true;
    }
}

void TCPClient::ConnectThread(void* arg)
{
    TCPClient* pclient = (TCPClient*)arg;
    pclient->run();
}

void TCPClient::AfterConnect(uv_connect_t* handle, int status)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->handle->data;
    TCPClient* parent = (TCPClient*)theclass->parent_server;
    if (status) {
        parent->connectstatus_ = CONNECT_ERROR;
        parent->errmsg_ = GetUVError(status);
        fprintf(stdout, "connect error:%s\n", parent->errmsg_.c_str());
#if 0
        LOGE("client(" << parent << ") connect error:" << parent->errmsg_);        
#endif // 0        
        if (parent->isreconnecting_) {//reconnect failure, restart timer to trigger reconnect.
            uv_timer_stop(&parent->reconnect_timer_);
            parent->repeat_time_ *= 2;
            uv_timer_start(&parent->reconnect_timer_, TCPClient::ReconnectTimer, parent->repeat_time_, parent->repeat_time_);
        }
        return;
    }

    int iret = uv_read_start(handle->handle, AllocBufferForRecv, AfterRecv);
    if (iret) {
        parent->errmsg_ = GetUVError(status);
        fprintf(stdout, "uv_read_start error:%s\n", parent->errmsg_.c_str());
#if 0
        LOGE("client(" << parent << ") uv_read_start error:" << parent->errmsg_);        
#endif // 0          
        parent->connectstatus_ = CONNECT_ERROR;
    } else {
        parent->connectstatus_ = CONNECT_FINISH;
        if (parent->conncb_) {
            parent->conncb_(parent->conncb_userdata_);
        }
    }
    if (parent->isreconnecting_) {
        fprintf(stdout, "reconnect succeed\n");     
        parent->StopReconnect();//reconnect succeed.
        if (parent->reconnectcb_) {
            parent->reconnectcb_(NET_EVENT_TYPE_RECONNECT, parent->reconnect_userdata_);
        }
    }
}

int TCPClient::Send(const char* data, std::size_t len)
{
    if (!data || len <= 0) {
        errmsg_ = "send data is null or len less than zero.";
#if 0
        LOGE(errmsg_);
#endif // 0 
        return 0;
    }
    uv_async_send(&async_handle_);
    size_t iret = 0;
    while (!isuseraskforclosed_) {
        uv_mutex_lock(&mutex_writebuf_);
        iret += write_circularbuf_.write(data + iret, len - iret);
        uv_mutex_unlock(&mutex_writebuf_);
        if (iret < len) {
            ThreadSleep(100);
            continue;
        } else {
            break;
        }
    }
    uv_async_send(&async_handle_);
    return iret;
}

void TCPClient::SetRecvCB(ClientRecvCB pfun, void* userdata)
{
    recvcb_ = pfun;
    recvcb_userdata_ = userdata;
}

void TCPClient::SetClosedCB(TcpCloseCB pfun, void* userdata)
{
    closedcb_ = pfun;
    closedcb_userdata_ = userdata;
}

void TCPClient::SetReconnectCB(ReconnectCB pfun, void* userdata)
{
    reconnectcb_ = pfun;
    reconnect_userdata_ = userdata;
}

void TCPClient::SetConnectedCB(ConnectedCB pfun, void* userdata) {
    conncb_ = pfun;
    conncb_userdata_ = userdata;
}

void TCPClient::AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
    assert(theclass);
    *buf = theclass->read_buf_;
}

void TCPClient::AfterRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
    assert(theclass);
    TCPClient* parent = (TCPClient*)theclass->parent_server;
    if (nread < 0) {
        if (parent->reconnectcb_) {
            parent->reconnectcb_(NET_EVENT_TYPE_DISCONNECT, parent->reconnect_userdata_);
        }
        if (!parent->StartReconnect()) {
            fprintf(stdout, "Start Reconnect Failure.\n");
            return;
        }
        if (nread == UV_EOF) {
            fprintf(stdout, "Server close(EOF), Client %p\n", handle);
#if 0            
            LOGW("Server close(EOF)");
#endif // 0             
        } else if (nread == UV_ECONNRESET) {
            fprintf(stdout, "Server close(conn reset),Client %p\n", handle);
#if 0            
            LOGW("Server close(conn reset)");
#endif // 0             
        } else {
            fprintf(stdout, "Server close,Client %p:%s\n", handle, GetUVError(nread).c_str());
#if 0            
            LOGW("Server close" << GetUVError(nread));
#endif // 0             
        }
        uv_close((uv_handle_t*)handle, AfterClientClose);//close before reconnect
        return;
    }
    parent->send_inl(NULL);
    if (nread > 0) {
        theclass->packet_->recvdata((const unsigned char*)buf->base, nread);
    }
}

void TCPClient::AfterSend(uv_write_t* req, int status)
{
    TCPClient* theclass = (TCPClient*)req->data;
    if (status < 0) {
        if (theclass->writeparam_list_.size() > MAXLISTSIZE) {
            FreeCliWriteParam((cli_write_param*)req);
        } else {
            theclass->writeparam_list_.push_back((cli_write_param*)req);
        }
#if 0
        LOGE("send error:" << GetUVError(status));        
#endif // 0
        fprintf(stderr, "send error %s\n", GetUVError(status));
        return;
    }
    theclass->send_inl(req);
}

/* Fully close a loop */
void TCPClient::CloseWalkCB(uv_handle_t* handle, void* arg)
{
    TCPClient* theclass = (TCPClient*)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, AfterClientClose);
    }
}

void TCPClient::AfterClientClose(uv_handle_t* handle)
{
    TCPClient* theclass = (TCPClient*)handle->data;
    fprintf(stdout, "Close CB handle %p\n", handle);    
    if (handle == (uv_handle_t*)&theclass->client_handle_->tcphandle && theclass->isreconnecting_) {
        //closed, start reconnect timer
        int iret = 0;
        iret = uv_timer_start(&theclass->reconnect_timer_, TCPClient::ReconnectTimer, theclass->repeat_time_, theclass->repeat_time_);
        if (iret) {
            uv_close((uv_handle_t*)&theclass->reconnect_timer_, TCPClient::AfterClientClose);
#if 0
            LOGE(GetUVError(iret));
#endif // 0              
            return;
        }
    }
}

void TCPClient::StartLog(const char* logpath /*= nullptr*/)
{
}

void TCPClient::StopLog()
{    
}

void TCPClient::GetPacket(const NetPacket& packethead, const unsigned char* packetdata, void* userdata)
{
    assert(userdata);
    TcpClientCtx* theclass = (TcpClientCtx*)userdata;
    TCPClient* parent = (TCPClient*)theclass->parent_server;
    if (parent->recvcb_) {//cb the data to user
        parent->recvcb_(packethead, packetdata, parent->recvcb_userdata_);
    }
}

void TCPClient::CBClose(void* userdata)
{
    TcpClientCtx* theclass = (TcpClientCtx*)userdata;
    TCPClient* parent = (TCPClient*)theclass->parent_server;
    parent->Close();
}

void TCPClient::AsyncCB(uv_async_t* handle)
{
    TCPClient* theclass = (TCPClient*)handle->data;
    if (theclass->isuseraskforclosed_) {
        theclass->closeinl();
        return;
    }
    //check data to send
    theclass->send_inl(NULL);
}

void TCPClient::send_inl(uv_write_t* req /*= NULL*/)
{
    cli_write_param* writep = (cli_write_param*)req;
    if (writep) {
        if (writeparam_list_.size() > MAXLISTSIZE) {
            FreeCliWriteParam(writep);
        } else {
            writeparam_list_.push_back(writep);
        }
    }
    while (true) {
        uv_mutex_lock(&mutex_writebuf_);
        if (write_circularbuf_.empty()) {
            uv_mutex_unlock(&mutex_writebuf_);
            break;
        }
        if (writeparam_list_.empty()) {
            writep = AllocCliWriteParam();
            writep->write_req_.data = this;
        } else {
            writep = writeparam_list_.front();
            writeparam_list_.pop_front();
        }
        writep->buf_.len = write_circularbuf_.read(writep->buf_.base, writep->buf_truelen_); 
        uv_mutex_unlock(&mutex_writebuf_);
        int iret = uv_write((uv_write_t*)&writep->write_req_, (uv_stream_t*)&client_handle_->tcphandle, &writep->buf_, 1, AfterSend);
        if (iret) {
            writeparam_list_.push_back(writep);//failure not call AfterSend. so recycle req
            fprintf(stdout, "send error. %s-%s\n", uv_err_name(iret), uv_strerror(iret));
#if 0
            LOGE("client(" << this << ") send error:" << GetUVError(iret));            
#endif
        }
    }
}

void TCPClient::Close(int flag)
{
    if (isclosed_) {
        return;
    }
    isuseraskforclosed_ = true;
    uv_async_send(&async_handle_);
}

bool TCPClient::StartReconnect(void)
{
    isreconnecting_ = true;
    client_handle_->tcphandle.data = this;
    repeat_time_ = 1e3;//1 sec
    return true;
}

void TCPClient::StopReconnect(void)
{
    isreconnecting_ = false;
    client_handle_->tcphandle.data = client_handle_;
    repeat_time_ = 1e3;//1 sec
    uv_timer_stop(&reconnect_timer_);
}

void TCPClient::ReconnectTimer(uv_timer_t* handle)
{
    TCPClient* theclass = (TCPClient*)handle->data;
    if (!theclass->isreconnecting_) {
        return;
    }
#if 0
    LOGI("start reconnect...\n");
#endif // 0     
    do {
        int iret = uv_tcp_init(&theclass->loop_, &theclass->client_handle_->tcphandle);
        if (iret) {
#if 0
            LOGE(GetUVError(iret));
#endif // 0                
            break;
        }
        theclass->client_handle_->tcphandle.data = theclass->client_handle_;
        theclass->client_handle_->parent_server = theclass;
        struct sockaddr* pAddr;
        if (theclass->isIPv6_) {
            struct sockaddr_in6 bind_addr;
            int iret = uv_ip6_addr(theclass->connectip_.c_str(), theclass->connectport_, &bind_addr);
            if (iret) {
#if 0
                LOGE(GetUVError(iret));
#endif // 0      
                uv_close((uv_handle_t*)&theclass->client_handle_->tcphandle, NULL);
                break;
            }
            pAddr = (struct sockaddr*)&bind_addr;
        } else {
            struct sockaddr_in bind_addr;
            int iret = uv_ip4_addr(theclass->connectip_.c_str(), theclass->connectport_, &bind_addr);
            if (iret) {
#if 0
                LOGE(GetUVError(iret));
#endif // 0      
                uv_close((uv_handle_t*)&theclass->client_handle_->tcphandle, NULL);
                break;
            }
            pAddr = (struct sockaddr*)&bind_addr;
        }
        iret = uv_tcp_connect(&theclass->connect_req_, &theclass->client_handle_->tcphandle, (const sockaddr*)pAddr, AfterConnect);
        if (iret) {
#if 0
            LOGE(GetUVError(iret));
#endif // 0
            uv_close((uv_handle_t*)&theclass->client_handle_->tcphandle, NULL);
            break;
        }
        return;
    } while (0);
    //reconnect failure, restart timer to trigger reconnect.
    uv_timer_stop(handle);
    theclass->repeat_time_ *= 2;
    uv_timer_start(handle, TCPClient::ReconnectTimer, theclass->repeat_time_, theclass->repeat_time_);
}
}