#include"Process.h"
#include "Logger.h"
#include"ThreadPool.h"
#include"PlayerServer.h"
#include"HttpParser.h"
#include"Sqlite3Client.h"
int CreateLogServer(CProcess* proc) {
    //printf("日志程序%s(%d):<%s> pid=%d\n", __FILE__,__LINE__,__FUNCTION__,getpid());
    CLoggerServer server;
    int ret = server.Start();
    if (ret != 0) {
        printf("日志程序%s(%d):<%s> pid=%d ret=%d error code=%d  msg=%s\n"
            , __FILE__, __LINE__, __FUNCTION__, getpid(),ret,errno,strerror(errno));
    }
    int fd = 0;
    while(true){
        proc->RecvFD(fd);
        printf("日志程序%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
        if (fd <= 0) {
            break;
        }
    }
    ret = server.Close();
    printf("日志程序%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    return 0;
}
int CreateClientServer(CProcess* proc) {
    //printf("网络程序%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    int fd = -1;
    int ret=proc->RecvFD(fd);
    if (ret != 0) {
        printf(" %d  %d  %s\n", __LINE__,errno, strerror(errno));
    }
    sleep(1);
    char buffer[10]="";
    lseek(fd, 0, SEEK_SET);//恢复fd初始
    ret = (int)read(fd, buffer, sizeof(buffer));
    //printf("read size=%d  read buffer=%s\n", ret,buffer);
    close(fd);
    return 0;
}

int LogTest() {
    printf(" %s(%d):%s \n", __FILE__, __LINE__, __FUNCTION__);
    char buffer[] = "hello edoyun!  你好";
    usleep(1000 * 100);//等待子进程启动
    printf(" %s(%d):%s \n", __FILE__, __LINE__, __FUNCTION__);
    TRACEI("here is log %d %c %f %g %s  哈哈 嘻嘻",10,'A',1.0f,2.0,buffer);
    DUMPD((void*)buffer, (size_t)sizeof(buffer));
    LOGE << 100 << "   " << 's' << "  " << 0.12345 << "    " << 1.12345678 << "   " << buffer << "你好";
    //printf(" %s(%d):%s \n", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

int oldTest() {
    //CProcess::SwitchDeamon();  //转到守护进程
    CProcess procLog, procClient;
    procLog.SetEctryFunc(CreateLogServer, &procLog);
    int ret = procLog.CreateSubPro();
    if (ret != 0) {
        printf("主程序%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -1;
    }
    ret = LogTest();
    CThread thread(LogTest);
    thread.Start();
    procClient.SetEctryFunc(CreateClientServer, &procClient);
    ret = procClient.CreateSubPro();
    if (ret != 0) {
        printf("主程序%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -2;
    }
    usleep(10000);
    int fd = open("./test.txt", O_RDWR | O_CREAT | O_APPEND);//读写 不存在就创建 存在就追加写
    if (fd == -1) {
        printf("文件创建失败");
        return -3;
    }
    ret = procClient.SendFD(fd);
    if (ret != 0) {
        printf(" %d  %d  %s\n", __LINE__, errno, strerror(errno));
    }
    ret = (int)write(fd, "edoyun", 6);//发送完文件描述符再写入
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);
    close(fd);

    ThreadPool pool;
    ret = pool.Start(4);
    ret = pool.AddTask(LogTest);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pool.AddTask(LogTest);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pool.AddTask(LogTest);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pool.AddTask(LogTest);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);
    (void)getchar();
    pool.Close();
    procLog.SendFD(-1);  //停止信号
    (void)getchar();
    return 0;
}

int Main() {
    int ret = 0;
    CProcess procLog;
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ret = procLog.SetEctryFunc(CreateLogServer, &procLog);
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ERR_RETURN(ret, -1);
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ret = procLog.CreateSubPro();
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ERR_RETURN(ret, -2);
    CPlayerServer business(2);
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    CServer server;
    ret = server.Init(&business);
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ERR_RETURN(ret, -3);
    ret = server.Run();
    printf("日志程序%s(%d):<%s>\n", __FILE__, __LINE__, __FUNCTION__);
    ERR_RETURN(ret, -4);
    return 0;
}

int httpTest() {
	Buffer str = "GET /favicon.ico HTTP/1.1\r\n"
		"Host: 0.0.0.0=5000\r\n"
		"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*; q = 0.8\r\n"
		"Accept-Language: en-us,en;q=0.5\r\n"
		"Accept-Encoding: gzip,deflate\r\n"
		"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
		"Keep-Alive: 300\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
    CHttpParser parser;
    size_t size = parser.Parser(str);
    if (parser.Errno() != 0) {//解析失败
        printf("%s(%d)[%s]  errno %d\n", __FILE__, __LINE__, __FUNCTION__, parser.Errno());
        return -1;
    }
    if (size != 368) {//未解析完
        printf("%s(%d)[%s]  errno (%d) size:%d  strlen=%d\n", __FILE__, __LINE__, __FUNCTION__, parser.Errno(),size,str.length());
        return -2;
    }
    printf("%s(%d)[%s]  method (%d)  URL:[%s]\n", __FILE__, __LINE__, __FUNCTION__, parser.Method(), (char*)parser.URL());
    str="GET /favicon.ico HTTP/1.1\r\n"
		"Host: 0.0.0.0=5000\r\n"
		"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    size = parser.Parser(str);
    printf("%s(%d)[%s]    size (%d) error:(%d)\n", __FILE__, __LINE__, __FUNCTION__, size, parser.Errno());
    if (parser.Errno() != 0x7F) {
        return -3;
    }
    if (size != 0) {
        return -4;
    }
    printf("%s(%d)[%s]\n", __FILE__, __LINE__, __FUNCTION__);
    CUrlParser url1("https://www.baidu.com/s?ie=utf8&oe=utf8&wd=httplib&tn=98010089_dg&ch=3");
    CUrlParser url2("http://127.0.0.1:19811/?time=144000&salt=9527&user=test&sign=1234567890abcdef");
    int ret = url1.Parser();
    if (ret != 0) {
        printf("%s(%d)[%s]  urlParser failed ret:%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
        return -5;
    }
    ret = url2.Parser();
    if (ret != 0) {
        printf("%s(%d)[%s]  urlParser failed ret:%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
        return -6;
    }
    printf("ie = %s  expect:utf8\n", (char*)url1["ie"]);
    printf("oe = %s  expect:utf8\n", (char*)url1["oe"]);
    printf("wd = %s  expect:httplib\n", (char*)url1["wd"]);
    printf("tn = %s  expect:98010089_dg\n", (char*)url1["tn"]);
    printf("ch = %s  expect:3\n", (char*)url1["ch"]);


    printf("time = %s  expect:144000\n", (char*)url2["time"]);
    printf("salt = %s  expect:9527\n", (char*)url2["salt"]);
    printf("user = %s  expect:test\n", (char*)url2["user"]);
    printf("sign = %s  expect:1234567890abcdef\n", (char*)url2["sign"]);
    return 0;
}


/*
* (int ntype,const Buffer& name, unsigned attr,
        const Buffer& type, const Buffer& size,
        const Buffer& default_, const Buffer& check)
*/
/*
class user_test :public _sqlite3_table_ {
public:
    virtual PTable Copy() const {
        return PTable(new user_test(*this));
    }
    user_test() :_sqlite3_table_() {
        Name = "user_test";
        {
            PField field(new _sqlite3_field_(TYPE_INT, "user_id", NOT_NULL | PRIMARYKEY | AUTOINCREMENT, "INT", "", "", ""));  //创建的列
            FieldDefine.push_back(field);
            Fields["user_id"] = field;
        }
        {
            PField field(new _sqlite3_field_(TYPE_VARCHAR, "user_qq", NOT_NULL | PRIMARYKEY | AUTOINCREMENT, "VARCHAR", "(15)", "", ""));  //创建的列
            FieldDefine.push_back(field);
            Fields["user_qq"] = field;
        }
    }
};
*/

/**
#include"MysqlClient.h"
DECLARE_TABLE_CLASS(user_test, _sqlite3_table_)
DECLARE__MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER","","","")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL|DEFAULT, "VARCHER", "(12)", "18888888888", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_END()




int sqlTest() {
    user_test test,value;

    printf("Create:%s\n", (char*)test.Create());
    printf("Drop:%s\n", (char*)test.Drop());
    printf("Delete:%s\n", (char*)test.Delete(test));
    value.Fields["user_qq"]->LoadFromStr("1249233018");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert:%s\n", (char*)value.Insert(value));
    value.Fields["user_qq"]->LoadFromStr("1999999999");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    printf("Modify:%s\n", (char*)test.Modify(value));
    printf("Query:%s\n", (char*)test.Query());
    printf("Drop:%s\n", (char*)test.Drop());
    //getchar();

    int ret = 0;
    CDataBaseClient* pClient = new CSqlite3Client();
    KeyValue args;
    args["host"] = "test.db";
    ret = pClient->Connect(args);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Create());
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Delete(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    value.Fields["user_qq"]->LoadFromStr("1249233018");

    value.Fields["user_qq"]->Condition = SQL_INSERT;
    ret = pClient->Exec(test.Insert(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    value.Fields["user_qq"]->LoadFromStr("1999999999");

    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    ret = pClient->Exec(test.Modify(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    Result result;
    ret = pClient->Exec(test.Query(),result,test);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Drop());
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Close();
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);


    return 0;
}

*/

#include"MysqlClient.h"
DECLARE_TABLE_CLASS(user_test_mysql, _mysql_table_)
DECLARE__MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE__MYSQL_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL | DEFAULT, "VARCHAR", "(12)", "18888888888", "")
DECLARE__MYSQL_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_END()


int mysqlTest() {
    user_test_mysql test, value;
    printf("Create:%s\n", (char*)test.Create());
    printf("Drop:%s\n", (char*)test.Drop());
    printf("Delete:%s\n", (char*)test.Delete(test));
    value.Fields["user_qq"]->LoadFromStr("1249233018");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert:%s\n", (char*)value.Insert(value));
    value.Fields["user_qq"]->LoadFromStr("1999999999");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    printf("Modify:%s\n", (char*)test.Modify(value));
    printf("Query:%s\n", (char*)test.Query());
    printf("Drop:%s\n", (char*)test.Drop());
    getchar();

    int ret = 0;
    CDataBaseClient* pClient = new CMysqlClient();
    KeyValue args;
    args["host"] = "10.164.241.100";
    args["user"] = "test";
    args["password"] = "123456";
    args["port"] = "3306";
    args["db"] = "test";
    ret = pClient->Connect(args);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Create());
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Delete(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    value.Fields["user_qq"]->LoadFromStr("1249233018");

    value.Fields["user_qq"]->Condition = SQL_INSERT;
    ret = pClient->Exec(test.Insert(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    value.Fields["user_qq"]->LoadFromStr("1999999999");

    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    ret = pClient->Exec(test.Modify(value));
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    Result result;
    ret = pClient->Exec(test.Query(), result, test);
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Exec(test.Drop());
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);

    ret = pClient->Close();
    printf(" %s(%d):%s  ret = %d \n", __FILE__, __LINE__, __FUNCTION__, ret);


    return 0;
}


#include"Crypto.h"
int cryptoTest() {
    Buffer data = "abcdef";
    data=Crypto::MD5(data);
    printf("EXCEPT:[E80B5017098950FC58AAD83C8C14978E]\n");
    printf("RESULT:[%s]\n", (char*)data);
    return 0;
}
int main()
{
    //int ret = httpTest();
    //printf("main:ret=%d\n", ret);
    int ret = 0;
    //ret = sqlTest();
    //ret = mysqlTest();
    //ret = cryptoTest();
    ret = Main();
    return ret;
}