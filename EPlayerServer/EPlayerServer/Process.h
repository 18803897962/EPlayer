#pragma once
#include"Function.h"
#include<memory.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include <netinet/in.h>
class CProcess {//进程控制函数
public:
    CProcess() {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    ~CProcess() {
        if (m_func != NULL) {
            delete m_func;
            m_func = NULL;
        }
    }
    template<typename _FUNCTION_, typename..._ARGS_>  //_FUNCTION_指代函数类型  _ARGS_指代可变参数类型
    int SetEctryFunc(_FUNCTION_ func, _ARGS_... args) {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);//可变参数用的时候...加在后面  定义时...加在前面
        return 0;
    }
    int CreateSubPro() {
        if (m_func == NULL) return -1;
        int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);//AF_LOCAL本地套接字  pipes管道依靠此函数来绑定
        if (ret == -1) return -2;
        pid_t pid = fork();
        if (pid == -1) return -3;
        if (pid == 0) {
            close(pipes[1]); //pipes[1]用来写，由父进程写入，子进程取出    
            //如果需要双向通信，则需要设置两对管道  不能一个管道同时双方读写，避免二者都读孔管道  或者都写管道将管道堵塞
            pipes[1] = 0;
            //return (*m_func)();  //*解引用 成为CFunctionBase类对象  ()函数调用，括号运算符重载
            ret = (*m_func)();
            exit(0);
        }
        close(pipes[0]);  //主进程只写入 不读出
        pipes[0] = 0;
        m_pid = pid;
        return 0;
    }
    int SendFD(int fd) {//主进程发送文件描述符给子进程
        struct msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov[2];
        char buffer[2][10] = { "tisheng" ,"zuwo" };
        iov[0].iov_base = buffer[0];
        iov[0].iov_len = sizeof(buffer[0]);//base的长度
        iov[1].iov_base = buffer[1];
        iov[1].iov_len = sizeof(buffer[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        //以下才是真正需要传递的数据
        //cmsghdr* cmsg=new cmsghdr;
        //bzero(cmsg,sizeof(cmsghdr));
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int))); //CMSG_LEN(len) 表示sizeof(cmsghdr)+len
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *(int*)CMSG_DATA(cmsg) = fd;   //CMSG_DATA设置cmsg的内容 返回void*类型 需要强转并赋值 !!!!!!
        //printf("%d  fd has been send fd=%d\n", __LINE__, fd);
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t size = sendmsg(pipes[1], &msg, 0);  //pipes[1]写入
        free(cmsg);
        if (size == -1) {//发送失败
            //printf(" %d  %d  %s   this fd=%d\n", __LINE__, errno, strerror(errno),fd);
            return -2;
        }
        //delete cmsg;
        return 0;
    }
    int RecvFD(int& fd) {//传递引用 便于修改
        msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov[2];
        char buffer[][10] = { "","" };
        iov[0].iov_base = buffer[0];
        iov[0].iov_len = sizeof(buffer[0]);
        iov[1].iov_base = buffer[1];
        iov[1].iov_len = sizeof(buffer[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));  //真正接收的数据
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//套接字对
        cmsg->cmsg_type = SCM_RIGHTS;//传过来的数据要拿到权限

        msg.msg_control = cmsg;//控制信息
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t size = recvmsg(pipes[0], &msg, 0);//pipes[0]用来接收
        if (size == -1) {//发送失败
            printf(" %d  %d  %s\n", __LINE__, errno, strerror(errno));
            return -2;
        }
        //delete cmsg;
        free(cmsg);
        fd = *(int*)CMSG_DATA(cmsg);  //得到文件描述符
        return 0;
    }
    int SendSocket(int fd,const struct sockaddr_in* addr) {//主进程发送文件描述符给子进程
        struct msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov;
        char buffer[20] = "";
        bzero(buffer, sizeof(buffer));
        memcpy(buffer, addr, sizeof(sockaddr_in));
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        //以下才是真正需要传递的数据
        //cmsghdr* cmsg=new cmsghdr;
        //bzero(cmsg,sizeof(cmsghdr));
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int))); //CMSG_LEN(len) 表示sizeof(cmsghdr)+len
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *(int*)CMSG_DATA(cmsg) = fd;   //CMSG_DATA设置cmsg的内容 返回void*类型 需要强转并赋值 !!!!!!
        //printf("%d  fd has been send fd=%d\n", __LINE__, fd);
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;

        ssize_t size = sendmsg(pipes[1], &msg, 0);  //pipes[1]写入
        free(cmsg);
        if (size == -1) {//发送失败
            printf(" %d  %d  %s   this fd=%d\n", __LINE__, errno, strerror(errno),fd);
            return -2;
        }
        //delete cmsg;
        return 0;
    }

    int RecvSocket(int& fd, const struct sockaddr_in* addr) {//传递引用 便于修改
        msghdr msg;
        bzero(&msg, sizeof(msghdr));
        iovec iov;
        char buffer[20] = "";
        bzero(buffer, sizeof(buffer));
        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));  //真正接收的数据
        memset(cmsg, 0, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;//套接字对
        cmsg->cmsg_type = SCM_RIGHTS;//传过来的数据要拿到权限

        msg.msg_control = cmsg;//控制信息
        msg.msg_controllen = CMSG_LEN(sizeof(int));
        ssize_t size = recvmsg(pipes[0], &msg, 0);//pipes[0]用来接收
        if (size == -1) {//发送失败
            printf(" %d  %d  %s\n", __LINE__, errno, strerror(errno));
            return -2;
        }
        //delete cmsg;
        memcpy((void*)addr, buffer, sizeof(sockaddr_in));
        free(cmsg);
        fd = *(int*)CMSG_DATA(cmsg);  //得到文件描述符
        return 0;
    }

    static int SwitchDeamon() {  //守护进程设置
        pid_t ret = fork();
        if (ret == -1) return -1;
        if (ret > 0) exit(0);//主进程退出
        //子进程
        ret = setsid();
        if (ret == -1) return -2;//setsid()失败  成功继续执行
        ret = fork();
        if (ret == -1) return -3;
        if (ret > 0) exit(0);//子进程退出 
        //孙进程 进入守护状态
        for (int i = 0; i < 3; i++) {
            close(i);  //关闭标准输入输出
        }
        umask(0);//清除文件遮蔽位
        signal(SIGCHLD, SIG_IGN);  //处理signal
        return 0;
    }
private:
    CFunctionBase* m_func;
    pid_t m_pid;
    int pipes[2];//管道
};