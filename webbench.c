/*
 * (C) Radim Kolar 1997-2004
 * This is free software, see GNU Public License version 2 for
 * details.
 *
 * Simple forking WWW Server benchmark:
 *
 * Usage:
 *   webbench --help
 *
 * Return codes:
 *    0 - sucess
 *    1 - benchmark failed (server is not on-line)
 *    2 - bad param
 *    3 - internal error, fork failed
 *
 */
#include "socket.c"
#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>

/* values */
volatile int timerexpired=0; //使用volatile 避免编译器做出优化,使得超时无法被发现
int speed=0; //成功次数
int failed=0; //失败次数
int bytes=0; //读取的bytes数
/* globals */
int http10=1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
/* Allow: GET, HEAD, OPTIONS, TRACE */
#define METHOD_GET 0   //设置http 请求的method
#define METHOD_HEAD 1
#define METHOD_OPTIONS 2
#define METHOD_TRACE 3
#define PROGRAM_VERSION "1.5"
int method=METHOD_GET; //默认GET(HTTP 0.9仅支持该方法)
int clients=1;  //并发数默认为1
int force=0;  //默认等待服务器回应
int force_reload=0; //默认不发送reload请求
int proxyport=80;  //http默认端口号为80
char *proxyhost=NULL;
int benchtime=30;   //测试时间默认为30s
/* internal */
int mypipe[2];  //存储管道的"描述符"
char host[MAXHOSTNAMELEN]; //存储host，并规定最大长度
#define REQUEST_SIZE 2048
char request[REQUEST_SIZE]; //记录请求信息
// 四个参数分别为：
// 1.指定option名字 2.有无参数
// 3.*flag 若NULL 返回参数4, 即val;非NULL, val值赋予flag
// 4.val 依据flag值：直接返回val或者给flag赋值后, 返回0
// 注意: 最后一个元素需要全为0
static const struct option long_options[]=
{
    {"force",no_argument,&force,1},
    {"reload",no_argument,&force_reload,1},
    {"time",required_argument,NULL,'t'},
    {"help",no_argument,NULL,'?'},
    {"http09",no_argument,NULL,'9'},
    {"http10",no_argument,NULL,'1'},
    {"http11",no_argument,NULL,'2'},
    {"get",no_argument,&method,METHOD_GET},
    {"head",no_argument,&method,METHOD_HEAD},
    {"options",no_argument,&method,METHOD_OPTIONS},
    {"trace",no_argument,&method,METHOD_TRACE},
    {"version",no_argument,NULL,'V'},
    {"proxy",required_argument,NULL,'p'},
    {"clients",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

/* prototypes */
static void benchcore(const char* host,const int port, const char *request); //子进程测试的具体实现
static int bench(void); //子进程测试，父进程接受数据，并输出
static void build_request(const char *url); //构建http request

static void alarm_handler(int signal)  //收到SIGALRM 设定timerexpired为1
{
    timerexpired=1;
}
//打印帮助信息
static void usage(void)
{
    fprintf(stderr,
            "webbench [option]... URL\n"
            "  -f|--force               Don't wait for reply from server.\n"
            "  -r|--reload              Send reload request - Pragma: no-cache.\n"
            "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
            "  -p|--proxy <server:port> Use proxy server for request.\n"
            "  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
            "  -9|--http09              Use HTTP/0.9 style requests.\n"
            "  -1|--http10              Use HTTP/1.0 protocol.\n"
            "  -2|--http11              Use HTTP/1.1 protocol.\n"
            "  --get                    Use GET request method.\n"
            "  --head                   Use HEAD request method.\n"
            "  --options                Use OPTIONS request method.\n"
            "  --trace                  Use TRACE request method.\n"
            "  -?|-h|--help             This information.\n"
            "  -V|--version             Display program version.\n"
            );
};

int main(int argc, char *argv[])
{
    int opt=0;
    int options_index=0;
    char *tmp=NULL;
    
    if(argc==1)
    {
        usage();
        return 2;
    }
    //功能: 处理命令行参数
    //前两个参数直接由main()函数传入
    //t,p,c 带参数;未开启 silent mode
    while((opt=getopt_long(argc,argv,"912Vfrt:p:c:?h",long_options,&options_index))!=EOF )
    {
        switch(opt)
        {
            case  0 : break; //long_options 负责处理
            case 'f': force=1;break;
            case 'r': force_reload=1;break;
            case '9': http10=0;break; //使用http 0.9
            case '1': http10=1;break; //使用http 1.0
            case '2': http10=2;break; //使用http 1.1
            case 'V': printf(PROGRAM_VERSION"\n");exit(0);
            case 't': benchtime=atoi(optarg);break;	 //获取的benchtime为string,转换为整型,便于后续处理
            case 'p':
                /* proxy server parsing server:port */
                tmp=strrchr(optarg,':'); //定位最后出现':'的位置
                proxyhost=optarg;  //通过后面的截断处理,最后剩下host部分
                if(tmp==NULL) //没找到':'
                {
                    break;
                }
                if(tmp==optarg) //':'之前没有内容,说明缺失了hostname
                {
                    fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
                    return 2;
                }
                if(tmp==optarg+strlen(optarg)-1) //':'之后没有内容,说明缺失端口号
                {
                    fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
                    return 2;
                }
                *tmp='\0'; //在':'处截断
                proxyport=atoi(tmp+1);break; //将端口号转化为整型
            case ':':                         //疑问：为什么会返回':'? ":XXX" 并没有remain silent
            case 'h':
            case '?': usage();return 2;break;
            case 'c': clients=atoi(optarg);break; //获取的client数量为string,转换为整型,便于后续处理
        }
    }
    
    if(optind==argc) { //缺少url,optind直接到达argc,即最后
        fprintf(stderr,"webbench: Missing URL!\n");
        usage();
        return 2;
    }
    
    if(clients==0) clients=1; 
    if(benchtime==0) benchtime=60; //疑问: 负数的处理呢？
    /* Copyright */ 
    fprintf(stderr,"Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
            "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
            );
    build_request(argv[optind]); //尝试建立请求 需要对非法URL进行检测
    /* print bench info */
    printf("\nBenchmarking: ");
    switch(method)
    {
        case METHOD_GET:
        default:
            printf("GET");break;
        case METHOD_OPTIONS:
            printf("OPTIONS");break;
        case METHOD_HEAD:
            printf("HEAD");break;
        case METHOD_TRACE:
            printf("TRACE");break;
    }
    printf(" %s",argv[optind]); //输出链接
    switch(http10)  //说明使用的协议
    {
        case 0: printf(" (using HTTP/0.9)");break;
        case 2: printf(" (using HTTP/1.1)");break;
    }
    printf("\n");
    if(clients==1) printf("1 client");
    else
        printf("%d clients",clients);
    
    printf(", running %d sec", benchtime);
    if(force) printf(", early socket close");
    if(proxyhost!=NULL) printf(", via proxy server %s:%d",proxyhost,proxyport);
    if(force_reload) printf(", forcing reload");
    printf(".\n");
    return bench();
}

void build_request(const char *url)
{
    char tmp[10];
    int i; //i记录了host开始的位置与开头的"距离"
    
    bzero(host,MAXHOSTNAMELEN); //清零操作,非标准库函数,推荐使用memset()
    bzero(request,REQUEST_SIZE);
    
    if(force_reload && proxyhost!=NULL && http10<1) http10=1;  //http0.9不支持相应功能,调整为http1.0
    if(method==METHOD_HEAD && http10<1) http10=1;  //http0.9没有HEAD 方法,调整为http1.0
    if(method==METHOD_OPTIONS && http10<2) http10=2; //http1.0没有OPTIONS 方法,调整为http1.1
    if(method==METHOD_TRACE && http10<2) http10=2;  //http0.9没有TRACE 方法,调整为http1.1
    
    switch(method)
    {
        default:  //在initial request line添加对应method
        case METHOD_GET: strcpy(request,"GET");break;
        case METHOD_HEAD: strcpy(request,"HEAD");break;
        case METHOD_OPTIONS: strcpy(request,"OPTIONS");break;
        case METHOD_TRACE: strcpy(request,"TRACE");break;
    }
		  
    strcat(request," "); //拼接上url与http verbs之间的空格
    
    if(NULL==strstr(url,"://"))  //判断URL合法性
    {
        fprintf(stderr, "\n%s: is not a valid URL.\n",url);
        exit(2);
    }
    if(strlen(url)>1500)  //长度过长
    {
        fprintf(stderr,"URL is too long.\n");
        exit(2);
    }
    if(proxyhost==NULL) //不使用HTTP协议而且未使用代理服务器,提示使用--proxy参数
        if (0!=strncasecmp("http://",url,7)) //不是'http://'开头 输出提示信息
        { fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
            exit(2);
        }
    /* protocol/host delimiter */  
    i=strstr(url,"://")-url+3;  //协议与host定界符:i为host开始的位置与开头的"距离"
    /* printf("%d\n",i); */
    
    if(strchr(url+i,'/')==NULL) {  //strchr()判断最后是否出现'/'
        fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
        exit(2);
    }
    if(proxyhost==NULL)
    {
        /* get port from hostname */  
        if(index(url+i,':')!=NULL &&          //如果有':'且在第一个'/'之前出现(后面也可能出现':')
           index(url+i,':')<index(url+i,'/'))
        {
            strncpy(host,url+i,strchr(url+i,':')-url-i); //分离host部分(除去Port),算出host长度,并赋值给host
            bzero(tmp,10);
            strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/')-index(url+i,':')-1);//tmp:存储端口号
            /* printf("tmp=%s\n",tmp); */
            proxyport=atoi(tmp); //cast为整型
            if(proxyport==0) proxyport=80; //0帮助选择合适端口->HTTP服务器监听端口默认为80
        } else  
        {
            strncpy(host,url+i,strcspn(url+i,"/")); //分离host(不含port),赋值给host 
        }
        // printf("Host=%s\n",host);
        strcat(request+strlen(request),url+i+strcspn(url+i,"/")); //构建initial request line,request(指针)+strlen()使得在正确的位置添加host
    } else    //代理服务器url直接使用完整url
    {
        // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
        strcat(request,url);
    }
    if(http10==1)  //设置协议类型
        strcat(request," HTTP/1.0");
    else if (http10==2)
        strcat(request," HTTP/1.1");
    strcat(request,"\r\n");
    
    //设置header lines
    if(http10>0)   
        strcat(request,"User-Agent: WebBench "PROGRAM_VERSION"\r\n");
    if(proxyhost==NULL && http10>0)
    {
        strcat(request,"Host: ");
        strcat(request,host);
        strcat(request,"\r\n");
    }
    if(force_reload && proxyhost!=NULL)
    {
        strcat(request,"Pragma: no-cache\r\n");
    }
    if(http10>1)  //HTTP/1.1 中定义,回复后断开连接
        strcat(request,"Connection: close\r\n");
    /* add empty line at end */
    if(http10>0) strcat(request,"\r\n");
    // printf("Req=%s\n",request);
}

/* vraci system rc error kod */
static int bench(void)
{
    int i,j,k;
    pid_t pid=0;
    FILE *f;
    
    /* check avaibility of target server */
    i=Socket(proxyhost==NULL?host:proxyhost,proxyport);
    //出错返回-1
    if(i<0) {
        fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
        return 1;
    }
    close(i); //关闭“描述符”
    /* create pipe */
    if(pipe(mypipe))
    {
        perror("pipe failed.");
        return 3;
    }
    
    /* not needed, since we have () in childrens */
    /* wait 4 next system clock tick */alarm
    /*
     cas=time(NULL);
     while(time(NULL)==cas)
     sched_yield();
     */
    
    /* fork childs */
    for(i=0;i<clients;i++)
    {
        pid=fork();
        if(pid <= (pid_t) 0)
        {
            /* child process or error*/  //系统调度使得子进程不进行进一步的操作,加快fork()速度
	        sleep(1); /* make childs faster */
            break;
        }
    }
    
    if( pid< (pid_t) 0)
    {
        fprintf(stderr,"problems forking worker no. %d\n",i);
        perror("fork failed.");
        return 3;
    }
    
    if(pid== (pid_t) 0) //pid为0 说明是子进程
    {
        /* I am a child */
        if(proxyhost==NULL) //不使用代理服务器
            benchcore(host,proxyport,request);
        else  //使用了代理服务器
            benchcore(proxyhost,proxyport,request);
        
        /* write results to pipe */
        f=fdopen(mypipe[1],"w"); //可以当作普通文件来处理
        if(f==NULL)
        {
            perror("open pipe for writing failed.");
            return 3;
        }
        /* fprintf(stderr,"Child - %d %d\n",speed,failed); */
        fprintf(f,"%d %d %d\n",speed,failed,bytes); //benchcore()获取的数据写入管道,父进程读取
        fclose(f);
        return 0;
    } else   //父进程
    {
        f=fdopen(mypipe[0],"r");  //管道与I/O流结合,直接当做流来操作
        if(f==NULL)
        {
            perror("open pipe for reading failed.");
            return 3;
        }
        setvbuf(f,NULL,_IONBF,0); //_IONBF模式,不使用buffer
        speed=0; //初始化相关统计数据
        failed=0;
        bytes=0;
        
        while(1)
        {
            pid=fscanf(f,"%d %d %d",&i,&j,&k); //读取speed,failed,bytes信息
            if(pid<2) //读取元素少于2,说明i,j,k没读取完整
            {
                fprintf(stderr,"Some of our childrens died.\n");
                break;
            }
            speed+=i; //增加i次成功数据
            failed+=j; //增加j次失败数据
            bytes+=k; //增加k bytes 数据
            /* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
            if(--clients==0) break; //clients数目的子进程数据读取完毕
        }
        fclose(f);//关闭对应文件指针
        //计算数据并输出
        printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
               (int)((speed+failed)/(benchtime/60.0f)),
               (int)(bytes/(float)benchtime),
               speed,
               failed);
    }
    return i;
}

void benchcore(const char *host,const int port,const char *req)
{
    int rlen;  //记录req的长度
    char buf[1500];
    int s,i;
    struct sigaction sa;
    
    /* setup alarm signal handler */
    sa.sa_handler=alarm_handler;
    sa.sa_flags=0; //
    if(sigaction(SIGALRM,&sa,NULL)) //成功返回0,否则退出
        exit(3);
    alarm(benchtime); //让系统在benchtime之后产生SIGALRM信号,俘获该信号,timerexpired＝1(见handler)
    
    rlen=strlen(req);
nexttry:while(1)
{
    if(timerexpired) 
    {
        if(failed>0)
        {
            /* fprintf(stderr,"Correcting failed by signal\n"); */
            failed--;
        }
        return;
    }
    s=Socket(host,port);  //获取sd,并使用connect()建立连接                        
    if(s<0) { failed++;continue;} //-1失败
    if(rlen!=write(s,req,rlen)) {failed++;close(s);continue;} //write()返回写入的byte数目,不等于rlen说明出错
    if(http10==0)  //若使用http0.9,利用shutdown()关闭发送端
        if(shutdown(s,1))  //shutdown()失败返回-1,成功返回0,并使得无法"发送"
        {   failed++;close(s);
            continue;
        }
    if(force==0) 
    {
        /* read all available data from socket */
        while(1)
        {
            if(timerexpired) break;  //volatile 检查是否超时
            i=read(s,buf,1500); //i为成功读取到的bytes数
            /* fprintf(stderr,"%d\n",i); */
            if(i<0) //发生错误
            { 
                failed++;
                close(s);
                goto nexttry;
            }
            else if(i==0) //无可读数据
                break; 
            else  
                bytes+=i; //子进程的变量改变不会影响父进程
        }
    }
    if(close(s)) {failed++;continue;} //close()释放descriptor
    speed++;
}
}
